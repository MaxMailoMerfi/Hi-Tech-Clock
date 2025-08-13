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

// ==== Часові інтервали ====
unsigned long lastPrintTime = 0;          // для виводу в Serial
const unsigned long printInterval = 1000; // раз на секунду

unsigned long lastWiFiCheck = 0;               // для перевірки WiFi
const unsigned long wifiCheckInterval = 10000; // кожні 10 секунд

// ==== Обчислення літнього часу ====
int getDaylightOffset(const struct tm *timeinfo)
{
  int month = timeinfo->tm_mon + 1;
  int wday = timeinfo->tm_wday; // 0=Нд, 1=Пн...
  int mday = timeinfo->tm_mday;

  if (month < 3 || month > 10)
    return 0;
  if (month > 3 && month < 10)
    return 3600;

  if (month == 3)
  {
    int lastSunday = 31 - ((wday + 31 - mday) % 7);
    return (mday >= lastSunday) ? 3600 : 0;
  }
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

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) // максимум 10 сек
  {
    Serial.print(".");
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED)
    Serial.println("\n✅ Підключено до WiFi");
  else
    Serial.println("\n❌ Не вдалося підключитися");
}

// ==== Отримання часу з NTP і запис в DS1302 ====
void syncTimeFromNTP()
{
  configTime(0, 0, ntpServer);
  struct tm timeinfo;

  Serial.println("⌛ Отримання часу з NTP...");
  if (!getLocalTime(&timeinfo, 5000)) // таймаут 5 сек
  {
    Serial.println("❌ Помилка отримання часу");
    return;
  }

  int daylightOffset_sec = getDaylightOffset(&timeinfo);

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
  delay(1000); // невелика затримка перед NTP
  syncTimeFromNTP();
}

void loop()
{
  unsigned long currentMillis = millis();

  // Перевірка WiFi раз на 10 секунд
  if (currentMillis - lastWiFiCheck >= wifiCheckInterval)
  {
    lastWiFiCheck = currentMillis;
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("⚠ WiFi розірвано");
    }
  }

  // Вивід часу раз на 1 секунду
  if (currentMillis - lastPrintTime >= printInterval)
  {
    lastPrintTime = currentMillis;
    RtcDateTime now = Rtc.GetDateTime();
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
               now.Day(), now.Month(), now.Year(),
               now.Hour(), now.Minute(), now.Second());
    Serial.println(buffer);
  }
}