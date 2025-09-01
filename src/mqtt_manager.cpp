/**
 * @file      mqtt_manager.cpp
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     MQTT manager implementation for A7670G Energy Project with HiveMQ Cloud
 */

#include "mqtt_manager.h"

// HiveMQ Cloud Root CA Certificate (ISRG Root X1)
const char *HivemqRootCA =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/\n"
    "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
    "DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow\n"
    "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
    "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB\n"
    "AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC\n"
    "ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL\n"
    "wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D\n"
    "LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK\n"
    "4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5\n"
    "bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y\n"
    "sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ\n"
    "Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4\n"
    "FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc\n"
    "SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql\n"
    "PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND\n"
    "TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw\n"
    "SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1\n"
    "c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx\n"
    "+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB\n"
    "ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu\n"
    "b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E\n"
    "U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu\n"
    "MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC\n"
    "5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW\n"
    "9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG\n"
    "WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O\n"
    "he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC\n"
    "Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5\n"
    "-----END CERTIFICATE-----";

MQTTManager::MQTTManager(TinyGsm *gsm)
    : modem(gsm), isConnected(false), isInitialized(false),
      lastConnectAttempt(0), lastKeepAlive(0)
{
    Serial.println("[MQTT] MQTT Manager created");
}

bool MQTTManager::begin()
{
  if (!modem)
  {
    Serial.println("MQTT: Error - modem not initialized");
    return false;
  }

  Serial.println("MQTT: Initializing MQTT manager...");

  // Initialize MQTT with SSL and SNI enabled
  if (!modem->mqtt_begin(true, true))
  {
    Serial.println("MQTT: Failed to initialize MQTT");
    return false;
  }

  // Set the SSL certificate
  Serial.println("MQTT: Setting SSL certificate...");
  modem->mqtt_set_certificate(HivemqRootCA);

  isInitialized = true;
  Serial.println("MQTT: MQTT manager initialized successfully");
  return true;
}

bool MQTTManager::connectToMQTT()
{
  if (!isInitialized)
  {
    Serial.println("MQTT: Manager not initialized");
    return false;
  }

  Serial.println("MQTT: Connecting to HiveMQ Cloud...");
  Serial.printf("MQTT: Broker: %s:%d\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);

  // Attempt to connect to MQTT broker (clientIndex = 0)
  if (!modem->mqtt_connect(0, MQTT_BROKER_HOST, MQTT_BROKER_PORT,
                           MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.println("MQTT: HiveMQ Cloud connection failed");
    return false;
  }

  Serial.println("MQTT: Connected successfully to HiveMQ Cloud!");
  isConnected = true;
  lastKeepAlive = millis();
  return true;
}

bool MQTTManager::isConnectedToMQTT()
{
  if (!isInitialized)
    return false;

  // Check connection status
  if (!modem->mqtt_connected())
  {
    if (isConnected)
    {
      Serial.println("MQTT: Connection lost");
      isConnected = false;
    }

    // Try to reconnect if enough time has passed
    if (millis() - lastConnectAttempt > MQTT_RECONNECT_INTERVAL)
    {
      lastConnectAttempt = millis();
      return connectToMQTT();
    }
    return false;
  }

  isConnected = true;
  return true;
}

void MQTTManager::handle()
{
  if (!isInitialized || !isConnected)
    return;

  // Handle MQTT messages
  modem->mqtt_handle();

  // Send keep-alive if needed
  if (millis() - lastKeepAlive > MQTT_KEEP_ALIVE_INTERVAL)
  {
    lastKeepAlive = millis();
    publishSystemStatus("online");
  }
}

String MQTTManager::createPayload(float value)
{
  return "{\"value\":" + String(value, 2) + "}";
}

String MQTTManager::createPayload(int value)
{
  return "{\"value\":" + String(value) + "}";
}

String MQTTManager::createPayload(const String &value)
{
  return "{\"value\":\"" + value + "\"}";
}

String MQTTManager::getTopicForParameter(MQTTParameterType paramType)
{
  switch (paramType)
  {
  case MQTT_PARAM_BATTERY_VOLTAGE:
    return "A7670/battery/voltage";
  case MQTT_PARAM_BATTERY_PERCENTAGE:
    return "A7670/battery/percentage";
  case MQTT_PARAM_BATTERY_STATUS:
    return "A7670/battery/status";
  case MQTT_PARAM_GSM_SIGNAL:
    return "A7670/gsm/signal";
  case MQTT_PARAM_GSM_OPERATOR:
    return "A7670/gsm/operator";
  case MQTT_PARAM_GSM_STATUS:
    return "A7670/gsm/status";
  case MQTT_PARAM_GPS_LATITUDE:
    return "A7670/gps/latitude";
  case MQTT_PARAM_GPS_LONGITUDE:
    return "A7670/gps/longitude";
  case MQTT_PARAM_GPS_ALTITUDE:
    return "A7670/gps/altitude";
  case MQTT_PARAM_GPS_SATELLITES:
    return "A7670/gps/satellites";
  case MQTT_PARAM_SYSTEM_STATUS:
    return "A7670/status";
  default:
    return "A7670/unknown";
  }
}

bool MQTTManager::publishParameter(MQTTParameterType paramType, float value)
{
  if (!isConnectedToMQTT())
    return false;

  String topic = getTopicForParameter(paramType);
  String payload = createPayload(value);

  Serial.printf("MQTT: Publishing to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishParameter(MQTTParameterType paramType, int value)
{
  if (!isConnectedToMQTT())
    return false;

  String topic = getTopicForParameter(paramType);
  String payload = createPayload(value);

  Serial.printf("MQTT: Publishing to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishParameter(MQTTParameterType paramType, const String &value)
{
  if (!isConnectedToMQTT())
    return false;

  String topic = getTopicForParameter(paramType);
  String payload = createPayload(value);

  Serial.printf("MQTT: Publishing to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishBatteryData(float voltage, int percentage, const String &status)
{
  if (!isConnectedToMQTT())
    return false;

  // Создаем JSON документ для данных батареи
  JsonDocument doc;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["timestamp"] = millis();
  doc["voltage"] = round(voltage * 100) / 100.0; // Округляем до 2 знаков
  doc["percentage"] = percentage;
  doc["status"] = status;

  // Сериализуем JSON в строку
  String payload;
  serializeJson(doc, payload);

  String topic = MQTT_TOPIC_BATTERY;
  Serial.printf("MQTT: Publishing battery data to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishGSMData(int signal, const String &operator_name, const String &status)
{
  if (!isConnectedToMQTT())
    return false;

  // Создаем JSON документ для данных GSM
  JsonDocument doc;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["timestamp"] = millis();
  doc["signal_strength"] = signal;
  doc["operator"] = operator_name;
  doc["status"] = status;

  // Сериализуем JSON в строку
  String payload;
  serializeJson(doc, payload);

  String topic = MQTT_TOPIC_GSM;
  Serial.printf("MQTT: Publishing GSM data to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishGPSData(float latitude, float longitude, float altitude, int satellites)
{
  if (!isConnectedToMQTT())
    return false;

  // Создаем JSON документ для данных GPS
  JsonDocument doc;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["timestamp"] = millis();
  doc["latitude"] = round(latitude * 1000000) / 1000000.0; // Округляем до 6 знаков
  doc["longitude"] = round(longitude * 1000000) / 1000000.0;
  doc["altitude"] = round(altitude * 100) / 100.0; // Округляем до 2 знаков
  doc["satellites"] = satellites;

  // Сериализуем JSON в строку
  String payload;
  serializeJson(doc, payload);

  String topic = MQTT_TOPIC_GPS;
  Serial.printf("MQTT: Publishing GPS data to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishSystemStatus(const String &status)
{
  if (!isConnectedToMQTT())
    return false;

  // Создаем JSON документ для общего статуса системы
  JsonDocument doc;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["timestamp"] = millis();
  doc["status"] = status;
  doc["uptime"] = millis();

  // Сериализуем JSON в строку
  String payload;
  serializeJson(doc, payload);

  String topic = MQTT_TOPIC_STATUS;
  Serial.printf("MQTT: Publishing system status to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

void MQTTManager::disconnect()
{
  if (isConnected && modem)
  {
    Serial.println("MQTT: Disconnecting...");
    modem->mqtt_disconnect();
    isConnected = false;
  }
}

bool MQTTManager::publishDeviceData(float voltage, int percentage, const String& batteryStatus,
                                   int signal, const String& operator_name, const String& gsmStatus,
                                   float latitude, float longitude, float altitude, int satellites,
                                   const String& systemStatus)
{
  if (!isConnectedToMQTT())
    return false;

  // Создаем комплексный JSON документ со всеми данными устройства
  JsonDocument doc;
  
  // Основная информация устройства
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["timestamp"] = millis();
  doc["device_type"] = "A7670G-Energy";
  
  // Данные батареи
  JsonObject battery = doc["battery"].to<JsonObject>();
  battery["voltage"] = round(voltage * 100) / 100.0;
  battery["percentage"] = percentage;
  battery["status"] = batteryStatus;
  
  // Данные GSM
  JsonObject gsm = doc["gsm"].to<JsonObject>();
  gsm["signal_strength"] = signal;
  gsm["operator"] = operator_name;
  gsm["status"] = gsmStatus;
  
  // Данные GPS
  JsonObject gps = doc["gps"].to<JsonObject>();
  gps["latitude"] = round(latitude * 1000000) / 1000000.0;
  gps["longitude"] = round(longitude * 1000000) / 1000000.0;
  gps["altitude"] = round(altitude * 100) / 100.0;
  gps["satellites"] = satellites;
  
  // Системные данные
  JsonObject system = doc["system"].to<JsonObject>();
  system["status"] = systemStatus;
  system["uptime"] = millis();
  system["free_heap"] = ESP.getFreeHeap();
  system["chip_temperature"] = round(temperatureRead() * 10) / 10.0;

  // Сериализуем JSON в строку
  String payload;
  serializeJson(doc, payload);

  String topic = MQTT_TOPIC_DEVICE;
  
  // Выводим красиво отформатированный JSON для отладки
  #ifdef DEBUG_MQTT_JSON
  printFormattedJson(doc, topic);
  #else
  Serial.printf("MQTT: Publishing complete device data to %s\n", topic.c_str());
  #endif
  
  Serial.printf("Payload size: %d bytes\n", payload.length());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

bool MQTTManager::publishAlert(const String& alertType, const String& severity, const String& message)
{
  if (!isConnectedToMQTT())
    return false;

  // Создаем JSON документ для алерта
  JsonDocument doc;
  doc["device_id"] = MQTT_CLIENT_ID;
  doc["timestamp"] = millis();
  doc["alert_type"] = alertType;
  doc["severity"] = severity;
  doc["message"] = message;
  doc["device_type"] = "A7670G-Energy";

  // Сериализуем JSON в строку
  String payload;
  serializeJson(doc, payload);

  String topic = MQTT_TOPIC_ALERTS;
  Serial.printf("MQTT: Publishing ALERT to %s: %s\n", topic.c_str(), payload.c_str());

  return modem->mqtt_publish(0, topic.c_str(), payload.c_str());
}

String MQTTManager::getConnectionStatus()
{
  if (!isInitialized)
    return "Not Initialized";
  if (!isConnected)
    return "Disconnected";
  return "Connected";
}

void MQTTManager::printFormattedJson(const JsonDocument& doc, const String& topic)
{
  Serial.printf("MQTT: Publishing to %s:\n", topic.c_str());
  serializeJsonPretty(doc, Serial);
  Serial.println();
}
