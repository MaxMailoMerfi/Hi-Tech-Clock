#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <time.h>
#include <FastLED.h>

// ==== Налаштування WiFi ====
char ssid[] = "deti_podzemelia";
char password[] = "12345678";

// ==== Піни DS1302 ====
#define PIN_CLK 14 // D5
#define PIN_DAT 12 // D6
#define PIN_RST 13 // D7

ThreeWire myWire(PIN_DAT, PIN_CLK, PIN_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

// ==== NTP ====
const char *ntpServer = "pool.ntp.org";
const long baseGmtOffset_sec = 2 * 3600; // GMT+2

// ==== Інтервали ====
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 1000;

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;

// ==== LED матриця ====
#define LED_PIN 2    // GPIO2 (D4)
#define NUM_LEDS 256 // 16x16
#define BRIGHTNESS 40
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

int getDaylightOffset(const struct tm *timeinfo)
{
  int month = timeinfo->tm_mon + 1;
  int wday = timeinfo->tm_wday;
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

void connectWiFi()
{
  Serial.print("Підключення до WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000)
  {
    Serial.print(".");
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED)
    Serial.println("\n✅ Підключено до WiFi");
  else
    Serial.println("\n❌ Не вдалося підключитися");
}

void syncTimeFromNTP()
{
  configTime(0, 0, ntpServer);
  struct tm timeinfo;

  Serial.println("⌛ Отримання часу з NTP...");
  if (!getLocalTime(&timeinfo, 5000))
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

// ==== Простий вивід часу на матрицю (HH:MM) ====
void showTimeOnMatrix(int hour, int minute)
{
  FastLED.clear();

  // Тут можна намалювати свої цифри (приклад спрощений — кожна цифра як блок)
  // Ліва цифра годин — червона
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 3; x++)
      leds[y * 16 + x] = CRGB::Red;

  // Права цифра годин — червона
  for (int y = 0; y < 8; y++)
    for (int x = 4; x < 7; x++)
      leds[y * 16 + x] = CRGB::Red;

  // Дві точки — зелені
  leds[3 * 16 + 8] = CRGB::Green;
  leds[5 * 16 + 8] = CRGB::Green;

  // Ліва цифра хвилин — синя
  for (int y = 0; y < 8; y++)
    for (int x = 9; x < 12; x++)
      leds[y * 16 + x] = CRGB::Blue;

  // Права цифра хвилин — синя
  for (int y = 0; y < 8; y++)
    for (int x = 13; x < 16; x++)
      leds[y * 16 + x] = CRGB::Blue;

  FastLED.show();
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  // LED матриця
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);

  connectWiFi();
  delay(1000);
  syncTimeFromNTP();
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastWiFiCheck >= wifiCheckInterval)
  {
    lastWiFiCheck = currentMillis;
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("⚠ WiFi розірвано");
    }
  }

  if (currentMillis - lastPrintTime >= printInterval)
  {
    lastPrintTime = currentMillis;
    RtcDateTime now = Rtc.GetDateTime();
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%02u:%02u:%02u"),
               now.Hour(), now.Minute(), now.Second());
    Serial.println(buffer);

    showTimeOnMatrix(now.Hour(), now.Minute());
  }
}
