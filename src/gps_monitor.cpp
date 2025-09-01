/**
 * @file      gps_monitor.cpp
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     GPS monitoring module implementation
 */

#include "gps_monitor.h"

GPSMonitor::GPSMonitor(HardwareSerial* ser) 
    : serial(ser), gpsEnabled(false), lastUpdateTime(0) {
    lastGPSData.isValid = false;
    lastGPSData.latitude = 0.0;
    lastGPSData.longitude = 0.0;
    lastGPSData.altitude = 0.0;
    lastGPSData.speed = 0.0;
    lastGPSData.course = 0.0;
    lastGPSData.satellites = 0;
    lastGPSData.hdop = 0.0;
    lastGPSData.timestamp = "";
    lastGPSData.date = "";
}

void GPSMonitor::begin() {
    Serial.println("GPS Monitor initialized");
    delay(1000);
    
    // Включаем GPS
    if (enableGPS()) {
        Serial.println("GPS module enabled successfully");
        gpsEnabled = true;
        delay(2000); // Дать GPS время для инициализации
        update(); // Первоначальное обновление
    } else {
        Serial.println("Warning: Failed to enable GPS module");
        gpsEnabled = false;
    }
}

String GPSMonitor::sendATCommand(const String& command, unsigned long timeout) {
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
        }
        delay(10);
    }
    
    lastResponse = response;
    return response;
}

bool GPSMonitor::enableGPS() {
    Serial.println("Attempting to enable GPS...");
    
    // Пробуем команды для A7670G
    // Сначала пробуем AT+CGPS=1,1 (включить GPS)
    String response = sendATCommand("AT+CGPS=1,1", 5000);
    Serial.print("GPS power response (CGPS): ");
    Serial.println(response);
    
    if (response.indexOf("OK") != -1) {
        Serial.println("GPS enabled with AT+CGPS=1,1");
        delay(2000);
        return true;
    }
    
    // Если не сработало, пробуем AT+CGPS=1
    response = sendATCommand("AT+CGPS=1", 5000);
    Serial.print("GPS power response (CGPS=1): ");
    Serial.println(response);
    
    if (response.indexOf("OK") != -1) {
        Serial.println("GPS enabled with AT+CGPS=1");
        delay(2000);
        return true;
    }
    
    // Пробуем старые команды для совместимости
    response = sendATCommand("AT+CGNSPWR=1", 5000);
    Serial.print("GPS power response (CGNSPWR): ");
    Serial.println(response);
    
    if (response.indexOf("OK") != -1) {
        Serial.println("GPS enabled with AT+CGNSPWR=1");
        delay(2000);
        return true;
    }
    
    Serial.println("Failed to power on GPS - no compatible command found");
    Serial.print("Last response: [");
    Serial.print(response);
    Serial.println("]");
    return false;
}

void GPSMonitor::disableGPS() {
    sendATCommand("AT+CGNSPWR=0", 2000);
    gpsEnabled = false;
}

void GPSMonitor::parseGNSSInfo(const String& response) {
    // Парсинг ответа +CGNSINF: <GNSS run status>,<Fix status>,<UTC date & Time>,
    // <Latitude>,<Longitude>,<MSL Altitude>,<Speed Over Ground>,<Course Over Ground>,
    // <Fix Mode>,<Reserved1>,<HDOP>,<PDOP>,<VDOP>,<Reserved2>,<GNSS Satellites in View>,
    // <GNSS Satellites Used>,<GLONASS Satellites Used>,<Reserved3>,<C/N0 max>,<HPA>,<VPA>
    
    int infIndex = response.indexOf("+CGNSINF:");
    if (infIndex != -1) {
        String data = response.substring(infIndex + 10);
        data.trim();
        
        // Разделяем данные по запятым
        int fieldIndex = 0;
        int startIndex = 0;
        
        for (int i = 0; i <= data.length(); i++) {
            if (i == data.length() || data.charAt(i) == ',') {
                String field = data.substring(startIndex, i);
                field.trim();
                
                switch (fieldIndex) {
                    case 0: // GNSS run status
                        break;
                    case 1: // Fix status
                        lastGPSData.isValid = (field == "1");
                        break;
                    case 2: // UTC date & Time
                        if (field.length() >= 14) {
                            lastGPSData.date = field.substring(0, 8);
                            lastGPSData.timestamp = field.substring(8);
                        }
                        break;
                    case 3: // Latitude
                        if (field.length() > 0) {
                            lastGPSData.latitude = field.toFloat();
                        }
                        break;
                    case 4: // Longitude
                        if (field.length() > 0) {
                            lastGPSData.longitude = field.toFloat();
                        }
                        break;
                    case 5: // MSL Altitude
                        if (field.length() > 0) {
                            lastGPSData.altitude = field.toFloat();
                        }
                        break;
                    case 6: // Speed Over Ground
                        if (field.length() > 0) {
                            lastGPSData.speed = field.toFloat();
                        }
                        break;
                    case 7: // Course Over Ground
                        if (field.length() > 0) {
                            lastGPSData.course = field.toFloat();
                        }
                        break;
                    case 10: // HDOP
                        if (field.length() > 0) {
                            lastGPSData.hdop = field.toFloat();
                        }
                        break;
                    case 14: // GNSS Satellites in View
                        if (field.length() > 0) {
                            lastGPSData.satellites = field.toInt();
                        }
                        break;
                }
                
                startIndex = i + 1;
                fieldIndex++;
                
                if (fieldIndex > 15) break; // Достаточно полей
            }
        }
    }
}

void GPSMonitor::parseCGPSInfo(const String& response) {
    // Парсинг ответа +CGPSINFO: для A7670G
    // Формат может быть: +CGPSINFO: <lat>,<lat_NS>,<lon>,<lon_EW>,<date>,<UTC time>,<alt>,<speed>,<course>
    
    int infIndex = response.indexOf("+CGPSINFO:");
    if (infIndex != -1) {
        String data = response.substring(infIndex + 11);
        data.trim();
        
        Serial.print("Parsing CGPSINFO data: ");
        Serial.println(data);
        
        // Если данные пустые или содержат только запятые, значит нет фикса
        if (data.length() == 0 || data.indexOf(",,") == 0) {
            lastGPSData.isValid = false;
            Serial.println("No GPS fix - empty data");
            return;
        }
        
        // Разделяем данные по запятым
        int fieldIndex = 0;
        int startIndex = 0;
        float tempLat = 0, tempLon = 0;
        String latNS = "", lonEW = "";
        
        for (int i = 0; i <= data.length(); i++) {
            if (i == data.length() || data.charAt(i) == ',') {
                String field = data.substring(startIndex, i);
                field.trim();
                
                switch (fieldIndex) {
                    case 0: // Latitude
                        if (field.length() > 0) {
                            tempLat = field.toFloat();
                        }
                        break;
                    case 1: // Latitude N/S
                        latNS = field;
                        break;
                    case 2: // Longitude
                        if (field.length() > 0) {
                            tempLon = field.toFloat();
                        }
                        break;
                    case 3: // Longitude E/W
                        lonEW = field;
                        break;
                    case 4: // Date
                        lastGPSData.date = field;
                        break;
                    case 5: // UTC time
                        lastGPSData.timestamp = field;
                        break;
                    case 6: // Altitude
                        if (field.length() > 0) {
                            lastGPSData.altitude = field.toFloat();
                        }
                        break;
                    case 7: // Speed
                        if (field.length() > 0) {
                            lastGPSData.speed = field.toFloat();
                        }
                        break;
                    case 8: // Course
                        if (field.length() > 0) {
                            lastGPSData.course = field.toFloat();
                        }
                        break;
                }
                
                startIndex = i + 1;
                fieldIndex++;
                
                if (fieldIndex > 8) break;
            }
        }
        
        // Конвертируем координаты в десятичные градусы
        if (tempLat > 0 && tempLon > 0) {
            lastGPSData.latitude = convertDMMtoDD(tempLat);
            lastGPSData.longitude = convertDMMtoDD(tempLon);
            
            // Применяем знак в зависимости от полушария
            if (latNS == "S") lastGPSData.latitude = -lastGPSData.latitude;
            if (lonEW == "W") lastGPSData.longitude = -lastGPSData.longitude;
            
            lastGPSData.isValid = true;
            lastGPSData.satellites = 4; // Предполагаем минимум 4 спутника для фикса
        } else {
            lastGPSData.isValid = false;
        }
    }
}

// Конвертация из формата DDMM.MMMM в DD.DDDDDD
float GPSMonitor::convertDMMtoDD(float dmm) {
    int degrees = (int)(dmm / 100);
    float minutes = dmm - (degrees * 100);
    return degrees + (minutes / 60.0);
}

void GPSMonitor::update() {
    if (!serial) return;
    
    // Пробуем получить информацию о GPS разными способами
    String gpsResponse;
    
    // Для A7670G пробуем AT+CGPSINFO
    gpsResponse = sendATCommand("AT+CGPSINFO", 3000);
    if (gpsResponse.indexOf("+CGPSINFO:") != -1) {
        Serial.print("GPS info (CGPSINFO): ");
        Serial.println(gpsResponse);
        parseCGPSInfo(gpsResponse);
        lastUpdateTime = millis();
        return;
    }
    
    // Пробуем стандартную команду
    gpsResponse = sendATCommand("AT+CGNSINF", 3000);
    if (gpsResponse.indexOf("+CGNSINF:") != -1) {
        Serial.print("GPS info (CGNSINF): ");
        Serial.println(gpsResponse);
        parseGNSSInfo(gpsResponse);
        lastUpdateTime = millis();
        return;
    }
    
    Serial.println("No GPS data available from any command");
    lastUpdateTime = millis();
}

GPSData GPSMonitor::getGPSData() {
    return lastGPSData;
}

bool GPSMonitor::isGPSValid() {
    return lastGPSData.isValid;
}

float GPSMonitor::getLatitude() {
    return lastGPSData.latitude;
}

float GPSMonitor::getLongitude() {
    return lastGPSData.longitude;
}

float GPSMonitor::getAltitude() {
    return lastGPSData.altitude;
}

float GPSMonitor::getSpeed() {
    return lastGPSData.speed;
}

int GPSMonitor::getSatellites() {
    return lastGPSData.satellites;
}

float GPSMonitor::getHDOP() {
    return lastGPSData.hdop;
}

String GPSMonitor::getCoordinatesString() {
    if (!lastGPSData.isValid) {
        return "No GPS Fix";
    }
    
    return String(lastGPSData.latitude, 6) + ", " + String(lastGPSData.longitude, 6);
}

String GPSMonitor::getGPSStatus() {
    // Сначала проверим, включен ли GPS
    if (!isGPSReady()) {
        return "GPS Disabled";
    } else if (!lastGPSData.isValid) {
        return "No Fix";
    } else if (lastGPSData.satellites >= 4) {
        return "Good Fix";
    } else {
        return "Poor Fix";
    }
}

bool GPSMonitor::isGPSReady() {
    // Проверяем разные варианты команд статуса GPS
    String response = sendATCommand("AT+CGPS?", 2000);
    Serial.print("GPS ready check response (CGPS): ");
    Serial.println(response);
    
    if (response.indexOf("+CGPS: 1") != -1) {
        Serial.println("GPS ready status: true (CGPS)");
        return true;
    }
    
    // Пробуем старую команду
    response = sendATCommand("AT+CGNSPWR?", 2000);
    Serial.print("GPS ready check response (CGNSPWR): ");
    Serial.println(response);
    
    bool isReady = response.indexOf("+CGNSPWR: 1") != -1;
    Serial.print("GPS ready status: ");
    Serial.println(isReady ? "true (CGNSPWR)" : "false");
    
    return isReady;
}

void GPSMonitor::printGPSInfo() {
    update();
    
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│            GPS STATUS               │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Status:      %-20s │\n", getGPSStatus().c_str());
    Serial.printf("│ Satellites:  %-20d │\n", lastGPSData.satellites);
    Serial.printf("│ HDOP:        %-20.2f │\n", lastGPSData.hdop);
    
    if (lastGPSData.isValid) {
        Serial.printf("│ Latitude:    %-20.6f │\n", lastGPSData.latitude);
        Serial.printf("│ Longitude:   %-20.6f │\n", lastGPSData.longitude);
        Serial.printf("│ Altitude:    %-20.1f │\n", lastGPSData.altitude);
        Serial.printf("│ Speed:       %-20.1f │\n", lastGPSData.speed);
        Serial.printf("│ Course:      %-20.1f │\n", lastGPSData.course);
        Serial.printf("│ Date:        %-20s │\n", lastGPSData.date.c_str());
        Serial.printf("│ Time:        %-20s │\n", lastGPSData.timestamp.c_str());
    } else {
        Serial.println("│ No GPS fix available                │");
    }
    
    Serial.printf("│ Update:      %-10lu ms         │\n", lastUpdateTime);
    
    if (!lastGPSData.isValid && gpsEnabled) {
        Serial.println("│ ⚠️  WARNING: NO GPS FIX!            │");
    }
    
    Serial.println("└─────────────────────────────────────┘");
}

void GPSMonitor::printGPSShort() {
    update();
    
    if (lastGPSData.isValid) {
        Serial.printf("[GPS] %s | Sats:%d | Alt:%.1fm | Speed:%.1fkm/h | HDOP:%.2f\n", 
                      getCoordinatesString().c_str(),
                      lastGPSData.satellites,
                      lastGPSData.altitude,
                      lastGPSData.speed,
                      lastGPSData.hdop);
    } else {
        Serial.printf("[GPS] %s | Sats:%d | Searching for satellites...\n", 
                      getGPSStatus().c_str(),
                      lastGPSData.satellites);
    }
}
