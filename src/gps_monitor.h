/**
 * @file      gps_monitor.h
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     GPS monitoring module for A7670G Energy Project
 */

#ifndef GPS_MONITOR_H
#define GPS_MONITOR_H

#include "Arduino.h"
#include "HardwareSerial.h"

struct GPSData {
    bool isValid;
    float latitude;
    float longitude;
    float altitude;
    float speed;
    float course;
    int satellites;
    float hdop;
    String timestamp;
    String date;
};

class GPSMonitor {
private:
    HardwareSerial* serial;
    GPSData lastGPSData;
    bool gpsEnabled;
    unsigned long lastUpdateTime;
    String lastResponse;
    
    // Внутренние методы
    String sendATCommand(const String& command, unsigned long timeout = 3000);
    void parseGNSSInfo(const String& response);
    void parseGPSLocation(const String& response);
    void parseCGPSInfo(const String& response);
    float convertDMMtoDD(float dmm);
    bool enableGPS();
    void disableGPS();
    
public:
    /**
     * @brief Конструктор
     * @param ser Указатель на Serial для AT команд
     */
    GPSMonitor(HardwareSerial* ser);
    
    /**
     * @brief Инициализация модуля GPS
     */
    void begin();
    
    /**
     * @brief Обновление GPS данных
     */
    void update();
    
    /**
     * @brief Получение текущих GPS данных
     * @return Структура с GPS данными
     */
    GPSData getGPSData();
    
    /**
     * @brief Проверка валидности GPS данных
     * @return true если GPS данные валидны
     */
    bool isGPSValid();
    
    /**
     * @brief Получение широты
     * @return Широта в градусах
     */
    float getLatitude();
    
    /**
     * @brief Получение долготы
     * @return Долгота в градусах
     */
    float getLongitude();
    
    /**
     * @brief Получение высоты
     * @return Высота в метрах
     */
    float getAltitude();
    
    /**
     * @brief Получение скорости
     * @return Скорость в км/ч
     */
    float getSpeed();
    
    /**
     * @brief Получение количества спутников
     * @return Количество видимых спутников
     */
    int getSatellites();
    
    /**
     * @brief Получение точности позиционирования
     * @return HDOP (Horizontal Dilution of Precision)
     */
    float getHDOP();
    
    /**
     * @brief Получение координат в виде строки
     * @return Строка с координатами
     */
    String getCoordinatesString();
    
    /**
     * @brief Получение статуса GPS в виде строки
     * @return Строка с статусом GPS
     */
    String getGPSStatus();
    
    /**
     * @brief Вывод полной информации о GPS в Serial
     */
    void printGPSInfo();
    
    /**
     * @brief Вывод краткой информации о GPS в одну строку
     */
    void printGPSShort();
    
    /**
     * @brief Проверка готовности GPS модуля
     * @return true если GPS модуль готов
     */
    bool isGPSReady();
};

#endif // GPS_MONITOR_H
