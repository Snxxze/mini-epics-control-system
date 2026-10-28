import serial
from epics import caput, caget

SERIAL_PORT = "/dev/ttyUSB0"
BAUD_RATE = 115200

TEMP_PV = "ESP32:TEMP"
HUMIDITY_PV = "ESP32:HUMIDITY"

def parse_sensor_data(line):
    if not line.startswith("TEMP="):
        return None

    try:
        parts = line.split(",")

        temperature = float(parts[0].split("=")[1])
        humidity = float(parts[1].split("=")[1])

        return temperature, humidity

    except (IndexError, ValueError):
        return None


def get_alarm_command():
    severity = caget(f"{TEMP_PV}.SEVR", as_string=True, timeout=2)
    
    if severity == "MINOR":
        return "ALARM=MINOR"

    if severity == "MAJOR":
        return "ALARM=MAJOR"

    return "ALARM=NONE"


with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2) as esp32:
    print(f"Connected to {SERIAL_PORT}")

    last_alarm_command = None

    try:
        while True:
            line = (
                esp32.readline()
                .decode("utf-8", errors="replace")
                .strip()
            )

            if not line:
                continue

            data = parse_sensor_data(line)

            if data is None:
                print(f"Ignored: {line}")
                continue

            temperature, humidity = data

            temp_written = caput(
                    TEMP_PV, 
                    temperature, 
                    wait=True, 
                    timeout=2
            )

            humidity_written = caput(
                    HUMIDITY_PV, 
                    humidity,
                    wait=True, 
                    timeout=2
            )

            if temp_written is None or humidity_written is None:
                print("Failed to write EPICS PV")
                continue

            alarm_command = get_alarm_command()

            esp32.write(
                f"{alarm_command}\n".encode("utf-8")
            )

            if alarm_command != last_alarm_command:
                print(f"Alarm changed: {alarm_command}")
                last_alarm_command = alarm_command

            print(
                    f"Temperature: {temperature:.1f} C | "
                    f"Humidity: {humidity:.1f} %"
            )

    except KeyboardInterrupt:
        print("\nBridge stopped")
