/**
 * @file      gps_monitor.h
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     GPS monitoring module for A7670G Energy Project using TinyGSM GPS functions
 */

#ifndef GPS_MONITOR_H
#define GPS_MONITOR_H

#include "Arduino.h"

// Определяем тип модема перед включением TinyGSM
#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif

#include <TinyGsmClient.h>

struct GPSData {
    bool isValid;
    float latitude;
    float longitude;
    float altitude;
    float speed;
    int visibleSatellites;
    int usedSatellites;
    float accuracy;
    uint8_t fixMode;
    
    // Данные времени
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    
    String rawData;
    unsigned long lastUpdate;
};

class GPSMonitor {
private:
    TinyGsm* modem;
    GPSData currentData;
    bool gpsEnabled;
    unsigned long lastUpdateTime;
    unsigned long updateInterval;
    
    // Константы
    static const unsigned long DEFAULT_UPDATE_INTERVAL = 30000; // 30 секунд
    static const unsigned long GPS_TIMEOUT = 15000; // 15 секунд таймаут для получения GPS
    
    // Внутренние методы
    void initializeGPSData();
    bool enableGPSModule();
    void disableGPSModule();
    String getGPSStatusString();
    
public:
    /**
     * @brief Конструктор
     * @param gsm Указатель на TinyGsm модем
     */
    GPSMonitor(TinyGsm* gsm);
    
    /**
     * @brief Инициализация модуля GPS
     * @return true если успешно инициализирован
     */
    bool begin();
    
    /**
     * @brief Обновление GPS данных
     * @param forceUpdate Принудительное обновление независимо от интервала
     * @return true если данные обновлены
     */
    bool update(bool forceUpdate = false);
    
    /**
     * @brief Получение текущих GPS данных
     * @return Структура с GPS данными
     */
    GPSData getGPSData() const;
    
    /**
     * @brief Проверка валидности GPS данных
     * @return true если GPS данные валидны
     */
    bool isGPSValid() const;
    
    /**
     * @brief Получение широты
     * @return Широта в градусах
     */
    float getLatitude() const;
    
    /**
     * @brief Получение долготы
     * @return Долгота в градусах
     */
    float getLongitude() const;
    
    /**
     * @brief Получение высоты
     * @return Высота в метрах
     */
    float getAltitude() const;
    
    /**
     * @brief Получение скорости
     * @return Скорость в узлах
     */
    float getSpeed() const;
    
    /**
     * @brief Получение количества видимых спутников
     * @return Количество видимых спутников
     */
    int getVisibleSatellites() const;
    
    /**
     * @brief Получение количества используемых спутников
     * @return Количество используемых спутников
     */
    int getUsedSatellites() const;
    
    /**
     * @brief Получение точности позиционирования
     * @return Точность в метрах
     */
    float getAccuracy() const;
    
    /**
     * @brief Получение режима фиксации GPS
     * @return Режим фиксации (0=нет фикса, 2=2D, 3=3D)
     */
    uint8_t getFixMode() const;
    
    /**
     * @brief Получение сырых GPS данных
     * @return Строка с сырыми GPS данными
     */
    String getRawGPSData();
    
    /**
     * @brief Получение координат в виде строки
     * @return Строка с координатами
     */
    String getCoordinatesString() const;
    
    /**
     * @brief Получение статуса GPS в виде строки
     * @return Строка с статусом GPS
     */
    String getGPSStatus() const;
    
    /**
     * @brief Получение времени последнего обновления
     * @return Время в миллисекундах
     */
    unsigned long getLastUpdateTime() const;
    
    /**
     * @brief Установка интервала обновления
     * @param interval Интервал в миллисекундах
     */
    void setUpdateInterval(unsigned long interval);
    
    /**
     * @brief Вывод полной информации о GPS в Serial
     */
    void printGPSInfo() const;
    
    /**
     * @brief Вывод краткой информации о GPS в одну строку
     */
    void printGPSShort() const;
    
    /**
     * @brief Проверка готовности GPS модуля
     * @return true если GPS модуль готов
     */
    bool isGPSReady() const;
    
    /**
     * @brief Завершение работы GPS модуля
     */
    void end();
};

#endif // GPS_MONITOR_H
