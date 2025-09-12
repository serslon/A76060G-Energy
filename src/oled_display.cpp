#include "oled_display.h"

// Глобальный объект дисплея
OLEDDisplay oledDisplay;

OLEDDisplay::OLEDDisplay() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), initialized(false) {
    // Инициализация структуры данных
    displayData.line1 = "";
    displayData.line2 = "";
    displayData.line3 = "";
    displayData.line4 = "";
    displayData.status = "Initializing...";
    displayData.batteryVoltage = 0.0;
    displayData.signalStrength = 0;
    displayData.gpsFixed = false;
    displayData.networkConnected = false;
    displayData.ip = "0.0.0.0";
    displayData.time = "--:--";
}

bool OLEDDisplay::begin() {
    Serial.println("Initializing OLED display...");
    
    // Инициализация I2C (если не была инициализирована)
    Wire.begin();
    
    // Инициализация дисплея
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        initialized = false;
        return false;
    }
    
    Serial.println("OLED display initialized successfully");
    initialized = true;
    
    // Очистка дисплея
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Показать экран загрузки
    showBootScreen();
    
    return true;
}

void OLEDDisplay::clear() {
    if (!initialized) return;
    display.clearDisplay();
}

void OLEDDisplay::update() {
    if (!initialized) return;
    display.display();
}

void OLEDDisplay::setTextSize(uint8_t size) {
    if (!initialized) return;
    display.setTextSize(size);
}

void OLEDDisplay::setTextColor(uint16_t color) {
    if (!initialized) return;
    display.setTextColor(color);
}

void OLEDDisplay::setCursor(int16_t x, int16_t y) {
    if (!initialized) return;
    display.setCursor(x, y);
}

void OLEDDisplay::print(const String& text) {
    if (!initialized) return;
    display.print(text);
}

void OLEDDisplay::println(const String& text) {
    if (!initialized) return;
    display.println(text);
}

void OLEDDisplay::showBootScreen() {
    if (!initialized) return;
    
    clear();
    setTextSize(1);
    setCursor(20, 5);
    print("LilyGO");
    
    setCursor(15, 15);
    print("A76060G-Energy");
    
    setCursor(40, 25);
    print("Starting...");
    
    update();
    delay(2000);
}

void OLEDDisplay::showMainScreen() {
    if (!initialized) return;
    
    clear();
    
    // Верхняя строка со статусом и иконками
    setTextSize(1);
    setCursor(0, 0);
    print("Bat:" + String(displayData.batteryVoltage, 1) + "V");
    
    setCursor(50, 0);
    print("Sig:" + String(displayData.signalStrength) + "%");
    
    // Иконки статуса
    drawBatteryIcon(110, 0, displayData.batteryVoltage);
    drawGPSIcon(100, 0, displayData.gpsFixed);
    drawNetworkIcon(90, 0, displayData.networkConnected);
    
    // Вторая строка - GPS и сеть
    setCursor(0, 10);
    print("GPS:" + String(displayData.gpsFixed ? "OK" : "NO"));
    
    setCursor(50, 10);
    print("Net:" + String(displayData.networkConnected ? "OK" : "NO"));
    
    // Третья строка - время и IP
    setCursor(0, 20);
    print("Time: " + displayData.time);
    
    setCursor(60, 20);
    print(displayData.ip.substring(displayData.ip.lastIndexOf('.') + 1)); // Показываем только последний октет IP
    
    update();
}

void OLEDDisplay::showNetworkInfo() {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    setCursor(0, 0);
    print("Network Info");
    
    setCursor(0, 10);
    print("Status: " + String(displayData.networkConnected ? "OK" : "NO"));
    
    setCursor(0, 20);
    print("IP: " + displayData.ip);
    
    drawProgressBar(70, 10, 50, 6, displayData.signalStrength);
    
    update();
}

void OLEDDisplay::showGPSInfo() {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    setCursor(0, 0);
    print("GPS Status");
    
    setCursor(0, 10);
    print("Status: " + String(displayData.gpsFixed ? "Fixed" : "Search"));
    
    if (displayData.gpsFixed) {
        drawGPSIcon(100, 10, true);
        setCursor(0, 20);
        print("Satellites: OK");
    } else {
        setCursor(0, 20);
        print("Searching...");
    }
    
    update();
}

void OLEDDisplay::showBatteryInfo() {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    setCursor(0, 0);
    print("Battery Status");
    
    setCursor(0, 10);
    print("Voltage: " + String(displayData.batteryVoltage, 2) + "V");
    
    // Вычисление процента заряда (примерно)
    float percentage = ((displayData.batteryVoltage - 3.0) / (4.2 - 3.0)) * 100.0;
    percentage = constrain(percentage, 0, 100);
    
    setCursor(0, 20);
    print("Charge: " + String((int)percentage) + "%");
    
    drawProgressBar(60, 20, 60, 8, (int)percentage);
    drawBatteryIcon(100, 10, displayData.batteryVoltage);
    
    update();
}

void OLEDDisplay::showStatusScreen() {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    setCursor(0, 0);
    print("System Status");
    
    setCursor(0, 10);
    print(displayData.status);
    
    setCursor(0, 20);
    print("Uptime: " + displayData.time);
    
    update();
}

void OLEDDisplay::updateBatteryVoltage(float voltage) {
    displayData.batteryVoltage = voltage;
}

void OLEDDisplay::updateSignalStrength(int strength) {
    displayData.signalStrength = constrain(strength, 0, 100);
}

void OLEDDisplay::updateGPSStatus(bool fixed) {
    displayData.gpsFixed = fixed;
}

void OLEDDisplay::updateNetworkStatus(bool connected) {
    displayData.networkConnected = connected;
}

void OLEDDisplay::updateIP(const String& ip) {
    displayData.ip = ip;
}

void OLEDDisplay::updateTime(const String& time) {
    displayData.time = time;
}

void OLEDDisplay::updateStatusLine(const String& status) {
    displayData.status = status;
}

void OLEDDisplay::drawProgressBar(int x, int y, int width, int height, int progress) {
    if (!initialized) return;
    
    progress = constrain(progress, 0, 100);
    
    // Рамка
    display.drawRect(x, y, width, height, SSD1306_WHITE);
    
    // Заполнение
    int fillWidth = (width - 2) * progress / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE);
    }
}

void OLEDDisplay::drawSignalBars(int x, int y, int strength) {
    if (!initialized) return;
    
    strength = constrain(strength, 0, 100);
    int bars = strength / 25; // 0-4 полоски
    
    for (int i = 0; i < 4; i++) {
        int barHeight = (i + 1) * 2;
        if (i < bars) {
            display.fillRect(x + i * 3, y + 8 - barHeight, 2, barHeight, SSD1306_WHITE);
        } else {
            display.drawRect(x + i * 3, y + 8 - barHeight, 2, barHeight, SSD1306_WHITE);
        }
    }
}

void OLEDDisplay::drawBatteryIcon(int x, int y, float voltage) {
    if (!initialized) return;
    
    // Корпус батареи
    display.drawRect(x, y + 2, 12, 6, SSD1306_WHITE);
    
    // Плюсовой контакт
    display.fillRect(x + 12, y + 3, 2, 4, SSD1306_WHITE);
    
    // Уровень заряда
    float percentage = ((voltage - 3.0) / (4.2 - 3.0)) * 100.0;
    percentage = constrain(percentage, 0, 100);
    
    int fillWidth = 10 * percentage / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 1, y + 3, fillWidth, 4, SSD1306_WHITE);
    }
}

void OLEDDisplay::drawGPSIcon(int x, int y, bool fixed) {
    if (!initialized) return;
    
    if (fixed) {
        // GPS иконка (активная)
        display.fillCircle(x + 4, y + 4, 3, SSD1306_WHITE);
        display.drawCircle(x + 4, y + 4, 6, SSD1306_WHITE);
    } else {
        // GPS иконка (неактивная)
        display.drawCircle(x + 4, y + 4, 3, SSD1306_WHITE);
        display.drawCircle(x + 4, y + 4, 6, SSD1306_WHITE);
    }
}

void OLEDDisplay::drawNetworkIcon(int x, int y, bool connected) {
    if (!initialized) return;
    
    if (connected) {
        // Сеть подключена - закрашенный треугольник
        display.fillTriangle(x, y + 8, x + 8, y, x + 8, y + 8, SSD1306_WHITE);
    } else {
        // Сеть не подключена - контур треугольника
        display.drawTriangle(x, y + 8, x + 8, y, x + 8, y + 8, SSD1306_WHITE);
    }
}

void OLEDDisplay::showMessage(const String& message, int duration) {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    
    // Вычисление позиции для центрирования
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(message, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    setCursor((SCREEN_WIDTH - textWidth) / 2, (SCREEN_HEIGHT - textHeight) / 2);
    print(message);
    
    update();
    
    if (duration > 0) {
        delay(duration);
    }
}

void OLEDDisplay::showError(const String& error) {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    setCursor(0, 0);
    print("ERROR:");
    
    setCursor(0, 15);
    setTextSize(1);
    
    // Разбиваем длинное сообщение на строки
    String msg = error;
    int lineHeight = 10;
    int currentY = 15;
    
    while (msg.length() > 0 && currentY < SCREEN_HEIGHT - lineHeight) {
        String line = msg.substring(0, 21); // Примерно 21 символ на строку
        if (msg.length() > 21) {
            int lastSpace = line.lastIndexOf(' ');
            if (lastSpace > 0) {
                line = line.substring(0, lastSpace);
                msg = msg.substring(lastSpace + 1);
            } else {
                msg = msg.substring(21);
            }
        } else {
            msg = "";
        }
        
        setCursor(0, currentY);
        print(line);
        currentY += lineHeight;
    }
    
    update();
}

void OLEDDisplay::showSuccess(const String& message) {
    if (!initialized) return;
    
    clear();
    
    setTextSize(1);
    setCursor(0, 0);
    print("SUCCESS:");
    
    setCursor(0, 15);
    print(message);
    
    // Галочка
    display.drawLine(100, 15, 105, 20, SSD1306_WHITE);
    display.drawLine(105, 20, 115, 10, SSD1306_WHITE);
    
    update();
    delay(2000);
}
