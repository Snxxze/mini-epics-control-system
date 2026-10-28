# Mini EPICS Command Cheat Sheet

รวมคำสั่งที่ใช้บ่อยในโปรเจกต์ ESP32 + DHT22 + Python bridge + EPICS + Phoebus

> รันคำสั่ง Linux จาก Ubuntu VM เว้นแต่หัวข้อจะระบุว่าเป็น PowerShell บน Windows

## 1. เข้า Ubuntu VM ผ่าน SSH

รันจาก PowerShell บน Windows:

```powershell
ssh -p 2223 gowit@127.0.0.1
```

เข้าโฟลเดอร์โปรเจกต์:

```bash
cd ~/mini-epics-control-system
```

## 2. ตรวจ Environment

```bash
cat /etc/os-release
uname -m
hostname
ip -4 addr
```

ตรวจคำสั่ง EPICS และ Arduino CLI:

```bash
command -v softIoc
command -v arduino-cli
```

โหลดค่าจาก `.bashrc` ใหม่:

```bash
source ~/.bashrc
```

ค่าที่เพิ่มไว้ใน `.bashrc`:

```bash
export EPICS_BASE="$HOME/EPICS/base-7.0.10"
export EPICS_HOST_ARCH="linux-x86_64"
export PATH="$EPICS_BASE/bin/$EPICS_HOST_ARCH:$PATH"
export PATH="$HOME/.local/bin:$PATH"
```

## 3. Git

ดูสถานะไฟล์:

```bash
git status
git diff
```

บันทึกงานและส่งขึ้น GitHub:

```bash
git add .
git commit -m "Describe the change"
git push
```

ดึงงานล่าสุด:

```bash
git pull
```

คืนไฟล์เดียวเป็นเวอร์ชันจาก commit ล่าสุด:

```bash
git restore ioc/db/esp32-sim.db
```

ถ้าไฟล์ถูก `git add` แล้ว:

```bash
git restore --staged ioc/db/esp32-sim.db
git restore ioc/db/esp32-sim.db
```

> `git restore` จะทิ้งการแก้ไขที่ยังไม่ได้ commit ของไฟล์นั้น

## 4. รัน EPICS IOC

รัน IOC แบบ Simulation:

```bash
cd ~/mini-epics-control-system
softIoc ioc/st.cmd
```

รัน IOC แบบ Hardware จริง:

```bash
cd ~/mini-epics-control-system
softIoc ioc/st-real.cmd
```

คำสั่งที่ใช้ตรง prompt `epics>`:

```text
dbl
dbpr "ESP32:TEMP"
dbpr "ESP32:HUMIDITY"
scanppl
exit
```

- `dbl` แสดงชื่อ Record/PV ที่โหลดอยู่
- `dbpr` แสดงรายละเอียดของ Record
- `scanppl` แสดง Record ใน periodic scan list
- `exit` ปิด IOC

หาก `exit` ไม่ทำงาน ให้กด `Ctrl+C`

## 5. ใช้งาน EPICS PV

อ่านค่า:

```bash
caget ESP32:TEMP
caget ESP32:HUMIDITY
caget ESP32:LED
```

เขียนค่า:

```bash
caput ESP32:TEMP 28.5
caput ESP32:HUMIDITY 65.0
caput ESP32:LED 1
caput ESP32:LED 0
```

ติดตามค่าที่เปลี่ยนแปลง:

```bash
camonitor ESP32:TEMP
camonitor ESP32:HUMIDITY
camonitor -m va ESP32:TEMP
```

> `camonitor` ไม่มี option `-a`; ใช้ `-m va` เมื่อต้องการ value และ alarm metadata

## 6. ตรวจ USB Serial ของ ESP32

```bash
lsusb
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
groups
```

พอร์ตที่ใช้ในโปรเจกต์:

```text
/dev/ttyUSB0
```

ถ้า user ไม่มี group `dialout`:

```bash
sudo usermod -aG dialout gowit
```

จากนั้น logout และ login ใหม่

ดู kernel log หลังเสียบ ESP32:

```bash
sudo dmesg | tail -n 40
```

## 7. Arduino CLI และ ESP32 Firmware

ตรวจเวอร์ชันและบอร์ด:

```bash
arduino-cli version
arduino-cli board list
arduino-cli core list
arduino-cli lib list
```

`Unknown` ใน `board list` เป็นเรื่องปกติสำหรับ CP2102 โดยใช้ FQBN นี้:

```text
esp32:esp32:esp32
```

Compile firmware:

```bash
arduino-cli compile \
  --fqbn esp32:esp32:esp32 \
  firmware/esp32_dht22
```

Compile และ Upload ในคำสั่งเดียว:

```bash
arduino-cli compile --upload \
  --port /dev/ttyUSB0 \
  --fqbn esp32:esp32:esp32 \
  "$HOME/mini-epics-control-system/firmware/esp32_dht22"
```

เปิด Serial Monitor ที่ baud rate `115200`:

```bash
arduino-cli monitor \
  --port /dev/ttyUSB0 \
  --config baudrate=115200
```

หยุด Serial Monitor ด้วย `Ctrl+C`

> Serial Monitor กับ Python bridge เปิด `/dev/ttyUSB0` พร้อมกันไม่ได้ ต้องปิดตัวหนึ่งก่อนเปิดอีกตัว

## 8. ติดตั้ง Arduino CLI ใหม่

```bash
mkdir -p "$HOME/.local/bin"
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
  | BINDIR="$HOME/.local/bin" sh
```

สร้าง config และติดตั้ง ESP32 core:

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

ติดตั้ง DHT library:

```bash
arduino-cli lib install "DHT sensor library"
```

## 9. Python Virtual Environment และ Bridge

ติดตั้ง venv package หากยังไม่มี:

```bash
sudo apt update
sudo apt install python3-venv
```

สร้างและเปิด virtual environment:

```bash
cd ~/mini-epics-control-system
python3 -m venv .venv
source .venv/bin/activate
```

ติดตั้ง dependency จากโปรเจกต์:

```bash
python -m pip install -r requirements.txt
```

หากยังไม่มี `requirements.txt`:

```bash
python -m pip install pyserial pyepics
python -m pip freeze > requirements.txt
```

รัน Python bridge:

```bash
python bridge/esp32_bridge.py
```

หยุด bridge ด้วย `Ctrl+C`

ออกจาก virtual environment:

```bash
deactivate
```

## 10. Phoebus และ Channel Access

Host-only network ของ Lab นี้:

```text
Windows host: 192.168.56.1
Ubuntu VM:    192.168.56.105
```

ตรวจ IP ฝั่ง Ubuntu:

```bash
ip -4 addr
```

ทดสอบจาก PowerShell บน Windows:

```powershell
ping 192.168.56.105
```

ไฟล์ `settings.ini` ในโฟลเดอร์ Phoebus:

```ini
org.phoebus.pv.ca/addr_list=192.168.56.105
org.phoebus.pv.ca/auto_addr_list=false
```

ชื่อ PV ที่ใช้ใน Phoebus Probe:

```text
ca://ESP32:TEMP
ca://ESP32:HUMIDITY
```

## 11. Terminal ที่ควรเปิดตอนรัน Hardware จริง

Terminal 1 — EPICS IOC:

```bash
cd ~/mini-epics-control-system
softIoc ioc/st-real.cmd
```

Terminal 2 — Python bridge:

```bash
cd ~/mini-epics-control-system
source .venv/bin/activate
python bridge/esp32_bridge.py
```

Windows — Phoebus:

```text
ca://ESP32:TEMP
ca://ESP32:HUMIDITY
```

## 12. Vim ที่ใช้บ่อย

```text
i       เข้า Insert mode
Esc     ออกจาก Insert mode
:w      บันทึก
:q      ออก
:wq     บันทึกแล้วออก
:q!     ออกโดยไม่บันทึก
/text   ค้นหา text
n       ไปผลการค้นหาถัดไป
G       ไปท้ายไฟล์
o       เพิ่มบรรทัดใหม่ด้านล่าง
```

