/**
 * @file      gsm_monitor.cpp
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     GSM network monitoring module implementation
 */

#include "gsm_monitor.h"

GSMMonitor::GSMMonitor(HardwareSerial* ser) 
    : serial(ser), signalQuality(-1), networkOperator("Unknown"), 
      networkStatus("Unknown"), isRegistered(false), networkType("Unknown"), 
      lastUpdateTime(0) {
}

void GSMMonitor::begin() {
    Serial.println("GSM Monitor initialized");
    delay(1000); // Дать модему время для готовности
    
    // Проверим готовность модема
    if (isModemReady()) {
        Serial.println("Modem is ready for GSM monitoring");
        update(); // Первоначальное обновление
    } else {
        Serial.println("Warning: Modem not responding to AT commands");
    }
}

String GSMMonitor::sendATCommand(const String& command, unsigned long timeout) {
    if (!serial) return "";
    
    // Очистить буфер
    while (serial->available()) {
        serial->read();
    }
    
    // Отправить команду
    serial->println(command);
    
    // Ждать ответ
    unsigned long start = millis();
    String response = "";
    
    while (millis() - start < timeout) {
        if (serial->available()) {
            char c = serial->read();
            response += c;
            
            // Если получили OK или ERROR, завершаем
            if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1) {
                break;
            }
        }
        delay(10);
    }
    
    lastResponse = response;
    return response;
}

void GSMMonitor::parseSignalQuality(const String& response) {
    // Парсинг ответа +CSQ: XX,YY
    int csqIndex = response.indexOf("+CSQ:");
    if (csqIndex != -1) {
        int commaIndex = response.indexOf(',', csqIndex);
        if (commaIndex != -1) {
            String rssiStr = response.substring(csqIndex + 6, commaIndex);
            rssiStr.trim();
            signalQuality = rssiStr.toInt();
        }
    }
}

void GSMMonitor::parseNetworkStatus(const String& response) {
    // Парсинг ответа +CREG: N,stat
    int cregIndex = response.indexOf("+CREG:");
    if (cregIndex != -1) {
        int commaIndex = response.indexOf(',', cregIndex);
        if (commaIndex != -1) {
            String statusStr = response.substring(commaIndex + 1, commaIndex + 2);
            int status = statusStr.toInt();
            
            switch (status) {
                case 0:
                    networkStatus = "Not searching";
                    isRegistered = false;
                    break;
                case 1:
                    networkStatus = "Registered (Home)";
                    isRegistered = true;
                    break;
                case 2:
                    networkStatus = "Searching";
                    isRegistered = false;
                    break;
                case 3:
                    networkStatus = "Registration denied";
                    isRegistered = false;
                    break;
                case 5:
                    networkStatus = "Registered (Roaming)";
                    isRegistered = true;
                    break;
                default:
                    networkStatus = "Unknown";
                    isRegistered = false;
                    break;
            }
        }
    }
}

void GSMMonitor::parseOperator(const String& response) {
    // Парсинг ответа +COPS: mode,format,"operator"
    int copsIndex = response.indexOf("+COPS:");
    if (copsIndex != -1) {
        int firstQuote = response.indexOf('"', copsIndex);
        if (firstQuote != -1) {
            int secondQuote = response.indexOf('"', firstQuote + 1);
            if (secondQuote != -1) {
                networkOperator = response.substring(firstQuote + 1, secondQuote);
            }
        }
    }
    
    if (networkOperator.length() == 0 || networkOperator == "\"\"") {
        networkOperator = "No operator";
    }
}

void GSMMonitor::update() {
    if (!serial) return;
    
    // Обновляем качество сигнала
    String csqResponse = sendATCommand("AT+CSQ", 2000);
    parseSignalQuality(csqResponse);
    
    // Обновляем статус сети
    String cregResponse = sendATCommand("AT+CREG?", 2000);
    parseNetworkStatus(cregResponse);
    
    // Обновляем оператора
    String copsResponse = sendATCommand("AT+COPS?", 3000);
    parseOperator(copsResponse);
    
    // Пытаемся определить тип сети
    String ctechResponse = sendATCommand("AT+COPS=3,2", 2000);
    if (ctechResponse.indexOf("OK") != -1) {
        networkType = "GSM/LTE";
    }
    
    lastUpdateTime = millis();
}

int GSMMonitor::getSignalQuality() {
    return signalQuality;
}

int GSMMonitor::getSignalStrength() {
    if (signalQuality == 99 || signalQuality < 0) {
        return -999; // Неизвестно
    }
    // Конверсия из CSQ в dBm: dBm = -113 + 2 * CSQ
    return -113 + (2 * signalQuality);
}

String GSMMonitor::getOperator() {
    return networkOperator;
}

String GSMMonitor::getNetworkStatus() {
    return networkStatus;
}

bool GSMMonitor::isNetworkRegistered() {
    return isRegistered;
}

String GSMMonitor::getNetworkType() {
    return networkType;
}

String GSMMonitor::getSignalQualityString() {
    if (signalQuality == 99 || signalQuality < 0) {
        return "Unknown";
    } else if (signalQuality >= 20) {
        return "Excellent";
    } else if (signalQuality >= 15) {
        return "Good";
    } else if (signalQuality >= 10) {
        return "Fair";
    } else if (signalQuality >= 5) {
        return "Poor";
    } else {
        return "Very Poor";
    }
}

bool GSMMonitor::isModemReady() {
    String response = sendATCommand("AT", 1000);
    return response.indexOf("OK") != -1;
}

void GSMMonitor::printGSMInfo() {
    update();
    
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│           GSM STATUS                │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Operator:    %-20s │\n", networkOperator.c_str());
    Serial.printf("│ Status:      %-20s │\n", networkStatus.c_str());
    Serial.printf("│ Signal:      %-2d/31 (%-11s) │\n", signalQuality, getSignalQualityString().c_str());
    Serial.printf("│ Strength:    %-4d dBm             │\n", getSignalStrength());
    Serial.printf("│ Network:     %-20s │\n", networkType.c_str());
    Serial.printf("│ Registered:  %-20s │\n", isRegistered ? "Yes" : "No");
    Serial.printf("│ Update:      %-10lu ms         │\n", lastUpdateTime);
    
    if (!isRegistered) {
        Serial.println("│ ⚠️  WARNING: NOT REGISTERED!        │");
    }
    
    Serial.println("└─────────────────────────────────────┘");
}

void GSMMonitor::printGSMShort() {
    update();
    Serial.printf("[GSM] %s | %s | Signal:%d/31(%ddBm) | %s | Reg:%s\n", 
                  networkOperator.c_str(), 
                  networkStatus.c_str(),
                  signalQuality, 
                  getSignalStrength(),
                  getSignalQualityString().c_str(),
                  isRegistered ? "YES" : "NO");
}
