# MQTT Integration для A7670G Energy Monitor

## Обзор

Проект теперь включает MQTT интеграцию для отправки данных мониторинга на HiveMQ Cloud.

## Настройки MQTT

### Конфигурация HiveMQ Cloud
- **Broker**: `185193e761bb414c939c1b05bed985de.s1.eu.hivemq.cloud:8883`
- **Username**: `A7670G_Client`  
- **Password**: `hngoAAHpKI2rDQ`
- **Client ID**: `A7670G_Energy_Monitor`
- **Протокол**: MQTTS (MQTT over SSL/TLS)

### Структура топиков

Все данные публикуются в топики с базовым префиксом `A7670/`:

#### Данные батареи:
- `A7670/battery/voltage` - Напряжение батареи (В)
- `A7670/battery/percentage` - Заряд батареи (%)
- `A7670/battery/status` - Статус батареи (строка)

#### Данные GSM:
- `A7670/gsm/signal` - Качество сигнала (dBm)
- `A7670/gsm/operator` - Оператор сети
- `A7670/gsm/status` - Статус сети

#### Данные GPS (когда включены):
- `A7670/gps/latitude` - Широта
- `A7670/gps/longitude` - Долгота
- `A7670/gps/altitude` - Высота (м)
- `A7670/gps/satellites` - Количество спутников

#### Системные данные:
- `A7670/status` - Общий статус системы

### Формат сообщений

Все сообщения публикуются в формате JSON:
```json
{
  "value": <значение>
}
```

### Примеры сообщений

```json
// Напряжение батареи
{
  "value": 3.42
}

// Процент заряда
{
  "value": 35
}

// Статус батареи
{
  "value": "Low"
}

// Качество сигнала GSM
{
  "value": 21
}

// Оператор
{
  "value": "25503"
}
```

## Интеграция в коде

### MQTTManager класс

Основной класс для работы с MQTT находится в файлах:
- `src/mqtt_manager.h` - Заголовочный файл
- `src/mqtt_manager.cpp` - Реализация

### Основные методы:

```cpp
// Инициализация
bool begin();

// Публикация отдельных параметров
bool publishParameter(MQTTParameterType paramType, float value);
bool publishParameter(MQTTParameterType paramType, int value);
bool publishParameter(MQTTParameterType paramType, const String& value);

// Публикация групп данных
bool publishBatteryData(float voltage, int percentage, const String& status);
bool publishGSMData(int signal, const String& operator_name, const String& status);
bool publishGPSData(float latitude, float longitude, float altitude, int satellites);

// Обработка MQTT (вызывать в loop)
void handle();

// Проверка соединения
bool isConnectedToMQTT();
```

## Безопасность

- Используется SSL/TLS шифрование
- Сертификат ISRG Root X1 для проверки подлинности сервера
- Аутентификация по имени пользователя и паролю

## Интервалы передачи

- **Данные мониторинга**: каждые 10 секунд
- **Keep-alive**: каждые 60 секунд
- **Переподключение**: каждые 10 секунд при разрыве соединения

## Отладка

MQTT менеджер выводит детальную информацию в Serial порт:
- Статус подключения
- Публикуемые сообщения  
- Ошибки соединения
- Переподключения

## Зависимости

- TinyGSM библиотека (форк от LewisXhe)
- A7670G модем с поддержкой SSL/TLS
- Стабильное GSM соединение для MQTT

## Примеры использования

```cpp
// В setup()
mqttManager.begin();

// В loop()
mqttManager.handle();

// Публикация данных
mqttManager.publishBatteryData(3.42, 35, "Low");
mqttManager.publishGSMData(21, "25503", "Connected");
```

## Мониторинг данных

Данные можно отслеживать через:
1. HiveMQ Cloud Console
2. MQTT клиенты (MQTT Explorer, mosquitto_sub)
3. Пользовательские приложения

Пример подписки через mosquitto:
```bash
mosquitto_sub -h 185193e761bb414c939c1b05bed985de.s1.eu.hivemq.cloud \
              -p 8883 \
              -u A7670G_Client \
              -P hngoAAHpKI2rDQ \
              --capath /etc/ssl/certs \
              -t "A7670/+"
```
