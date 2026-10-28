#include <DHT.h>

const int DHT_PIN = 4;
const int DHT_TYPE = DHT22;

const int YELLOW_LED_PIN = 18;
const int RED_LED_PIN = 19;

DHT dht(DHT_PIN, DHT_TYPE);

void setAlarmLeds(const String& alarmLevel) {
  if (alarmLevel == "MINOR") {
    digitalWrite(YELLOW_LED_PIN, HIGH); 
    digitalWrite(RED_LED_PIN, LOW); 
  }
  else if (alarmLevel == "MAJOR") { 
    digitalWrite(YELLOW_LED_PIN, LOW); 
    digitalWrite(RED_LED_PIN, HIGH); 
  }
  else {
    digitalWrite(YELLOW_LED_PIN, LOW); 
    digitalWrite(RED_LED_PIN, LOW); 
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  // LED self-test
  digitalWrite(YELLOW_LED_PIN, HIGH);
  delay(500);
  digitalWrite(YELLOW_LED_PIN, LOW);

  digitalWrite(RED_LED_PIN, HIGH);
  delay(500);
  digitalWrite(RED_LED_PIN, LOW);

  dht.begin();

  delay(2000);
  Serial.println("ESP32 DHT22 ready");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("ALARM=")) {
      String alarmLevel = command.substring(6);
      setAlarmLeds(alarmLevel);
    }
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("ERROR=DHT_READ_FAILED");
    delay(2000);
    return;
  }

  Serial.print("TEMP=");
  Serial.print(temperature, 1);
  Serial.print(" ,HUM=");
  Serial.println(humidity, 1);

  delay(2000);
}
