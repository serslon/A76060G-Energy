/**
 * @file      battery_monitor.h
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     Battery monitoring module for A7670G Energy Project
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include "Arduino.h"

class BatteryMonitor {
private:
    int adcPin;
    float voltageDividerRatio;
    int lastAdcValue;
    float lastVoltage;
    float lastPercentage;
    unsigned long lastReadTime;
    
    // Калибровочные значения для Li-ion батареи
    static const float MIN_VOLTAGE;    // Минимальное напряжение (3.0V)
    static const float MAX_VOLTAGE;    // Максимальное напряжение (4.2V)
    static const int ADC_RESOLUTION;   // Разрешение АЦП ESP32
    static const float REFERENCE_VOLTAGE; // Опорное напряжение ESP32
    
public:
    /**
     * @brief Конструктор
     * @param pin GPIO пин для чтения напряжения батареи
     * @param ratio Коэффициент делителя напряжения (обычно 2.0 для делителя 1:1)
     */
    BatteryMonitor(int pin, float ratio = 2.0);
    
    /**
     * @brief Инициализация модуля
     */
    void begin();
    
    /**
     * @brief Чтение текущего напряжения батареи
     * @return Напряжение в вольтах
     */
    float readVoltage();
    
    /**
     * @brief Получение уровня заряда в процентах
     * @return Заряд в процентах (0-100)
     */
    float getBatteryPercentage();
    
    /**
     * @brief Получение сырого значения АЦП
     * @return Значение АЦП (0-4095)
     */
    int getRawADC();
    
    /**
     * @brief Получение статуса батареи в виде строки
     * @return Строка с описанием состояния батареи
     */
    String getBatteryStatus();
    
    /**
     * @brief Проверка, нужна ли зарядка батареи
     * @return true если батарея разряжена
     */
    bool isLowBattery();
    
    /**
     * @brief Обновление показаний батареи
     */
    void update();
    
    /**
     * @brief Вывод полной информации о батарее в Serial
     */
    void printBatteryInfo();
    
    /**
     * @brief Вывод краткой информации о батарее в одну строку
     */
    void printBatteryShort();
};

#endif // BATTERY_MONITOR_H
