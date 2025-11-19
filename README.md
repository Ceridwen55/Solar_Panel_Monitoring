**Solar Panel Monitoring Demo (Arduino UNO + INA219 + DS3231 + microSD + I2C LCD)**

This project is a simple solar panel monitoring system based on Arduino UNO, designed to measure voltage (V), current (A), power (W), and energy (Wh) in real time. Data is displayed on a 16×2 I2C LCD and logged into a microSD card in CSV format for later analysis (Excel, Python, etc.).

**📦 Features**

Real-time monitoring of:

Voltage (V)

Current (A)

Power (W)

Energy accumulation (Wh)

Accurate timestamp using DS3231 RTC

Automatic data logging to microSD (CSV)

Live display on I2C LCD 16×2

Data smoothing using Exponential Moving Average (EMA)

Safe SD card handling with periodic flush every 5 writes

**🛠️ Hardware Components**
Component	Function
Arduino UNO	Main MCU for reading sensors & storing data
INA219	Measures voltage & current from the solar panel
DS3231 RTC	Provides real-time clock timestamp
LCD 16×2 I2C (0x27/0x3F)	Displays live monitoring data
MicroSD module (SPI)	Saves CSV logs
Capacitors & resistors	Decoupling & pull-up/pull-down if needed

**🔌 Pin Connections**
I2C — Arduino UNO
Device	SDA	SCL	Address
LCD 16×2 I2C	A4	A5	0x27 (common)
INA219	A4	A5	0x40
DS3231	A4	A5	0x68
SPI — MicroSD
Signal	Pin
CS	10
MOSI	11
MISO	12
SCK	13

⚠️ Use a 3.3V-regulated microSD module to avoid damaging the SD card.

**🧪 How It Works**

INA219 reads voltage and current from the solar panel.

The Arduino computes:

Power (P = V × I)

Accumulated energy (Wh)

EMA smoothing is applied to stabilize noisy readings.

DS3231 RTC provides accurate timestamps.

Data is shown on the LCD.

Data is logged into a CSV file on the microSD card:

timestamp,voltage_V,current_A,power_W,energy_Wh
2025-11-20 13:25:18,12.50,0.32,4.00,0.0011


Logging is always appended, never overwrites existing data.

**📁 CSV Log Structure
**
Columns:

timestamp

voltage_V

current_A

power_W

energy_Wh

Example:

2025-11-20 14:02:33,12.487,0.113,1.407,0.0041
2025-11-20 14:02:34,12.489,0.115,1.436,0.0045

**▶️ How to Use**

Connect all components according to the wiring table.

Load the provided code into Arduino IDE.

Install required libraries:

Adafruit INA219

RTClib

LiquidCrystal_I2C

SD

Upload the sketch to Arduino UNO.

Insert a FAT32-formatted microSD card.

Power the system and observe live monitoring on LCD or Serial Monitor.


**📸 LCD Display Example**

Line 1:

V:12.5,I:0.12


Line 2:

P:1.5,E:0.004


It also periodically shows the current time:

14:32:11


**⚙️ Technical Notes**

EMA smoothing uses:

EMA = (1 - α)*old + α*new
α = 0.25


Energy (Wh) is computed from smoothed power (W).

CSV is flushed every 5 writes to extend SD card life.

If RTC lost power, it auto-syncs to compile-time.

**📝 License**

This project is free to use for learning, research, or personal applications.

**🤝 Contributions**

Feel free to open issues or pull requests for improvements, bug fixes, or new features due this project is only a learning tools for me. Thanks
