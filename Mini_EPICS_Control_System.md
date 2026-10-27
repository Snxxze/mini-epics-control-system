# ระบบควบคุม Mini EPICS

> **สถานะ:** อยู่ระหว่างการวางแผน / Learning Lab — ยังไม่ได้เริ่ม implement  
> **Environment:** Linux Virtual Machine (VirtualBox)  
> **เป้าหมายหลัก:** ทำความเข้าใจ EPICS IOC, Records, Process Variables, Channel Access และ flow ของการควบคุมกับ readback โดยไม่ต้องใช้ hardware จริง

## 1. ภาพรวมโปรเจกต์

สร้างระบบควบคุม EPICS ขนาดเล็กที่จำลอง ESP32 ซึ่งเชื่อมต่อกับ sensor วัดอุณหภูมิ/ความชื้นและ LED ในระยะแรก อุปกรณ์ทั้งหมดจะถูก **จำลองด้วย software** บน Linux VM หลังจากนั้นจึงสามารถเปลี่ยน backend ของ PV ชุดเดิมที่ client ใช้งาน ให้เชื่อมต่อกับ ESP32 จริงผ่าน communication adapter ได้

### ขอบเขตงาน

- รัน EPICS Soft IOC บน Linux ภายใน VirtualBox
- เปิดให้ใช้งาน Process Variable จำนวน 4 ตัว ซึ่งแทนค่าที่อ่านจาก sensor รวมถึงคำสั่งและ readback ของ LED
- ใช้ EPICS Channel Access client (`caget`, `caput`, `camonitor`) เพื่อตรวจสอบและโต้ตอบกับ PV
- แยกความแตกต่างระหว่าง **สถานะที่สั่ง (commanded state)** และ **สถานะที่อุปกรณ์รายงาน (reported state)**
- สาธิตการประมวลผล Record และพฤติกรรมของอุปกรณ์จำลองใน Lab ถัดไป

### สิ่งที่ไม่รวมอยู่ใน Phase 1

- การต่อ ESP32, DHT22 และ LED จริง
- Production hardware driver, MQTT broker, Bluesky orchestration, authentication และ production deployment

## 2. Architecture ที่เสนอ

```text
Windows Host
└── VirtualBox
    └── Linux VM
        ├── EPICS Client (Terminal A)
        │   ├── caget
        │   ├── caput
        │   └── camonitor
        │          │
        │          │ Channel Access (localhost)
        │          ▼
        └── EPICS Soft IOC (Terminal B)
            ├── IOC Database (Records ในหน่วยความจำ)
            │   ├── ESP32:TEMP
            │   ├── ESP32:HUMIDITY
            │   ├── ESP32:LED
            │   └── ESP32:LED:RBV
            └── Logic ของอุปกรณ์จำลอง
                ├── Virtual temperature sensor
                ├── Virtual humidity sensor
                └── Virtual LED พร้อม reported state
```

**ข้อสำคัญ:** EPICS IOC database คือชุดของ runtime records **ไม่ใช่** SQL database ใน Phase 1 prefix `ESP32:` เป็นเพียง naming convention สำหรับการเรียนรู้ ไม่ได้ยืนยันว่ามี ESP32 เชื่อมต่ออยู่จริง

## 3. IOC Records / PV Contract ที่เสนอ

| ชื่อ PV | Record type | ค่าตัวอย่าง | วัตถุประสงค์ | สิทธิ์การใช้งานที่ตั้งใจไว้ |
|---|---|---:|---|---|
| `ESP32:TEMP` | `ai` (Analog Input) | `28.5` | อุณหภูมิจำลอง | Read |
| `ESP32:HUMIDITY` | `ai` (Analog Input) | `65.0` | ความชื้นสัมพัทธ์จำลอง | Read |
| `ESP32:LED` | `bo` (Binary Output) | `1` | สถานะ LED ที่ต้องการ (`0` OFF, `1` ON) | Write / Read |
| `ESP32:LED:RBV` | `bi` (Binary Input) | `1` | สถานะ LED ที่ simulator รายงาน | Read |

### Field configuration ที่แนะนำ

| Record | Field | ค่าที่เสนอ | ความหมาย |
|---|---|---|---|
| `ESP32:TEMP` | `EGU` | `degC` | หน่วยอุณหภูมิ |
| `ESP32:TEMP` | `PREC` | `1` | จำนวนตำแหน่งทศนิยมที่แสดงผล |
| `ESP32:HUMIDITY` | `EGU` | `%` | หน่วยความชื้นสัมพัทธ์ |
| `ESP32:HUMIDITY` | `PREC` | `1` | จำนวนตำแหน่งทศนิยมที่แสดงผล |
| `ESP32:LED` | `ZNAM` / `ONAM` | `OFF` / `ON` | Label ของคำสั่ง |
| `ESP32:LED:RBV` | `ZNAM` / `ONAM` | `OFF` / `ON` | Label ของ readback |

**หมายเหตุด้านการออกแบบ:**

- ค่าข้างต้นเป็นเพียง **ตัวอย่าง** ไม่ใช่ค่าที่อ่านได้แบบ live
- `ai`/`bi` มีไว้ใช้เป็น Record สำหรับ sensor/readback โดยต้อง implement แหล่งข้อมูลจำลองและกลไกการประมวลผลเพิ่มเติม เพราะการสร้าง Record เพียงอย่างเดียวไม่ทำให้ข้อมูลจาก sensor เปลี่ยนแปลงเอง
- `bo` แทน output ที่ต้องการสั่ง โดย simulator ต้องส่งต่อคำสั่งที่สำเร็จไปยัง reported state อย่างชัดเจน **ห้ามสมมติ** ว่า `ESP32:LED:RBV` จะเปลี่ยนตาม `ESP32:LED` โดยอัตโนมัติ
- สำหรับ Lab แรก ให้ใช้กลไก simulation ที่ชัดเจน เช่น simulation fields/links ที่เหมาะสม หรือ external updater แทนการสมมติว่าสามารถใช้ `caput` เขียนค่าไปยัง input record ได้อย่างอิสระ
- สถานะ GPIO ที่รายงานกลับมาไม่ได้ยืนยันด้วยตัวเองว่า LED จริงเปล่งแสงแล้ว

## 4. ตัวอย่าง Data Flow

### A. การ Monitor อุณหภูมิ

```text
Virtual temperature source
  → การประมวลผล IOC Record อัปเดต ESP32:TEMP.VAL
  → Channel Access ให้บริการ ESP32:TEMP
  → caget อ่านค่าล่าสุดที่ให้บริการอยู่
  → camonitor รับการอัปเดตเมื่อค่ามีการเปลี่ยนแปลงและถูก post
```

การอ่าน PV จาก client **ไม่จำเป็นต้อง** trigger การอ่านค่าจาก hardware หรือการ sample ใหม่ ค่าที่ได้รับอาจเป็นค่าจากการอัปเดต Record ครั้งล่าสุดที่เสร็จสมบูรณ์แล้ว

### B. การควบคุมและ Readback ของ LED

```text
caput ESP32:LED 1
  → เขียนค่าผ่าน Channel Access
  → IOC ประมวลผล ESP32:LED (bo)
  → simulated output handler รับคำสั่ง
  → simulator เปลี่ยนสถานะ LED
  → simulator อัปเดต ESP32:LED:RBV (bi)
  → caget ESP32:LED:RBV คืนค่า reported state
```

Command และ readback เป็นคนละแนวคิดกัน ค่าทั้งสองอาจไม่ตรงกันหากอุปกรณ์ยังไม่ทำงานตามคำสั่ง การสื่อสารล้มเหลว หรือ simulator ตั้งใจจำลอง delay/failure

## 5. Learning Labs

| Lab | หัวข้อ | ผลลัพธ์ที่คาดหวัง |
|---|---|---|
| 1 | การเตรียม Linux VM | ยืนยัน distro, architecture และการเข้าถึง terminal |
| 2 | การติดตั้ง EPICS Base | ตรวจสอบว่าใช้งาน `softIoc`, `caget`, `caput`, `camonitor` ได้ |
| 3 | IOC database แรก | เริ่ม Soft IOC และเปิดให้ใช้งาน Record ทั้ง 4 ตัวตามชื่อที่กำหนด |
| 4 | Channel Access | อ่าน เขียน และ monitor PV ที่เหมาะสมจาก terminal อีกหน้าต่างหนึ่ง |
| 5 | พฤติกรรม Simulation | ทำให้อุณหภูมิ/ความชื้นเปลี่ยนแปลง และ implement command/readback ของ LED |
| 6 | การประมวลผล Record | สังเกต processing timing, timestamps, alarms และค่าที่ stale |
| 7 (ไม่บังคับ) | External client/network | เข้าถึง IOC จาก VM/host เครื่องอื่นหลังตั้งค่า network แล้ว |
| 8 (อนาคต) | การเชื่อมต่อ ESP32 จริง | เปลี่ยนแหล่งข้อมูลจำลองเป็น serial/network bridge |

แต่ละ Lab ควรดำเนินตามลำดับ **แนวคิด → สมมติฐาน → คำสั่ง → การสังเกต → คำอธิบาย → การตรวจสอบความเข้าใจ** และไม่ควรถือว่า Lab เสร็จสมบูรณ์จนกว่าจะตรวจสอบผลลัพธ์ตามที่คาดหวังแล้ว

## 6. โครงสร้างโปรเจกต์ที่เสนอ

```text
mini-epics-control/
├── README.md
├── ioc/
│   ├── db/
│   │   └── esp32-sim.db         # นิยาม EPICS Records
│   └── st.cmd                   # IOC startup script (จะเขียนในภายหลัง)
├── simulator/
│   └── README.md                # เลือกแนวทาง Simulation ใน Lab 5
└── docs/
    ├── architecture.md
    └── lab-notes.md
```

นี่เป็นเพียงโครงสร้าง folder ที่ **เสนอไว้** ไม่ได้หมายความว่า file เหล่านี้มีอยู่แล้ว จะกำหนด path ของ startup script และแนวทาง implement simulation ให้เรียบร้อยหลังจากตรวจสอบ version ของ EPICS Base ที่ติดตั้งอยู่

## 7. Acceptance Criteria เบื้องต้น

- [ ] Linux VM เริ่มทำงาน และมี EPICS Base tools พร้อมใช้งาน
- [ ] Soft IOC เริ่มทำงานโดยไม่มี error จากการโหลด database
- [ ] `caget ESP32:TEMP` คืนค่าเป็นตัวเลข
- [ ] `caget ESP32:HUMIDITY` คืนค่าเป็นตัวเลข
- [ ] `caput ESP32:LED 1` ส่งคำสั่งที่ simulated output ยอมรับ
- [ ] `caget ESP32:LED:RBV` รายงานสถานะของ simulator หลังประมวลผลคำสั่งแล้ว
- [ ] `camonitor` ตรวจพบการอัปเดตค่าจำลองอย่างน้อยหนึ่งครั้ง
- [ ] อธิบายได้ว่าการอ่าน PV ต่างจากการประมวลผล Record อย่างไร
- [ ] อธิบายได้ว่าทำไม LED command และ LED readback จึงแยกเป็นคนละ PV

## 8. Architecture สำหรับ Hardware ในอนาคต

```text
EPICS Client
    ⇅ Channel Access
EPICS IOC (Linux VM / Linux PC)
    ⇅ Device support หรือ custom bridge
ESP32 Firmware
    ├── DHT22 temperature / humidity sensor
    └── GPIO → LED + resistor
```

Transport ที่เป็นไปได้ ได้แก่ USB Serial (แนะนำสำหรับการเชื่อมต่อ hardware จริงครั้งแรก), TCP, HTTP หรือ MQTT โดยแต่ละแบบต้องมี bridge/device-support implementation ที่สอดคล้องกัน **EPICS ไม่สามารถสื่อสารด้วย protocol ของ ESP32 firmware ได้โดยอัตโนมัติ**

## 9. ประเด็นที่ต้องตัดสินใจก่อนเริ่ม Implementation

1. Linux distribution/version ใดติดตั้งอยู่ใน VirtualBox?
2. จะติดตั้ง EPICS Base จาก source หรือใช้ package/distribution ที่เหมาะสม?
3. สำหรับ IOC แรก จะใช้ database แบบ static อย่างง่ายก่อน implement simulation logic หรือไม่?
4. จะใช้กลไก simulation แบบใดเพื่อให้ค่า sensor เป็น read-only และรองรับ LED readback?
5. ควรเปิดให้ host ภายนอกเข้าถึงได้หลังจากทดสอบผ่าน localhost แล้วเท่านั้นหรือไม่? (แนะนำ: ใช่)

## 10. เอกสารอ้างอิง

- [EPICS Introduction](https://docs.epics-controls.org/en/latest/getting-started/EPICS_Intro.html)
- [EPICS Documentation](https://docs.epics-controls.org/en/latest/)

---

**สถานะปัจจุบัน:** อยู่ในขั้นวางแผนเท่านั้น ขั้นตอนถัดไปคือการตรวจสอบ Environment ของ Linux VM แล้วจึงเริ่ม Lab 1 โดยยังไม่สมมติว่ามี hardware หรือ IOC ที่กำลังทำงานอยู่
