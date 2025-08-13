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
const long baseGmtOffset_sec = 2 * 3600; // GMT+2 для України

// ==== Обчислення літнього часу ====
int getDaylightOffset(const struct tm *timeinfo)
{
  int month = timeinfo->tm_mon + 1;
  int wday = timeinfo->tm_wday; // 0=Нд, 1=Пн...
  int mday = timeinfo->tm_mday;

  if (month < 3 || month > 10)
    return 0; // Січень-Лютий, Листопад-Грудень — зимовий
  if (month > 3 && month < 10)
    return 3600; // Квітень-Вересень — літній

  // Березень — перевіряємо останню неділю
  if (month == 3)
  {
    int lastSunday = 31 - ((wday + 31 - mday) % 7);
    return (mday >= lastSunday) ? 3600 : 0;
  }
  // Жовтень — перевіряємо останню неділю
  if (month == 10)
  {
    int lastSunday = 31 - ((wday + 31 - mday) % 7);
    return (mday < lastSunday) ? 3600 : 0;
  }
  return 0;
}

// ==== Підключення до WiFi ====
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
  // Отримуємо UTC
  configTime(0, 0, ntpServer);

  Serial.println("⌛ Отримання часу з NTP...");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("❌ Помилка отримання часу");
    return;
  }

  // Розраховуємо літній час
  int daylightOffset_sec = getDaylightOffset(&timeinfo);

  // Повторне налаштування часу з урахуванням зсувів
  configTime(baseGmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);

  Serial.println("✅ Час отримано з інтернету");
  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d (DST=%d)\n",
                timeinfo.tm_mday,
                timeinfo.tm_mon + 1,
                timeinfo.tm_year + 1900,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec,
                daylightOffset_sec ? 1 : 0);

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
