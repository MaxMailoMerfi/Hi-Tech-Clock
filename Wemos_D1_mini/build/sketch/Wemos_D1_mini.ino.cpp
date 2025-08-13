#line 1 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos_D1_mini\\Wemos_D1_mini.ino"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <time.h>

// ==== Налаштування WiFi (масиви char) ====
char ssid[] = "deti_podzemelia"; // логін WiFi
char password[] = "12345678";    // пароль WiFi

// ==== Піни DS1302 ====
#define PIN_CLK 14 // D5
#define PIN_DAT 12 // D6
#define PIN_RST 13 // D7

ThreeWire myWire(PIN_DAT, PIN_CLK, PIN_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

// ==== NTP налаштування ====
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 2 * 3600; // GMT+2
const int daylightOffset_sec = 1;    // літній час (0 якщо не потрібно)
             const long gmtOffset_sec = 3 * 3600; // GMT+2
const int daylightOffset_sec = 0;                 // літній час (0 якщо не потрібно)

// ==== Підключення до WiFi ====
#line 27 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos_D1_mini\\Wemos_D1_mini.ino"
void connectWiFi();
#line 42 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos_D1_mini\\Wemos_D1_mini.ino"
void syncTimeFromNTP();
#line 74 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos_D1_mini\\Wemos_D1_mini.ino"
void setup();
#line 88 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos_D1_mini\\Wemos_D1_mini.ino"
void loop();
#line 27 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos_D1_mini\\Wemos_D1_mini.ino"
void connectWiFi()
{
  Serial.print("Підключення до WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Підключено до WiFi");
}

// ==== Отримання часу з NTP і запис в DS1302 ====
void syncTimeFromNTP()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("⌛ Отримання часу з NTP...");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("❌ Помилка отримання часу");
    return;
  }

  Serial.println("✅ Час отримано з інтернету");
  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d\n",
                timeinfo.tm_mday,
                timeinfo.tm_mon + 1,
                timeinfo.tm_year + 1900,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);

  // Запис в DS1302
  RtcDateTime newTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec);
  Rtc.SetDateTime(newTime);
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);

  connectWiFi();
  delay(200);
  syncTimeFromNTP();
}

void loop()
{
  // Читання часу з DS1302
  RtcDateTime now = Rtc.GetDateTime();
  char buffer[20];
  snprintf_P(buffer, sizeof(buffer), PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             now.Day(), now.Month(), now.Year(),
             now.Hour(), now.Minute(), now.Second());

  Serial.println(buffer);

  delay(1000);
}
