/**
 * @file      main.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xian LILYGO Technology Co., Ltd.
 * @date      2023-10-26
 * @note      This is the main project file for A7670G Energy Project with Battery, GSM Monitoring and MQTT (GPS disabled)
 */
#include "utilities.h"
#include "battery_monitor.h"
#include "gsm_monitor.h"
#include "mqtt_manager.h"

// Определяем тип модема перед включением TinyGSM
#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif

#include <TinyGsmClient.h>
// #include "gps_monitor.h"  // Отключено временно
#include "Arduino.h"

// Глобальные объекты мониторинга
BatteryMonitor batteryMonitor(BOARD_BAT_ADC_PIN, 2.0); // GPIO35, коэффициент делителя 2.0
GSMMonitor gsmMonitor(&SerialAT); // Используем SerialAT для AT команд
TinyGsm modem(SerialAT); // TinyGSM для MQTT
MQTTManager mqttManager(&modem); // MQTT менеджер
// GPSMonitor gps(&SerialAT); // Используем SerialAT для GPS команд (отключено)

// Переменные для таймеров
unsigned long lastMonitoringCheck = 0;
const unsigned long MONITORING_INTERVAL = 10000; // 10 секунд

bool checkRespond()
{
    for (int j = 0; j < 10; j++) {
        SerialAT.print("AT\r\n");
        String input = SerialAT.readString();
        if (input.indexOf("OK") >= 0) {
            return true;
        }
        delay(200);
    }
    return false;
}

uint32_t AutoBaud()
{
    static uint32_t rates[] = {115200, 9600, 57600,  38400, 19200,  74400, 74880,
                               230400, 460800, 2400,  4800,  14400, 28800
                              };
    for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
        uint32_t rate = rates[i];
        Serial.printf("Trying baud rate %u\n", rate);
        SerialAT.updateBaudRate(rate);
        delay(10);
        for (int j = 0; j < 10; j++) {
            SerialAT.print("AT\r\n");
            String input = SerialAT.readString();
            if (input.indexOf("OK") >= 0) {
                Serial.printf("Modem responded at rate:%u\n", rate);
                return rate;
            }
        }
    }
    SerialAT.updateBaudRate(115200);
    return 0;
}

void setup()
{
    Serial.begin(115200); // Set console baud rate

    Serial.println("Start A7670G Energy Project with Battery and GSM Monitoring");
    
    // Объекты мониторинга уже объявлены глобально
    // BatteryMonitor batteryMonitor(BOARD_BAT_ADC_PIN, 2.0);
    // GSMMonitor gsmMonitor(&SerialAT);
    // GPSMonitor gpsMonitor(&SerialAT);  // Отключено временно

    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

#ifdef BOARD_LED_PIN
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, !LED_ON);
#endif

#ifdef BOARD_POWERON_PIN
    /* Set Power control pin output
    * * @note      Known issues, ESP32 (V1.2) version of T-A7670, T-A7608,
    *            when using battery power supply mode, BOARD_POWERON_PIN (IO12) must be set to high level after esp32 starts, otherwise a reset will occur.
    * */
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

    // Set modem reset pin ,reset modem
#ifdef MODEM_RESET_PIN
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL); delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL); delay(2600);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
#endif

#ifdef MODEM_FLIGHT_PIN
    // If there is an airplane mode control, you need to exit airplane mode
    pinMode(MODEM_FLIGHT_PIN, OUTPUT);
    digitalWrite(MODEM_FLIGHT_PIN, HIGH);
#endif

    // Pull down DTR to ensure the modem is not in sleep state
    pinMode(MODEM_DTR_PIN, OUTPUT);
    digitalWrite(MODEM_DTR_PIN, LOW);


    // Turn on the modem
    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(MODEM_POWERON_PULSE_WIDTH_MS);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);

    // Check whether it has been started
    bool started = checkRespond();
    if (!started) {
        Serial.println("Wait modem started...");
        // Wait for the modem to finish booting
        delay(MODEM_START_WAIT_MS);
    }

    // Determine the communication baud rate
    if (AutoBaud()) {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" You can now send AT commands"));
        Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        Serial.println(F(" can have undesired consequences..."));
        Serial.println(F("***********************************************************\n"));
        
        // Инициализация GSM монитора после успешного подключения к модему
        Serial.println("Initializing GSM Monitor...");
        gsmMonitor.begin();
        gsmMonitor.printGSMInfo(); // Показать начальное состояние GSM
        
        // Ожидание подключения к сети
        Serial.println("Waiting for network connection...");
        int attempts = 0;
        while (!gsmMonitor.isNetworkRegistered() && attempts < 30) {
            delay(1000);
            gsmMonitor.update();
            attempts++;
            Serial.printf("Network registration attempt %d/30\n", attempts);
        }
        
        if (gsmMonitor.isNetworkRegistered()) {
            Serial.println("Network connected successfully!");
            
            // Инициализация GPRS соединения
            Serial.println("Initializing GPRS connection...");
            // Попробуем разные APN для украинских операторов
            bool gprsConnected = false;
            
            // Список возможных APN для украинских операторов
            const char* apns[] = {"internet", "www.ab.kyivstar.net", "3g.kyivstar.net", "internet.life.com.ua", "internet.vodafone.ua"};
            int apnCount = sizeof(apns) / sizeof(apns[0]);
            
            for (int i = 0; i < apnCount && !gprsConnected; i++) {
                Serial.printf("Trying APN: %s\n", apns[i]);
                if (modem.gprsConnect(apns[i], "", "")) {
                    if (modem.isNetworkConnected()) {
                        Serial.printf("GPRS connected successfully with APN: %s\n", apns[i]);
                        gprsConnected = true;
                        break;
                    }
                }
                delay(2000);
            }
            
            if (gprsConnected) {
                Serial.println("GPRS connected successfully!");
                
                // Проверяем IP адрес
                IPAddress localIP = modem.localIP();
                Serial.printf("Local IP: %s\n", localIP.toString().c_str());
                
                if (localIP == IPAddress(0, 0, 0, 0)) {
                    Serial.println("No valid IP address received!");
                } else {
                    Serial.printf("Valid IP received: %s\n", localIP.toString().c_str());
                    
                    // Тест подключения к интернету
                    Serial.println("Testing internet connectivity...");
                    
                    // Попробуем подключиться к простому HTTP серверу для теста
                    TinyGsmClient client(modem);
                    if (client.connect("httpbin.org", 80)) {
                        Serial.println("Internet connectivity test: SUCCESS");
                        client.stop();
                        
                        // Инициализация MQTT менеджера
                        Serial.println("Initializing MQTT Manager...");
                        mqttManager.begin();
                        
                        // Отправляем уведомление о старте устройства
                        delay(2000); // Даем время на установку MQTT соединения
                        mqttManager.publishAlert("DEVICE_STARTUP", "INFO", 
                                               "A7670G Energy Monitor started successfully");
                        mqttManager.publishSystemStatus("Device started - all systems operational");
                    } else {
                        Serial.println("Internet connectivity test: FAILED");
                        Serial.println("Network may not have internet access");
                    }
                }
            } else {
                Serial.println("GPRS connection failed!");
            }
        } else {
            Serial.println("Network registration failed!");
        }
        
        // Инициализация GPS монитора (отключено)
        // Serial.println("Initializing GPS Monitor...");
        // gpsMonitor.begin();
        // gpsMonitor.printGPSInfo(); // Показать начальное состояние GPS
        
    } else {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
        Serial.println(F("***********************************************************\n"));
    }


#ifdef BOARD_LED_PIN
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, LED_ON);
#endif

}

void loop()
{
    // Обработка MQTT
    mqttManager.handle();
    
    // Обработка AT команд
    if (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    if (Serial.available()) {
        SerialAT.write(Serial.read());
    }
    
    // Мониторинг батареи и GSM каждые 10 секунд (GPS отключен)
    unsigned long currentTime = millis();
    if (currentTime - lastMonitoringCheck >= MONITORING_INTERVAL) {
        Serial.println("\n=== MONITORING UPDATE ===");
        
        // Выводим информацию о батарее
        batteryMonitor.printBatteryShort();
        
        // Выводим информацию о GSM
        gsmMonitor.printGSMShort();
        
        // Публикуем данные в MQTT
        Serial.printf("[MQTT] Connection status: %s\n", mqttManager.getConnectionStatus().c_str());
        
        // Собираем все данные
        float voltage = batteryMonitor.readVoltage();
        int percentage = (int)batteryMonitor.getBatteryPercentage();
        String batteryStatus = batteryMonitor.getBatteryStatus();
        
        int signal = gsmMonitor.getSignalStrength();
        String operatorName = gsmMonitor.getOperator();
        String gsmStatus = gsmMonitor.getNetworkStatus();
        
        // GPS данные (заглушка, поскольку GPS отключен)
        float latitude = 0.0;
        float longitude = 0.0;
        float altitude = 0.0;
        int satellites = 0;
        
        String systemStatus = "Operational";
        
        // Отправляем комплексные данные устройства в одном JSON объекте
        mqttManager.publishDeviceData(voltage, percentage, batteryStatus,
                                     signal, operatorName, gsmStatus,
                                     latitude, longitude, altitude, satellites,
                                     systemStatus);
        
        // Также отправляем отдельные данные батареи для совместимости
        mqttManager.publishBatteryData(voltage, percentage, batteryStatus);
        
        // Выводим информацию о GPS (отключено)
        // gps.printGPSShort();
        
        lastMonitoringCheck = currentTime;
        
        // Проверка критических состояний
        if (batteryMonitor.isLowBattery()) {
            Serial.println("⚠️  WARNING: Battery level is critically low!");
            mqttManager.publishAlert("BATTERY_LOW", "HIGH", 
                                   "Battery voltage: " + String(voltage, 2) + "V (" + 
                                   String(percentage) + "%)");
        }
        
        if (!gsmMonitor.isNetworkRegistered()) {
            Serial.println("⚠️  WARNING: Not registered in GSM network!");
            mqttManager.publishAlert("NETWORK_LOST", "CRITICAL", 
                                   "Device lost GSM network registration");
        }
        
        // GPS проверка отключена
        // if (!gps.isGPSValid()) {
        //     Serial.println("⚠️  INFO: GPS fix not available");
        //     mqttManager.publishAlert("GPS_LOST", "LOW", "GPS fix not available");
        // }
        
        Serial.println("========================\n");
    }
    
    delay(100); // Небольшая задержка для стабильности
}
