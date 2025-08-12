#include <Arduino.h>
#line 1 "D:\\Max\\WEMOS\\Hi-Tech-Clock\\Wemos D1 mini\\Wemos D1 mini.ino"
// #include <Arduino.h>
#include <DS1302.h> // Бібліотека з https://www.rinkydinkelectronics.com/library.php?id=73

// Піни підключення DS1302
#define DS1302_CLK 14 // GPIO14 = D5 (SCK)
#define DS1302_DAT 12 // GPIO12 = D6 (MISO)
#define DS1302_RST 13 // GPIO13 = D7 (MOSI)

// Створюємо об'єкт годинника
DS1302 rtc(DS1302_RST, DS1302_DAT, DS1302_CLK);

void setup()
{
    Serial.begin(115200);

    // Ініціалізація DS1302
    rtc.halt(false);         // Запустити годинник (якщо був зупинений)
    rtc.writeProtect(false); // Вимкнути захист від запису

    // Один раз встановлюємо час (потім можна закоментувати)
    // Формат: рік, місяць, день, год, хв, сек, деньТижня (1=Пн ... 7=Нд)
    rtc.setDOW(TUESDAY);      // День тижня
    rtc.setTime(14, 30, 0);   // Години, хвилини, секунди
    rtc.setDate(12, 8, 2025); // День, місяць, рік

    Serial.println("DS1302 готовий!");
}

void loop()
{
    // Виводимо дату та час кожну секунду
    Serial.print(rtc.getDOWStr()); // День тижня
    Serial.print(" ");
    Serial.print(rtc.getDateStr()); // Дата
    Serial.print(" -- ");
    Serial.println(rtc.getTimeStr()); // Час

    delay(1000);
}

