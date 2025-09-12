/**
 * @file      mqtt_manager.h
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     MQTT manager for A7670G Energy Project with HiveMQ Cloud
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "Arduino.h"
#include <ArduinoJson.h>

// Определяем тип модема перед включением TinyGSM
#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif

#include <TinyGsmClient.h>

// MQTT Configuration for HiveMQ Cloud
#define MQTT_BROKER_HOST "185193e761bb414c939c1b05bed985de.s1.eu.hivemq.cloud"
#define MQTT_BROKER_PORT 8883
#define MQTT_USERNAME "A7670G_Client"
#define MQTT_PASSWORD "hngoAAHpKI2rDQ"
#define MQTT_CLIENT_ID "A7670G_Energy_Monitor"

// Topic configuration
#define MQTT_TOPIC_BASE "A7670"
#define MQTT_TOPIC_BATTERY "A7670/battery"
#define MQTT_TOPIC_GSM "A7670/gsm"
#define MQTT_TOPIC_GPS "A7670/gps"
#define MQTT_TOPIC_STATUS "A7670/status"
#define MQTT_TOPIC_DEVICE "A7670/device"
#define MQTT_TOPIC_ALERTS "A7670/alerts"
#define MQTT_TOPIC_ALERTS "A7670/alerts"

// MQTT Settings
#define MQTT_KEEP_ALIVE_INTERVAL 60000 // 60 seconds
#define MQTT_RECONNECT_INTERVAL 10000  // 10 seconds

enum MQTTParameterType
{
    MQTT_PARAM_BATTERY_VOLTAGE,
    MQTT_PARAM_BATTERY_PERCENTAGE,
    MQTT_PARAM_BATTERY_STATUS,
    MQTT_PARAM_GSM_SIGNAL,
    MQTT_PARAM_GSM_OPERATOR,
    MQTT_PARAM_GSM_STATUS,
    MQTT_PARAM_GPS_LATITUDE,
    MQTT_PARAM_GPS_LONGITUDE,
    MQTT_PARAM_GPS_ALTITUDE,
    MQTT_PARAM_GPS_SATELLITES,
    MQTT_PARAM_SYSTEM_STATUS
};

class MQTTManager
{
private:
    TinyGsm *modem;
    bool isConnected;
    bool isInitialized;
    unsigned long lastConnectAttempt;
    unsigned long lastKeepAlive;

    // Internal methods
    bool connectToMQTT();
    String createPayload(float value);
    String createPayload(int value);
    String createPayload(const String &value);
    String getTopicForParameter(MQTTParameterType paramType);

public:
    /**
     * @brief Constructor
     * @param gsm Pointer to TinyGSM modem object
     */
    MQTTManager(TinyGsm *gsm);

    /**
     * @brief Initialize MQTT manager
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Check if MQTT is connected and handle reconnection
     * @return true if connected
     */
    bool isConnectedToMQTT();

    /**
     * @brief Handle MQTT operations (should be called in loop)
     */
    void handle();

    /**
     * @brief Publish float parameter to MQTT
     * @param paramType Parameter type
     * @param value Float value to publish
     * @return true if published successfully
     */
    bool publishParameter(MQTTParameterType paramType, float value);

    /**
     * @brief Publish integer parameter to MQTT
     * @param paramType Parameter type
     * @param value Integer value to publish
     * @return true if published successfully
     */
    bool publishParameter(MQTTParameterType paramType, int value);

    /**
     * @brief Publish string parameter to MQTT
     * @param paramType Parameter type
     * @param value String value to publish
     * @return true if published successfully
     */
    bool publishParameter(MQTTParameterType paramType, const String &value);

    /**
     * @brief Publish battery data
     * @param voltage Battery voltage
     * @param percentage Battery percentage
     * @param status Battery status string
     * @param timestamp Timestamp of the data
     * @return true if all published successfully
     */
    bool publishBatteryData(float voltage, int percentage, const String &status, unsigned long timestamp);

    /**
     * @brief Publish GSM data
     * @param signal Signal strength
     * @param operator_name Operator name
     * @param status Connection status
     * @return true if all published successfully
     */
    bool publishGSMData(int signal, const String &operator_name, const String &status);

    /**
     * @brief Publish GPS data
     * @param latitude GPS latitude
     * @param longitude GPS longitude
     * @param altitude GPS altitude
     * @param satellites Number of satellites
     * @return true if all published successfully
     */
    bool publishGPSData(float latitude, float longitude, float altitude, int satellites);

    /**
     * @brief Publish system status
     * @param status System status string
     * @return true if published successfully
     */
    bool publishSystemStatus(const String &status);

    /**
     * @brief Publish complete device data (battery, GSM, GPS, system)
     * @param voltage Battery voltage
     * @param percentage Battery percentage
     * @param batteryStatus Battery status
     * @param signal GSM signal strength
     * @param operator_name GSM operator name
     * @param gsmStatus GSM connection status
     * @param latitude GPS latitude
     * @param longitude GPS longitude
     * @param altitude GPS altitude
     * @param satellites Number of satellites
     * @param systemStatus System status
     * @param currentTime Current time string (optional)
     * @param timeSync Time synchronization status (optional)
     * @return true if published successfully
     */
    bool publishDeviceData(float voltage, int percentage, const String &batteryStatus,
                           int signal, const String &operator_name, const String &gsmStatus,
                           float latitude, float longitude, float altitude, int satellites,
                           const String &systemStatus, unsigned long currentTimestamp = 0,
                           const String &timeSync = "");

    /**
     * @brief Publish critical alert/warning
     * @param alertType Type of alert (BATTERY_LOW, NETWORK_LOST, GPS_LOST, etc.)
     * @param severity Severity level (LOW, MEDIUM, HIGH, CRITICAL)
     * @param message Alert message
     * @param timestamp Alert timestamp (optional)
     * @return true if published successfully
     */
    bool publishAlert(const String &alertType, const String &severity, const String &message, unsigned long timestamp = 0);

    /**
     * @brief Disconnect from MQTT broker
     */
    void disconnect();

    /**
     * @brief Get connection status as string
     * @return Connection status string
     */
    String getConnectionStatus();

private:
    /**
     * @brief Helper function to print formatted JSON for debugging
     * @param doc JsonDocument to print
     * @param topic MQTT topic name
     */
    void printFormattedJson(const JsonDocument &doc, const String &topic);
};

#endif // MQTT_MANAGER_H
