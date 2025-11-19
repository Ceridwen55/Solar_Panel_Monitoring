/*
  ---- DESKRIPSI SINGKAT PROYEK DEMO MONITORING PANEL SURYA ----

  Komponen kunci = 
  -ARDUINO UNO (Simpel untuk prototyping cepat)
  -INA219 (Bagus dan murah)
  -DS3231 (RTC kecil dan murah)
  -I2C LCD LM016L 16X2 (Murah dan cocok untuk prototyping)
  -microSD GS (SPI)
  -Kapasitor dan resistor untuk decoupling sama pullup/pulldown jika dibutuhkan

  Sambungan (COM Protocol) = 

  - I2C (ARDUINO UNO): SDA=A4, SCL=A5
  - SPI (UNO): CS=10, SCK=13, MISO=12, MOSI=11

  Catatan:
   - Pakai I2C LCD (LM016L). Standar address: 0x27 
   - INA219 standard I2C address: 0x40 (Adafruit/3-5V).
   - DS3231 I2C address: 0x68 (5V or 3.3V).
   - microSD module harus pakai LDO 3.3 V

*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

// --- Pins (ARDUINO UNO) ---
const uint8_t SD_CS = 10;       // CS10 untuk MicroSD

// --- Peripherals ---
Adafruit_INA219 ina219;                 // I2C 0x40
LiquidCrystal_I2C lcd(0x27, 16, 2);     // I2C alamat 0x27, ganti ke 0x3F  misal beda 
RTC_DS3231 rtc; //keterangan terkait RTC

// --- State ---
bool sdBeres = false; //cek SD Card
bool rtcBeres = false; //cek RTC
File logFile;

unsigned long lastSampleMs = 0; 
const unsigned long SAMPLE_MS = 1000;

float vEMA = NAN; //moving average untuk voltase
float iEMA = NAN; //moving average untuk arus
float pEMA = NAN; //moving average untuk power/watt

const float konstanta = 0.25f;
double energy_Wh = 0.0;

//--- Helpers (UNO-friendly, ga ada String heap) ---
void duaDesimal(char* out, uint8_t v) //dapat lebih detail angka voltase
{
  out[0] = '0' + (v/10);
  out[1] = '0' + (v%10);
  out[2] = 0;
}

void getTimestamp(char* buf, size_t len) //untuk dapat waktu di RTC
{
  if (rtcBeres){
    DateTime t = rtc.now();
    // YYYY-MM-DD HH:MM:SS
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d",
             t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
  }
  else
  {
    unsigned long s = millis()/1000;
    uint8_t hh = (s/3600UL)%24;
    uint8_t mm = (s/60UL)%60;
    uint8_t ss = s%60;
    snprintf(buf, len, "0000-00-00 %02u:%02u:%02u", hh, mm, ss);
  }
}

void openCSV() //start untuk buka file CSV buat dimasukin ke microSD
{
  if (!SD.exists("/panelsurya_log.csv"))
  {
    logFile = SD.open("/panelsurya_log.csv", FILE_WRITE);
    if (logFile){
      logFile.println(F("timestamp,voltage_V,current_A,power_W,energy_Wh"));
      logFile.flush();
    }
  } 
  else 
  {
    logFile = SD.open("/panelsurya_log.csv", FILE_WRITE);
  }

  if (logFile) 
  {
      logFile.seek(logFile.size()); //UNTUK APPEND
  }
  if (!logFile) sdBeres = false;
}

void appendCSV(const char* ts, float v, float i, float p, double eWh) //Nambah baris data
{
  if (!sdBeres || !logFile) return;
  char vbuffer[12], ibuffer[12], pbuffer[12], ebuffer[16];
  dtostrf(v, 0, 3, vbuffer);
  dtostrf(i, 0, 3, ibuffer);
  dtostrf(p, 0, 3, pbuffer);
  dtostrf(eWh, 0, 4, ebuffer);

  logFile.print(ts); logFile.print(',');
  logFile.print(vbuffer); logFile.print(',');
  logFile.print(ibuffer); logFile.print(',');
  logFile.print(pbuffer); logFile.print(',');
  logFile.println(ebuffer);

  static uint8_t flushKontrol=0;
  if ((++flushKontrol % 5) == 0) logFile.flush(); //Buat nge-flush biar awet kartunya per 5x aksi
}

void lcdSplash(const char* line) //Splash diawal screen
{
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("Monitor PS"));
  lcd.setCursor(0,1); lcd.print(line);
  
}

// --- Setup ---
void setup()
{
  // Serial buat debug ( kalau butuh terminal)
  Serial.begin(115200);
  delay(100);

  // I2C
  Wire.begin(); // UNO: pakai pin A4(SDA), A5(SCL)

  // LCD
  lcd.init();
  lcd.backlight();
  lcdSplash("Good Day :)");

  // Sensor PVI INA219
  if (!ina219.begin()){
    Serial.println(F("ERROR INA219"));
    lcdSplash("INA219 FAIL");
  } 
  else 
  {
    ina219.setCalibration_32V_2A();
  }
  

  // SD
  if (!SD.begin(SD_CS))
  {
    Serial.println(F("ERROR SD CONNECTOR"));
    sdBeres = false;
    lcdSplash("Cek koneksi SD nya");
  } 
  else 
  {
    sdBeres = true;
    openCSV(); //buka fungsi open CSV nya
  }

  // RTC
  if (!rtc.begin()){
    rtcBeres = false;
    Serial.println(F("CEK LAGI RTC"));
    lcdSplash("RTC WARN");
  } 
  else 
  {
    rtcBeres = true;
    if (rtc.lostPower())
    {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //biar waktu stay in-line jika mati RTC
    }
  }

  delay(800);
  lcd.clear();
}

// --- Super Loop ---
void loop()
{
  unsigned long now = millis(); //waktu relatif pas arduino pertama kali dinyalakan

  if (now - lastSampleMs >= SAMPLE_MS)
  {
    lastSampleMs = now;

    // Baca INA219
    float v = ina219.getBusVoltage_V();
    float i = ina219.getCurrent_mA() / 1000.0f;
    float p = v * i;

    // Ngehalusin Moving Average (EMA)
    if (isnan(vEMA))
    { 
      vEMA=v; 
      iEMA=i; 
      pEMA=p; 
    }

    else 
    {

      vEMA =v; iEMA = i; ; pEMA = p;
      
      //Intinya menggunakan konstanta (0.25) untuk mencari moving average 
      vEMA = (1-konstanta)*vEMA + konstanta * v; 
      iEMA = (1-konstanta)*iEMA + konstanta * i;
      pEMA = (1-konstanta)*pEMA + konstanta * p;
      
    }

    // Energi Wh (watthour)
    energy_Wh += (double)pEMA * ((double)SAMPLE_MS/1000.0) / 3600.0;

    // Timestamp
    char slotwaktu[24];
    getTimestamp(slotwaktu, sizeof(slotwaktu));

    // LCD update
    lcd.setCursor(0,0); lcd.print(F("V:")); lcd.print(vEMA,2);lcd.print(F(",I:")); lcd.print(iEMA,2);
    lcd.setCursor(0,1); lcd.print(F("P:")); lcd.print(pEMA,1); lcd.print(F(",E:")); lcd.print(energy_Wh,3);
    delay(1000);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(slotwaktu + 11); // HH:MM:SS
    delay(500);


    // Serial debug
    Serial.println(slotwaktu); Serial.print(F(" V=")); Serial.println(v,3);
    Serial.print(F(" I=")); Serial.println(i,3);
    Serial.print(F(" P=")); Serial.println(p,3);
    Serial.print(F(" E=")); Serial.println(energy_Wh,4);
    Serial.println(F(" Wh"));

    // Log
    if (sdBeres)
    { 

    appendCSV(slotwaktu, vEMA, iEMA, pEMA, energy_Wh);
    
    }
  }
}
