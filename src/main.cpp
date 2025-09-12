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
#include "oled_display.h"
#include "time_manager.h"

// Определяем тип модема перед включением TinyGSM
#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif

#include <TinyGsmClient.h>
#include "gps_monitor.h"
#include "Arduino.h"

// Глобальные объекты мониторинга
BatteryMonitor batteryMonitor(BOARD_BAT_ADC_PIN, 2.0); // GPIO35, коэффициент делителя 2.0
GSMMonitor gsmMonitor(&SerialAT);                      // Используем SerialAT для AT команд
TinyGsm modem(SerialAT);                               // TinyGSM для MQTT
MQTTManager mqttManager(&modem);                       // MQTT менеджер
GPSMonitor gpsMonitor(&modem);                         // Используем TinyGSM modem для GPS
TimeManager timeManager(&modem, 2);                    // Time manager с UTC+2 для Украины

// Переменные для таймеров
unsigned long lastMonitoringCheck = 0;
const unsigned long MONITORING_INTERVAL = 10000; // 10 секунд
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 2000; // 2 секунды
unsigned long displayMode = 0;                      // 0 - главный экран, 1 - сеть, 2 - GPS, 3 - батарея

bool checkRespond()
{
    for (int j = 0; j < 10; j++)
    {
        SerialAT.print("AT\r\n");
        String input = SerialAT.readString();
        if (input.indexOf("OK") >= 0)
        {
            return true;
        }
        delay(200);
    }
    return false;
}

uint32_t AutoBaud()
{
    static uint32_t rates[] = {115200, 9600, 57600, 38400, 19200, 74400, 74880,
                               230400, 460800, 2400, 4800, 14400, 28800};
    for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++)
    {
        uint32_t rate = rates[i];
        Serial.printf("Trying baud rate %u\n", rate);
        SerialAT.updateBaudRate(rate);
        delay(10);
        for (int j = 0; j < 10; j++)
        {
            SerialAT.print("AT\r\n");
            String input = SerialAT.readString();
            if (input.indexOf("OK") >= 0)
            {
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

    // Инициализация OLED дисплея
    if (oledDisplay.begin())
    {
        oledDisplay.updateStatusLine("Initializing system...");
    }
    else
    {
        Serial.println("OLED display initialization failed");
    }

    // Объекты мониторинга уже объявлены глобально
    // BatteryMonitor batteryMonitor(BOARD_BAT_ADC_PIN, 2.0);
    // GSMMonitor gsmMonitor(&SerialAT);
    // GPSMonitor gpsMonitor(&modem);  // Включаем GPS монитор с TinyGSM

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
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
    delay(2600);
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
    if (!started)
    {
        Serial.println("Wait modem started...");
        // Wait for the modem to finish booting
        delay(MODEM_START_WAIT_MS);
    }

    // Determine the communication baud rate
    if (AutoBaud())
    {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" You can now send AT commands"));
        Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
        Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
        Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
        Serial.println(F(" can have undesired consequences..."));
        Serial.println(F("***********************************************************\n"));

        // Инициализация GSM монитора после успешного подключения к модему
        Serial.println("Initializing GSM Monitor...");
        oledDisplay.updateStatusLine("Initializing GSM...");
        gsmMonitor.begin();
        gsmMonitor.printGSMInfo(); // Показать начальное состояние GSM

        // Ожидание подключения к сети
        Serial.println("Waiting for network connection...");
        oledDisplay.updateStatusLine("Connecting to network...");
        int attempts = 0;
        while (!gsmMonitor.isNetworkRegistered() && attempts < 30)
        {
            delay(1000);
            gsmMonitor.update();
            attempts++;
            Serial.printf("Network registration attempt %d/30\n", attempts);
            oledDisplay.updateStatusLine("Network attempt " + String(attempts) + "/30");
        }

        if (gsmMonitor.isNetworkRegistered())
        {
            Serial.println("Network connected successfully!");
            oledDisplay.updateStatusLine("Network connected!");
            oledDisplay.updateNetworkStatus(true);

            // Инициализация GPRS соединения
            Serial.println("Initializing GPRS connection...");
            oledDisplay.updateStatusLine("Connecting GPRS...");
            // Попробуем разные APN для украинских операторов
            bool gprsConnected = false;

            // Список возможных APN для украинских операторов
            const char *apns[] = {"internet", "www.ab.kyivstar.net", "3g.kyivstar.net", "internet.life.com.ua", "internet.vodafone.ua"};
            int apnCount = sizeof(apns) / sizeof(apns[0]);

            for (int i = 0; i < apnCount && !gprsConnected; i++)
            {
                Serial.printf("Trying APN: %s\n", apns[i]);
                if (modem.gprsConnect(apns[i], "", ""))
                {
                    if (modem.isNetworkConnected())
                    {
                        Serial.printf("GPRS connected successfully with APN: %s\n", apns[i]);
                        gprsConnected = true;
                        break;
                    }
                }
                delay(2000);
            }

            if (gprsConnected)
            {
                Serial.println("GPRS connected successfully!");
                oledDisplay.updateStatusLine("GPRS connected!");

                // Проверяем IP адрес
                IPAddress localIP = modem.localIP();
                Serial.printf("Local IP: %s\n", localIP.toString().c_str());
                oledDisplay.updateIP(localIP.toString());

                if (localIP == IPAddress(0, 0, 0, 0))
                {
                    Serial.println("No valid IP address received!");
                }
                else
                {
                    Serial.printf("Valid IP received: %s\n", localIP.toString().c_str());

                    // Тест подключения к интернету
                    Serial.println("Testing internet connectivity...");

                    // Попробуем подключиться к простому HTTP серверу для теста
                    TinyGsmClient client(modem);
                    if (client.connect("httpbin.org", 80))
                    {
                        Serial.println("Internet connectivity test: SUCCESS");
                        client.stop();

                        // Инициализация TimeManager для синхронизации времени
                        Serial.println("Initializing Time Manager...");
                        oledDisplay.updateStatusLine("Initializing time...");
                        if (timeManager.begin())
                        {
                            Serial.println("Time Manager initialized successfully");

                            // Выполняем начальную синхронизацию времени с NTP
                            Serial.println("Performing initial NTP synchronization...");
                            oledDisplay.updateStatusLine("Syncing time with NTP...");

                            if (timeManager.performInitialSync())
                            {
                                Serial.println("NTP synchronization successful!");
                                oledDisplay.updateStatusLine("Time synchronized!");
                                timeManager.printTimeInfo();
                            }
                            else
                            {
                                Serial.println("NTP synchronization failed, will retry later");
                                oledDisplay.updateStatusLine("Time sync failed");
                            }
                        }
                        else
                        {
                            Serial.println("Time Manager initialization failed");
                            oledDisplay.updateStatusLine("Time init failed");
                        }

                        // Инициализация MQTT менеджера
                        Serial.println("Initializing MQTT Manager...");
                        oledDisplay.updateStatusLine("Starting MQTT...");
                        mqttManager.begin();

                        // Отправляем уведомление о старте устройства
                        delay(2000); // Даем время на установку MQTT соединения
                        mqttManager.publishAlert("DEVICE_STARTUP", "INFO",
                                                 "A7670G Energy Monitor started successfully");
                        mqttManager.publishSystemStatus("Device started - all systems operational");
                    }
                    else
                    {
                        Serial.println("Internet connectivity test: FAILED");
                        Serial.println("Network may not have internet access");
                    }
                }
            }
            else
            {
                Serial.println("GPRS connection failed!");
            }
        }
        else
        {
            Serial.println("Network registration failed!");
        }

        // Инициализация GPS монитора
        Serial.println("Initializing GPS Monitor...");
        oledDisplay.updateStatusLine("Initializing GPS...");
        if (gpsMonitor.begin())
        {
            Serial.println("GPS Monitor initialized successfully");
            oledDisplay.updateStatusLine("GPS initialized");
        }
        else
        {
            Serial.println("GPS Monitor initialization failed - continuing without GPS");
            oledDisplay.updateStatusLine("GPS init failed");
        }

        // Финальная инициализация дисплея
        oledDisplay.updateStatusLine("System ready!");
        delay(2000);
        oledDisplay.showMainScreen();
    }
    else
    {
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

    // Обновление TimeManager (включая периодическую синхронизацию каждый час)
    timeManager.update();

    // Обновление дисплея
    unsigned long displayTime = millis();
    if (displayTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL)
    {
        // Обновляем данные дисплея
        float voltage = batteryMonitor.readVoltage();
        int signal = gsmMonitor.getSignalStrength();
        bool networkConnected = gsmMonitor.isNetworkRegistered();
        bool gpsFixed = gpsMonitor.isGPSValid();

        // Получаем текущее реальное время от TimeManager в формате для дисплея
        String currentTimeStr = timeManager.getDisplayTimeString();

        // Обновляем данные дисплея
        oledDisplay.updateBatteryVoltage(voltage);
        oledDisplay.updateSignalStrength(signal);
        oledDisplay.updateNetworkStatus(networkConnected);
        oledDisplay.updateGPSStatus(gpsFixed);
        oledDisplay.updateTime(currentTimeStr);

        // Показываем соответствующий экран
        switch (displayMode)
        {
        case 0:
            oledDisplay.showMainScreen();
            break;
        case 1:
            oledDisplay.showNetworkInfo();
            break;
        case 2:
            oledDisplay.showGPSInfo();
            break;
        case 3:
            oledDisplay.showBatteryInfo();
            break;
        default:
            oledDisplay.showMainScreen();
            displayMode = 0;
        }

        lastDisplayUpdate = displayTime;
    }

    // Обработка AT команд
    if (SerialAT.available())
    {
        Serial.write(SerialAT.read());
    }
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.equalsIgnoreCase("sleep"))
        {
            Serial.println("[CMD] Preparing ESP32 for deep sleep...");

            // Отправляем последнее сообщение в MQTT о переходе в спящий режим
            mqttManager.publishSystemStatus("Device entering deep sleep mode");
            delay(1000); // Дать время отправить сообщение

            // Отключаем GPS
            if (gpsMonitor.isGPSReady())
            {
                Serial.println("[CMD] Disabling GPS...");
                gpsMonitor.end();
            }

            // Переводим GSM модем в спящий режим (не отключаем полностью)
            Serial.println("[CMD] Setting GSM modem to sleep mode...");
            SerialAT.println("AT+CSCLK=2"); // Автоматический переход в сон
            delay(500);

            // Устанавливаем DTR в HIGH для перевода модема в сон
            digitalWrite(MODEM_DTR_PIN, HIGH);
            delay(100);

            Serial.println("[CMD] ESP32 entering deep sleep. Press RESET to wake up.");
            Serial.flush(); // Дождаться отправки всех данных в Serial

            // Переводим ESP32 в глубокий сон (бесконечный, пробуждение только по RESET)
            esp_deep_sleep_start();
        }
        else if (cmd.equalsIgnoreCase("wake"))
        {
            Serial.println("[CMD] Waking up GSM modem...");
            digitalWrite(MODEM_DTR_PIN, LOW); // Пробуждаем модем
            delay(100);
            SerialAT.println("AT"); // Проверяем связь
        }
        else if (cmd.equalsIgnoreCase("screen"))
        {
            displayMode = (displayMode + 1) % 4;
            String screens[] = {"Main", "Network", "GPS", "Battery"};
            Serial.println("[CMD] Switched to " + screens[displayMode] + " screen");
            oledDisplay.showMessage("Screen: " + screens[displayMode], 1000);
        }
        else if (cmd.equalsIgnoreCase("display"))
        {
            Serial.println("[CMD] Display status:");
            Serial.println("  Current screen: " + String(displayMode));
            Serial.println("  Display initialized: " + String(oledDisplay.isInitialized()));
            Serial.println("  Commands: 'screen' - switch screen, 'test' - test display");
        }
        else if (cmd.equalsIgnoreCase("test"))
        {
            Serial.println("[CMD] Testing display...");
            oledDisplay.showMessage("Display Test", 1000);
            oledDisplay.showSuccess("Test Successful!");
            delay(1000);
        }
        else if (cmd.equalsIgnoreCase("time"))
        {
            Serial.println("[CMD] Time information:");
            timeManager.printTimeInfo();
        }
        else if (cmd.equalsIgnoreCase("synctime"))
        {
            Serial.println("[CMD] Forcing time synchronization...");
            oledDisplay.showMessage("Syncing time...", 500);
            if (timeManager.forceSync())
            {
                Serial.println("[CMD] Time synchronization successful!");
                oledDisplay.showSuccess("Time synced!");
                timeManager.printTimeInfo();
            }
            else
            {
                Serial.println("[CMD] Time synchronization failed!");
                oledDisplay.showError("Time sync failed!");
            }
        }
        else if (cmd.equalsIgnoreCase("updatetime"))
        {
            Serial.println("[CMD] Forcing time update from modem...");
            oledDisplay.showMessage("Updating time...", 500);
            if (timeManager.forceTimeUpdate())
            {
                Serial.println("[CMD] Time update successful!");
                oledDisplay.showSuccess("Time updated!");
                timeManager.printTimeInfo();
            }
            else
            {
                Serial.println("[CMD] Time update failed!");
                oledDisplay.showError("Time update failed!");
            }
        }
        else if (cmd.equalsIgnoreCase("diagtime"))
        {
            Serial.println("[CMD] Diagnosing time issues...");
            Serial.println("=== TIME DIAGNOSIS ===");

            // Check network registration
            if (!gsmMonitor.isNetworkRegistered())
            {
                Serial.println("❌ PROBLEM: Not registered on cellular network");
                Serial.println("   SOLUTION: Wait for network registration or check SIM card");
            }
            else
            {
                Serial.println("✅ Cellular network is connected");
            }

            // Check internet connectivity
            if (modem.isNetworkConnected())
            {
                Serial.println("✅ GPRS/Internet connection is active");
            }
            else
            {
                Serial.println("❌ PROBLEM: No internet connection");
                Serial.println("   SOLUTION: Check GPRS/APN settings");
            }

            // Show current time status
            timeManager.printTimeInfo();

            // Try raw time query
            Serial.println("\n=== RAW TIME QUERY ===");
            int year, month, day, hour, minute, second;
            float timezone;
            if (modem.getNetworkTime(&year, &month, &day, &hour, &minute, &second, &timezone))
            {
                Serial.printf("Raw modem time: %04d-%02d-%02d %02d:%02d:%02d TZ:%.1f\n",
                              year, month, day, hour, minute, second, timezone);

                if (year > 2050)
                {
                    Serial.println("❌ PROBLEM: Modem clock shows future year - not synced with network");
                    Serial.println("   SOLUTION: Try 'inittime' command to initialize modem time settings");
                }
                else if (year < 2025)
                {
                    Serial.println("❌ PROBLEM: Modem clock shows past year - clock reset or no battery");
                    Serial.println("   SOLUTION: Use 'inittime' or 'synctime' command");
                }
                else
                {
                    Serial.println("✅ Modem time appears reasonable");
                }
            }
            else
            {
                Serial.println("❌ PROBLEM: Cannot get time from modem");
                Serial.println("   SOLUTION: Check AT commands or modem communication");
            }

            Serial.println("=== END DIAGNOSIS ===");
        }
        else if (cmd.equalsIgnoreCase("inittime"))
        {
            Serial.println("[CMD] Initializing modem time settings with proper AT sequence...");
            oledDisplay.showMessage("Init modem time...", 500);
            if (timeManager.initializeModemTime())
            {
                Serial.println("[CMD] Modem time initialization successful!");
                oledDisplay.showSuccess("Modem time init OK!");

                // Try to get time after initialization
                delay(1000);
                if (timeManager.forceTimeUpdate(3))
                {
                    Serial.println("[CMD] Time successfully updated after initialization!");
                    timeManager.printTimeInfo();
                }
                else
                {
                    Serial.println("[CMD] Time initialization completed but time still invalid");
                }
            }
            else
            {
                Serial.println("[CMD] Modem time initialization failed!");
                oledDisplay.showError("Init failed!");
            }
        }
        else if (cmd.startsWith("timezone "))
        {
            int tz = cmd.substring(9).toInt();
            if (tz >= -12 && tz <= 14)
            {
                timeManager.setTimezone(tz);
                Serial.printf("[CMD] Timezone set to UTC%+d\n", tz);
                oledDisplay.showMessage("Timezone: UTC" + String(tz >= 0 ? "+" : "") + String(tz), 1000);
            }
            else
            {
                Serial.println("[CMD] Invalid timezone. Range: -12 to +14");
            }
        }
        else
        {
            SerialAT.println(cmd);
        }
    }

    // Мониторинг батареи и GSM каждые 10 секунд (GPS отключен)
    unsigned long monitoringTime = millis();
    if (monitoringTime - lastMonitoringCheck >= MONITORING_INTERVAL)
    {
        Serial.println("\n=== MONITORING UPDATE ===");

        // Выводим информацию о батарее
        batteryMonitor.printBatteryShort();

        // Выводим информацию о GSM
        gsmMonitor.printGSMShort();

        // Обновляем и выводим информацию о GPS
        gpsMonitor.update();
        gpsMonitor.printGPSShort();

        // Публикуем данные в MQTT
        Serial.printf("[MQTT] Connection status: %s\n", mqttManager.getConnectionStatus().c_str());

        // Собираем все данные
        float voltage = batteryMonitor.readVoltage();
        int percentage = (int)batteryMonitor.getBatteryPercentage();
        String batteryStatus = batteryMonitor.getBatteryStatus();

        int signal = gsmMonitor.getSignalStrength();
        String operatorName = gsmMonitor.getOperator();
        String gsmStatus = gsmMonitor.getNetworkStatus();

        // GPS данные (теперь реальные)
        float latitude = gpsMonitor.getLatitude();
        float longitude = gpsMonitor.getLongitude();
        float altitude = gpsMonitor.getAltitude();
        int satellites = gpsMonitor.getVisibleSatellites();

        String systemStatus = "Operational";

        // Получаем данные времени
        unsigned long currentTimestamp = timeManager.getUnixTimestamp();
        String timeSyncStatus = timeManager.getSyncStatus();

        // Отправляем комплексные данные устройства в одном JSON объекте
        mqttManager.publishDeviceData(voltage, percentage, batteryStatus,
                                      signal, operatorName, gsmStatus,
                                      latitude, longitude, altitude, satellites,
                                      systemStatus, currentTimestamp, timeSyncStatus);

        // Также отправляем отдельные данные батареи для совместимости
        mqttManager.publishBatteryData(voltage, percentage, batteryStatus, currentTimestamp);

        lastMonitoringCheck = monitoringTime;

        // Проверка критических состояний
        if (batteryMonitor.isLowBattery())
        {
            Serial.println("⚠️  WARNING: Battery level is critically low!");
            mqttManager.publishAlert("BATTERY_LOW", "HIGH",
                                     "Battery voltage: " + String(voltage, 2) + "V (" +
                                         String(percentage) + "%)",
                                     currentTimestamp);
        }

        if (!gsmMonitor.isNetworkRegistered())
        {
            Serial.println("⚠️  WARNING: Not registered in GSM network!");
            mqttManager.publishAlert("NETWORK_LOST", "CRITICAL",
                                     "Device lost GSM network registration", currentTimestamp);
        }

        // GPS проверка
        if (!gpsMonitor.isGPSValid())
        {
            Serial.println("⚠️  INFO: GPS fix not available");
            // Отправляем алерт только если GPS был включен, но потерял фикс
            if (gpsMonitor.isGPSReady() && gpsMonitor.getLastUpdateTime() > 0)
            {
                mqttManager.publishAlert("GPS_LOST", "LOW", "GPS fix not available", currentTimestamp);
            }
        }

        Serial.println("========================\n");
    }

    delay(100); // Небольшая задержка для стабильности
}
