#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "utilities.h" // Для определения пинов I2C

// Параметры дисплея
#define SCREEN_WIDTH 128  // Ширина OLED дисплея в пикселях
#define SCREEN_HEIGHT 32  // Высота OLED дисплея в пикселях
#define OLED_RESET -1     // Пин сброса (-1 если используется общий пин сброса Arduino)
#define SCREEN_ADDRESS 0x3C // I2C адрес SSD1306 дисплея (обычно 0x3C или 0x3D)

// Класс для управления OLED дисплеем
class OLEDDisplay {
private:
    Adafruit_SSD1306 display;
    bool initialized;
    
    // Структура для хранения данных экрана
    struct DisplayData {
        String line1;
        String line2;
        String line3;
        String line4;
        String status;
        float batteryVoltage;
        int signalStrength;
        bool gpsFixed;
        bool networkConnected;
        String ip;
        String time;
    } displayData;
    
public:
    OLEDDisplay();
    
    // Инициализация дисплея
    bool begin();
    
    // Основные функции отображения
    void clear();
    void update();
    void setTextSize(uint8_t size);
    void setTextColor(uint16_t color);
    void setCursor(int16_t x, int16_t y);
    void print(const String& text);
    void println(const String& text);
    
    // Готовые экраны
    void showBootScreen();
    void showMainScreen();
    void showNetworkInfo();
    void showGPSInfo();
    void showBatteryInfo();
    void showStatusScreen();
    
    // Обновление данных
    void updateBatteryVoltage(float voltage);
    void updateSignalStrength(int strength);
    void updateGPSStatus(bool fixed);
    void updateNetworkStatus(bool connected);
    void updateIP(const String& ip);
    void updateTime(const String& time);
    void updateStatusLine(const String& status);
    
    // Графические элементы
    void drawProgressBar(int x, int y, int width, int height, int progress);
    void drawSignalBars(int x, int y, int strength);
    void drawBatteryIcon(int x, int y, float voltage);
    void drawGPSIcon(int x, int y, bool fixed);
    void drawNetworkIcon(int x, int y, bool connected);
    
    // Утилиты
    void showMessage(const String& message, int duration = 2000);
    void showError(const String& error);
    void showSuccess(const String& message);
    
    // Проверка инициализации
    bool isInitialized() const { return initialized; }
};

extern OLEDDisplay oledDisplay;

#endif // OLED_DISPLAY_H
