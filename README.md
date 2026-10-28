# Mini EPICS Control System

โปรเจกต์ Lab สำหรับเรียนรู้การสร้าง Control System ขนาดเล็กด้วย EPICS, ESP32 และ Phoebus โดยอ่านอุณหภูมิและความชื้นจาก DHT22 ส่งค่าเข้า EPICS PV และแสดงผลผ่าน Operator Interface บน Windows

ระบบรองรับ Alarm-to-LED: เมื่ออุณหภูมิถึงระดับ `MINOR` จะเปิด LED เหลือง และเมื่อถึงระดับ `MAJOR` จะเปิด LED แดง

## System Architecture

```text
DHT22
  ↓ GPIO4
ESP32-WROOM-32
  ↓ USB Serial (/dev/ttyUSB0)
Python bridge
  ↓ Channel Access
EPICS softIOC
  ↓ CA ผ่าน VirtualBox Host-only network
Phoebus Dashboard บน Windows

EPICS Alarm severity
  ↓ Python bridge ส่ง ALARM=...
ESP32
  ├── GPIO18 → LED เหลือง (MINOR)
  └── GPIO19 → LED แดง (MAJOR)
```

## Features

- อ่าน Temperature และ Humidity จาก DHT22 ทุก 2 วินาที
- ส่งข้อมูลผ่าน USB Serial ในรูปแบบ `TEMP=27.3,HUM=59.4`
- Python bridge เขียนค่าจริงเข้า EPICS PV
- รองรับ Simulation IOC สำหรับฝึกใช้งานโดยไม่ต้องต่อ Hardware
- แสดงค่าและ Alarm ผ่าน Phoebus Display Builder
- ใช้ EPICS alarm fields กำหนด `MINOR` และ `MAJOR`
- ควบคุม LED เหลือง/แดงตาม `ESP32:TEMP.SEVR`

## Hardware

- ESP32-WROOM-32 development board
- DHT22 แบบ 3 ขา (`-`, `OUT`, `+`)
- LED เหลือง 1 ดวง
- LED แดง 1 ดวง
- Resistor 220–330 Ω จำนวน 2 ตัว
- Breadboard และสาย Jumper

### Wiring

```text
DHT22 +    → ESP32 3V3
DHT22 OUT  → ESP32 GPIO4
DHT22 -    → ESP32 GND

GPIO18 → Resistor → ขายาว LED เหลือง
ขาสั้น LED เหลือง → GND

GPIO19 → Resistor → ขายาว LED แดง
ขาสั้น LED แดง → GND
```

> ห้ามต่อ LED เข้ากับ GPIO โดยไม่มี Resistor

## EPICS PVs

| PV | Record | หน่วย | หน้าที่ |
|---|---|---|---|
| `ESP32:TEMP` | `ai` | `degC` | อุณหภูมิจาก DHT22 |
| `ESP32:HUMIDITY` | `ai` | `%RH` | ความชื้นจาก DHT22 |
| `ESP32:TEMP.STAT` | Common field | — | สถานะ Alarm เช่น `HIGH`, `HIHI` |
| `ESP32:TEMP.SEVR` | Common field | — | Severity เช่น `NO_ALARM`, `MINOR`, `MAJOR` |

Temperature alarm configuration:

| เงื่อนไข | Status | Severity | LED |
|---|---|---|---|
| ต่ำกว่า 28.5 °C | `NO_ALARM` | `NO_ALARM` | ดับทั้งคู่ |
| ตั้งแต่ 28.5 °C | `HIGH` | `MINOR` | เหลือง |
| ตั้งแต่ 29.5 °C | `HIHI` | `MAJOR` | แดง |

Alarm hysteresis ตั้งไว้ที่ `0.2 °C`

## Prerequisites

- Ubuntu VM บน Oracle VirtualBox
- EPICS Base 7.0.10
- Arduino CLI และ ESP32 core
- Python 3 พร้อม `venv`
- Phoebus/CS-Studio บน Windows
- VirtualBox USB passthrough สำหรับ CP2102
- VirtualBox Host-only network สำหรับ Channel Access

## Installation

Clone repository บน Ubuntu VM:

```bash
git clone https://github.com/Snxxze/mini-epics-control-system.git
cd mini-epics-control-system
```

สร้าง Python virtual environment และติดตั้ง dependency:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

ติดตั้ง DHT library หากยังไม่มี:

```bash
arduino-cli lib install "DHT sensor library"
```

## Build and Upload Firmware

ตรวจ Serial port:

```bash
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Compile และ Upload:

```bash
arduino-cli compile --upload \
  --port /dev/ttyUSB0 \
  --fqbn esp32:esp32:esp32 \
  "$HOME/mini-epics-control-system/firmware/esp32_dht22"
```

ดูข้อมูลจาก ESP32:

```bash
arduino-cli monitor \
  --port /dev/ttyUSB0 \
  --config baudrate=115200
```

> ปิด Serial Monitor ด้วย `Ctrl+C` ก่อนเปิด Python bridge เพราะทั้งสองโปรแกรมใช้ `/dev/ttyUSB0` พร้อมกันไม่ได้

## Run with Real Hardware

รันสอง Terminal พร้อมกัน

### Terminal 1 — EPICS IOC

```bash
cd ~/mini-epics-control-system
softIoc ioc/st-real.cmd
```

ไฟล์ `ioc/st-real.cmd` ผูก CA server กับ Host-only IP `192.168.56.105` หาก VM ได้ IP อื่นให้แก้ค่านี้ก่อนรัน

### Terminal 2 — Python bridge

```bash
cd ~/mini-epics-control-system
source .venv/bin/activate

export EPICS_CA_AUTO_ADDR_LIST=NO
export EPICS_CA_ADDR_LIST=192.168.56.105

python bridge/esp32_bridge.py
```

ตรวจค่าใน Terminal ที่สาม:

```bash
caget ESP32:TEMP
caget ESP32:HUMIDITY
camonitor -m va ESP32:TEMP ESP32:HUMIDITY
```

## Run in Simulation Mode

```bash
cd ~/mini-epics-control-system
softIoc ioc/st.cmd
```

> ห้ามเปิด `ioc/st.cmd` และ `ioc/st-real.cmd` พร้อมกัน เพราะทั้งสอง IOC ใช้ชื่อ PV เดียวกัน

## Phoebus

สร้าง `settings.ini` ไว้ในโฟลเดอร์เดียวกับ `phoebus.bat`:

```ini
org.phoebus.pv.ca/addr_list=192.168.56.105
org.phoebus.pv.ca/auto_addr_list=false
```

หาก Host-only IP ของ VM เปลี่ยน ต้องแก้ IP ในไฟล์นี้และ `ioc/st-real.cmd`

เปิด Dashboard:

```text
phoebus/esp32-dashboard.bob
```

PV ที่ใช้ใน Widget:

```text
ca://ESP32:TEMP
ca://ESP32:HUMIDITY
```

## Project Structure

```text
mini-epics-control-system/
├── bridge/
│   └── esp32_bridge.py
├── docs/
│   ├── commands.md
│   └── lab-notes.md
├── firmware/
│   └── esp32_dht22/
│       └── esp32_dht22.ino
├── ioc/
│   ├── db/
│   │   ├── esp32-real.db
│   │   └── esp32-sim.db
│   ├── st-real.cmd
│   └── st.cmd
├── phoebus/
│   └── esp32-dashboard.bob
├── Mini_EPICS_Control_System.md
├── README.md
└── requirements.txt
```

## Documentation

- [Command Cheat Sheet](docs/commands.md)
- [Lab Notes](docs/lab-notes.md)
- [Mini EPICS Control System Guide](Mini_EPICS_Control_System.md)

## Quick Troubleshooting

ESP32 ไม่ปรากฏใน Ubuntu:

```bash
lsusb
ls -l /dev/ttyUSB* 2>/dev/null
```

Python หา PV ไม่เจอ:

```bash
pgrep -af softIoc
ip -4 addr
caget -w 3 ESP32:TEMP
```

พบ `Identical process variable names on multiple servers`:

- ตรวจว่าเปิด IOC เพียงตัวเดียว
- ตรวจ `EPICS_CAS_INTF_ADDR_LIST` ใน `ioc/st-real.cmd`
- กำหนด `EPICS_CA_AUTO_ADDR_LIST=NO` ให้ CA client

ดูคำสั่งและวิธีแก้ปัญหาเพิ่มเติมได้ที่ [docs/commands.md](docs/commands.md)
