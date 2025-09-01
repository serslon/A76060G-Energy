/**
 * @file      gsm_monitor.h
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     GSM network monitoring module for A7670G Energy Project
 */

#ifndef GSM_MONITOR_H
#define GSM_MONITOR_H

#include "Arduino.h"
#include "HardwareSerial.h"

class GSMMonitor {
private:
    HardwareSerial* serial;
    String lastResponse;
    int signalQuality;
    String networkOperator;
    String networkStatus;
    bool isRegistered;
    String networkType;
    unsigned long lastUpdateTime;
    
    // Внутренние методы
    String sendATCommand(const String& command, unsigned long timeout = 1000);
    void parseSignalQuality(const String& response);
    void parseNetworkStatus(const String& response);
    void parseOperator(const String& response);
    
public:
    /**
     * @brief Конструктор
     * @param ser Указатель на Serial для AT команд
     */
    GSMMonitor(HardwareSerial* ser);
    
    /**
     * @brief Инициализация модуля
     */
    void begin();
    
    /**
     * @brief Обновление всех параметров GSM сети
     */
    void update();
    
    /**
     * @brief Получение качества сигнала (0-31, 99=неизвестно)
     * @return Качество сигнала
     */
    int getSignalQuality();
    
    /**
     * @brief Получение качества сигнала в dBm
     * @return Сила сигнала в dBm
     */
    int getSignalStrength();
    
    /**
     * @brief Получение названия оператора
     * @return Название оператора
     */
    String getOperator();
    
    /**
     * @brief Получение статуса сети
     * @return Статус подключения к сети
     */
    String getNetworkStatus();
    
    /**
     * @brief Проверка регистрации в сети
     * @return true если зарегистрирован в сети
     */
    bool isNetworkRegistered();
    
    /**
     * @brief Получение типа сети
     * @return Тип сети (2G, 3G, 4G)
     */
    String getNetworkType();
    
    /**
     * @brief Получение качества сигнала в виде строки
     * @return Описание качества сигнала
     */
    String getSignalQualityString();
    
    /**
     * @brief Вывод полной информации о GSM в Serial
     */
    void printGSMInfo();
    
    /**
     * @brief Вывод краткой информации о GSM в одну строку
     */
    void printGSMShort();
    
    /**
     * @brief Проверка готовности модема
     * @return true если модем отвечает на AT команды
     */
    bool isModemReady();
};

#endif // GSM_MONITOR_H
