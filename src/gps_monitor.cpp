/**
 * @file      gps_monitor.cpp
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     GPS monitoring module implementation using TinyGSM GPS functions (по примеру GPS_BuiltIn)
 */

#include "gps_monitor.h"

GPSMonitor::GPSMonitor(TinyGsm* gsm)
    : modem(gsm), gpsEnabled(false), lastUpdateTime(0), updateInterval(DEFAULT_UPDATE_INTERVAL) {
    initializeGPSData();
}

void GPSMonitor::initializeGPSData() {
    currentData = {};
    currentData.isValid = false;
    currentData.rawData = "";
    currentData.lastUpdate = 0;
}

bool GPSMonitor::begin() {
    if (!modem) return false;
    Serial.println("GPS Monitor: Initializing...");
    String modemName = modem->getModemName();
    Serial.print("GPS Monitor: Modem model: "); Serial.println(modemName);
    if (modemName == "UNKNOWN") {
        Serial.println("GPS Monitor: Unable to obtain module information");
        return false;
    }
    // Проверка поддержки GPS
    if (modemName.startsWith("A7670E-LNXY-UBL") ||
        modemName.startsWith("A7670SA-LASE") ||
        modemName.startsWith("A7670SA-LASC") ||
        modemName.startsWith("A7670G-LLSE") ||
        modemName.startsWith("A7670G-LABE") ||
        modemName.startsWith("A7670E-LASE ")) {
        Serial.println("GPS Monitor: This modem does not have built-in GPS function.");
        return false;
    }
    // Включаем GPS
    Serial.println("GPS Monitor: Enabling GPS/GNSS/GLONASS...");
    int retry = 0;
    while (
#if defined(MODEM_GPS_ENABLE_GPIO) && (MODEM_GPS_ENABLE_GPIO != -1)
    !modem->enableGPS(MODEM_GPS_ENABLE_GPIO, MODEM_GPS_ENABLE_LEVEL)
#else
    !modem->enableGPS()
#endif
    ) {
        Serial.print(".");
        delay(500);
        if (++retry > 10) {
            Serial.println("\nGPS Monitor: Failed to enable GPS");
            return false;
        }
    }
    Serial.println("\nGPS Monitor: GPS enabled");
    modem->setGPSBaud(115200);
    gpsEnabled = true;
    delay(2000);
    return true;
}

bool GPSMonitor::enableGPSModule() {
    if (!modem) return false;
#if defined(MODEM_GPS_ENABLE_GPIO) && (MODEM_GPS_ENABLE_GPIO != -1)
    return modem->enableGPS(MODEM_GPS_ENABLE_GPIO, MODEM_GPS_ENABLE_LEVEL);
#else
    return modem->enableGPS();
#endif
}

void GPSMonitor::disableGPSModule() {
    if (!modem || !gpsEnabled) return;
    #if defined(MODEM_GPS_ENABLE_GPIO) && (MODEM_GPS_ENABLE_GPIO != -1)
    modem->disableGPS(MODEM_GPS_ENABLE_GPIO, !MODEM_GPS_ENABLE_LEVEL);
    #else
    modem->disableGPS();
    #endif
    gpsEnabled = false;
    initializeGPSData();
}

bool GPSMonitor::update(bool forceUpdate) {
    if (!modem || !gpsEnabled) return false;
    unsigned long now = millis();
    if (!forceUpdate && (now - lastUpdateTime < updateInterval)) return false;
    lastUpdateTime = now;
    float lat=0, lon=0, speed=0, alt=0, accuracy=0;
    int vsat=0, usat=0, year=0, month=0, day=0, hour=0, min=0, sec=0;
    uint8_t fixMode=0;
    bool ok = modem->getGPS(&fixMode, &lat, &lon, &speed, &alt, &vsat, &usat, &accuracy,
                            &year, &month, &day, &hour, &min, &sec);
    if (ok && fixMode > 0) {
        currentData.isValid = true;
        currentData.latitude = lat;
        currentData.longitude = lon;
        currentData.altitude = alt;
        currentData.speed = speed;
        currentData.visibleSatellites = vsat;
        currentData.usedSatellites = usat;
        currentData.accuracy = accuracy;
        currentData.fixMode = fixMode;
        currentData.year = year;
        currentData.month = month;
        currentData.day = day;
        currentData.hour = hour;
        currentData.minute = min;
        currentData.second = sec;
        currentData.lastUpdate = now;
        currentData.rawData = modem->getGPSraw();
        return true;
    } else {
        currentData.isValid = false;
        currentData.fixMode = fixMode;
        currentData.lastUpdate = now;
        return false;
    }
}

GPSData GPSMonitor::getGPSData() const { return currentData; }
bool GPSMonitor::isGPSValid() const { return currentData.isValid && currentData.fixMode > 0; }
float GPSMonitor::getLatitude() const { return currentData.latitude; }
float GPSMonitor::getLongitude() const { return currentData.longitude; }
float GPSMonitor::getAltitude() const { return currentData.altitude; }
float GPSMonitor::getSpeed() const { return currentData.speed; }
int GPSMonitor::getVisibleSatellites() const { return currentData.visibleSatellites; }
int GPSMonitor::getUsedSatellites() const { return currentData.usedSatellites; }
float GPSMonitor::getAccuracy() const { return currentData.accuracy; }
uint8_t GPSMonitor::getFixMode() const { return currentData.fixMode; }
String GPSMonitor::getRawGPSData() { return modem && gpsEnabled ? modem->getGPSraw() : ""; }
String GPSMonitor::getCoordinatesString() const {
    if (!isGPSValid()) return "No GPS fix";
    return String("Lat: ") + String(currentData.latitude, 6) + ", Lon: " + String(currentData.longitude, 6);
}
String GPSMonitor::getGPSStatus() const {
    if (!gpsEnabled) return "GPS Disabled";
    if (!currentData.isValid) return "No GPS fix";
    String status = "GPS Fix ";
    switch (currentData.fixMode) {
        case 0: status += "(No Fix)"; break;
        case 1: status += "(Dead Reckoning)"; break;
        case 2: status += "(2D)"; break;
        case 3: status += "(3D)"; break;
        default: status += "(Unknown)"; break;
    }
    status += " - " + String(currentData.visibleSatellites) + " sats";
    return status;
}
unsigned long GPSMonitor::getLastUpdateTime() const { return currentData.lastUpdate; }
void GPSMonitor::setUpdateInterval(unsigned long interval) { updateInterval = interval; }
void GPSMonitor::printGPSInfo() const {
    Serial.println("=== GPS Information ===");
    Serial.printf("GPS Enabled: %s\n", gpsEnabled ? "Yes" : "No");
    Serial.printf("GPS Valid: %s\n", isGPSValid() ? "Yes" : "No");
    if (isGPSValid()) {
        Serial.printf("Fix Mode: %d\n", currentData.fixMode);
        Serial.printf("Latitude: %.6f°\n", currentData.latitude);
        Serial.printf("Longitude: %.6f°\n", currentData.longitude);
        Serial.printf("Altitude: %.2f m\n", currentData.altitude);
        Serial.printf("Speed: %.2f knots\n", currentData.speed);
        Serial.printf("Visible Satellites: %d\n", currentData.visibleSatellites);
        Serial.printf("Used Satellites: %d\n", currentData.usedSatellites);
        Serial.printf("Accuracy: %.2f m\n", currentData.accuracy);
        if (currentData.year > 0) {
            Serial.printf("Date/Time: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                         currentData.year, currentData.month, currentData.day,
                         currentData.hour, currentData.minute, currentData.second);
        }
        Serial.printf("Last Update: %lu ms ago\n", millis() - currentData.lastUpdate);
    } else {
        Serial.printf("Status: %s\n", getGPSStatus().c_str());
        if (currentData.lastUpdate > 0) {
            Serial.printf("Last Update Attempt: %lu ms ago\n", millis() - currentData.lastUpdate);
        }
    }
    Serial.println("======================");
}
void GPSMonitor::printGPSShort() const {
    if (isGPSValid()) {
        Serial.printf("[GPS] Fix:%d Lat:%.6f Lon:%.6f Alt:%.1fm Sats:%d Speed:%.1fkn\n",
                     currentData.fixMode, currentData.latitude, currentData.longitude, 
                     currentData.altitude, currentData.visibleSatellites, currentData.speed);
    } else {
        Serial.printf("[GPS] %s\n", getGPSStatus().c_str());
    }
}
bool GPSMonitor::isGPSReady() const { return gpsEnabled && modem != nullptr; }
void GPSMonitor::end() { disableGPSModule(); Serial.println("GPS Monitor: Shutdown complete"); }
