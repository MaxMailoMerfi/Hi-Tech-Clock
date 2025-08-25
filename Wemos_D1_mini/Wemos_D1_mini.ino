#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <time.h>
#include <FastLED.h>

// ==== WiFi ====
char ssid[] = "deti_podzemelia";
char password[] = "12345678";

// ==== DS1302 ====
#define PIN_CLK 14 // D5
#define PIN_DAT 12 // D6
#define PIN_RST 13 // D7

ThreeWire myWire(PIN_DAT, PIN_CLK, PIN_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

// ==== NTP ====
const char *ntpServer = "pool.ntp.org";
const long baseGmtOffset_sec = 2 * 3600;

// ==== Інтервали ====
// Інтервал виводу часу на серійний порт
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 1000;
// Інтервал перевірки WiFi статусу
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;
// ==== змінні для мигання ====
bool colonVisible = true;
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 1000;

// ==== LED матриця ====
#define LED_PIN 2
#define NUM_LEDS 256
#define BRIGHTNESS 20
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==== Масив символів 3x5 ====
const byte font3x5[10][5] = {
    {B111, B101, B101, B101, B111}, // 0
    {B010, B011, B010, B010, B111}, // 1
    {B111, B100, B111, B001, B111}, // 2
    {B111, B100, B111, B100, B111}, // 3
    {B101, B101, B111, B100, B100}, // 4
    {B111, B001, B111, B100, B111}, // 5
    {B111, B001, B111, B101, B111}, // 6
    {B111, B100, B100, B100, B100}, // 7
    {B111, B101, B111, B101, B111}, // 8
    {B111, B101, B111, B100, B111}  // 9
};

// Перетворення (X,Y) у індекс світлодіода
int XY(int x, int y)
{
  // Робимо поворот на 90° вправо:
  // нові координати (x', y') = (15 - y, x)
  int newX = y;
  int newY = 15 - x;

  if (newY % 2 == 0)
    return newY * 16 + newX;
  else
    return newY * 16 + (15 - newX);
}

// Малювання однієї цифри 3x5
void drawDigit3x5(int digit, int x0, int y0, CRGB color)
{
  for (int row = 0; row < 5; row++)
  {
    for (int col = 0; col < 3; col++)
    {
      if (font3x5[digit][row] & (1 << (2 - col)))
      {
        leds[XY(x0 + col, y0 + row)] = color;
      }
    }
  }
}

// Малювання двокрапки
void drawColon(int x0, int y0, CRGB color)
{
  leds[XY(x0, y0 + 1)] = color;
  leds[XY(x0, y0 + 3)] = color;
}

// Вивід часу HH:MM
void showTimeOnMatrix(int hour, int minute)
{
  FastLED.clear();

  drawDigit3x5(hour / 10, 13, 0, CRGB::Red); // десятки годин
  drawDigit3x5(hour % 10, 9, 0, CRGB::Red); // одиниці годин
 
  // двокрапка (мигає)
  if (colonVisible)
  {
    drawColon(8, 0, CRGB::Green);
    drawColon(7, 0, CRGB::Green);
  }
  drawDigit3x5(minute / 10, 4, 0, CRGB::Blue); // десятки хвилин
  drawDigit3x5(minute % 10, 0, 0, CRGB::Blue); // одиниці хвилин

  FastLED.show();
}

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
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000)
  {
    Serial.print(".");
    delay(500);
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
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);

  connectWiFi();
  delay(1000);
  while (WiFi.status() != WL_CONNECTED)
  {
    syncTimeFromNTP();
  }
}

void loop()
{
  unsigned long currentMillis = millis();
  // Перевірка WiFi статусу
  if (currentMillis - lastWiFiCheck >= wifiCheckInterval)
  {
    lastWiFiCheck = currentMillis;
    if (WiFi.status() != WL_CONNECTED)
      Serial.println("⚠ WiFi розірвано");
  }

  // мигання двокрапки
  if (currentMillis - lastBlink >= blinkInterval)
  {
    lastBlink = currentMillis;
    colonVisible = !colonVisible;
  }

  // Отримання часу з RTC
  if (currentMillis - lastPrintTime >= printInterval)
  {
    lastPrintTime = currentMillis;
    RtcDateTime now = Rtc.GetDateTime();
    Serial.printf("%02u:%02u:%02u\n", now.Hour(), now.Minute(), now.Second());
    showTimeOnMatrix(now.Hour(), now.Minute());
  }
}
