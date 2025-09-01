/**
 * @file      battery_monitor.cpp
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     Battery monitoring module implementation
 */

#include "battery_monitor.h"

// Константы для Li-ion батареи
const float BatteryMonitor::MIN_VOLTAGE = 3.0;        // Минимальное напряжение
const float BatteryMonitor::MAX_VOLTAGE = 4.2;        // Максимальное напряжение
const int BatteryMonitor::ADC_RESOLUTION = 4095;      // 12-bit АЦП
const float BatteryMonitor::REFERENCE_VOLTAGE = 3.3;  // Опорное напряжение ESP32

BatteryMonitor::BatteryMonitor(int pin, float ratio) 
    : adcPin(pin), voltageDividerRatio(ratio), lastAdcValue(0), 
      lastVoltage(0.0), lastPercentage(0.0), lastReadTime(0) {
}

void BatteryMonitor::begin() {
    pinMode(adcPin, INPUT);
    // Настраиваем АЦП для более точных измерений
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // Для измерения до 3.3V
    
    Serial.println("Battery Monitor initialized");
    Serial.printf("ADC Pin: GPIO%d\n", adcPin);
    Serial.printf("Voltage Divider Ratio: %.2f\n", voltageDividerRatio);
    
    // Первоначальное чтение
    update();
}

float BatteryMonitor::readVoltage() {
    int adcValue = analogRead(adcPin);
    lastAdcValue = adcValue;
    
    // Преобразуем значение АЦП в напряжение
    float voltage = (adcValue * REFERENCE_VOLTAGE / ADC_RESOLUTION) * voltageDividerRatio;
    lastVoltage = voltage;
    lastReadTime = millis();
    
    return voltage;
}

float BatteryMonitor::getBatteryPercentage() {
    float voltage = readVoltage();
    
    // Ограничиваем напряжение в диапазоне
    if (voltage > MAX_VOLTAGE) voltage = MAX_VOLTAGE;
    if (voltage < MIN_VOLTAGE) voltage = MIN_VOLTAGE;
    
    // Вычисляем процент заряда
    float percentage = ((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0;
    lastPercentage = percentage;
    
    return percentage;
}

int BatteryMonitor::getRawADC() {
    return lastAdcValue;
}

String BatteryMonitor::getBatteryStatus() {
    float percentage = getBatteryPercentage();
    
    if (percentage >= 80) {
        return "Excellent";
    } else if (percentage >= 60) {
        return "Good";
    } else if (percentage >= 40) {
        return "Fair";
    } else if (percentage >= 20) {
        return "Low";
    } else {
        return "Critical";
    }
}

bool BatteryMonitor::isLowBattery() {
    return getBatteryPercentage() < 20.0;
}

void BatteryMonitor::update() {
    getBatteryPercentage(); // Это обновит все значения
}

void BatteryMonitor::printBatteryInfo() {
    update();
    
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│          BATTERY STATUS             │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Voltage:     %.3f V               │\n", lastVoltage);
    Serial.printf("│ Percentage:  %.1f %%                │\n", lastPercentage);
    Serial.printf("│ Status:      %-12s         │\n", getBatteryStatus().c_str());
    Serial.printf("│ ADC Raw:     %-4d                  │\n", lastAdcValue);
    Serial.printf("│ Read Time:   %-10lu ms         │\n", lastReadTime);
    
    if (isLowBattery()) {
        Serial.println("│ ⚠️  WARNING: LOW BATTERY!           │");
    }
    
    Serial.println("└─────────────────────────────────────┘");
}

// Функция для вывода краткой информации в одну строку
void BatteryMonitor::printBatteryShort() {
    update();
    Serial.printf("[BATTERY] %.3fV | %.1f%% | %s | ADC:%d\n", 
                  lastVoltage, lastPercentage, getBatteryStatus().c_str(), lastAdcValue);
}
