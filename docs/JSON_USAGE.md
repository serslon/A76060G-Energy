# Использование ArduinoJson в A7670G Energy Monitor

## Обзор

Проект теперь использует библиотеку ArduinoJson для формирования JSON данных, что делает код более читаемым, надежным и менее подверженным ошибкам.

## Преимущества JsonDocument перед ручным формированием JSON

### ✅ **Было (ручное формирование):**
```cpp
String payload = "{";
payload += "\"device_id\":\"" + String(MQTT_CLIENT_ID) + "\",";
payload += "\"timestamp\":" + String(millis()) + ",";
payload += "\"voltage\":" + String(voltage, 2) + ",";
payload += "\"percentage\":" + String(percentage) + ",";
payload += "\"status\":\"" + status + "\"";
payload += "}";
```

### ✅ **Стало (ArduinoJson):**
```cpp
JsonDocument doc;
doc["device_id"] = MQTT_CLIENT_ID;
doc["timestamp"] = millis();
doc["voltage"] = round(voltage * 100) / 100.0;
doc["percentage"] = percentage;
doc["status"] = status;

String payload;
serializeJson(doc, payload);
```

## Ключевые улучшения

### 1. **Читаемость кода**
- Четкая структура данных
- Нет необходимости в ручном экранировании кавычек
- Автоматическое форматирование типов данных

### 2. **Безопасность**
- Автоматическое экранирование специальных символов
- Предотвращение синтаксических ошибок JSON
- Контроль типов данных

### 3. **Структурированные объекты**
```cpp
// Создание вложенных объектов
JsonObject battery = doc["battery"].to<JsonObject>();
battery["voltage"] = round(voltage * 100) / 100.0;
battery["percentage"] = percentage;
battery["status"] = batteryStatus;

JsonObject gsm = doc["gsm"].to<JsonObject>();
gsm["signal_strength"] = signal;
gsm["operator"] = operator_name;
gsm["status"] = gsmStatus;
```

### 4. **Точное округление чисел**
```cpp
// Округление до 2 знаков после запятой
doc["voltage"] = round(voltage * 100) / 100.0;

// Округление до 6 знаков для координат
doc["latitude"] = round(latitude * 1000000) / 1000000.0;

// Округление до 1 знака для температуры
doc["temperature"] = round(temperatureRead() * 10) / 10.0;
```

## Пример результирующего JSON

### Данные батареи:
```json
{
  "device_id": "A7670G_Energy_Monitor",
  "timestamp": 12345678,
  "voltage": 3.79,
  "percentage": 85,
  "status": "Good"
}
```

### Комплексные данные устройства:
```json
{
  "device_id": "A7670G_Energy_Monitor",
  "timestamp": 12345678,
  "device_type": "A7670G-Energy",
  "battery": {
    "voltage": 3.79,
    "percentage": 85,
    "status": "Good"
  },
  "gsm": {
    "signal_strength": -67,
    "operator": "Vodafone UA",
    "status": "Connected"
  },
  "gps": {
    "latitude": 50.450001,
    "longitude": 30.523400,
    "altitude": 179.50,
    "satellites": 8
  },
  "system": {
    "status": "Operational",
    "uptime": 12345678,
    "free_heap": 250000,
    "chip_temperature": 45.2
  }
}
```

## Отладочные возможности

### Включение детального вывода JSON
В `platformio.ini` раскомментируйте строку:
```ini
build_flags = ${esp32dev_base.build_flags}
            -DLILYGO_T_A7670
            -DDEBUG_MQTT_JSON  ; <-- Раскомментировать эту строку
```

### Форматированный вывод для отладки
```cpp
void MQTTManager::printFormattedJson(const JsonDocument& doc, const String& topic)
{
  Serial.printf("MQTT: Publishing to %s:\n", topic.c_str());
  serializeJsonPretty(doc, Serial);
  Serial.println();
}
```

## Используемые функции ArduinoJson

### Основные функции:
- `JsonDocument doc` - создание документа
- `doc["key"] = value` - добавление значений
- `doc["object"].to<JsonObject>()` - создание вложенных объектов
- `serializeJson(doc, string)` - сериализация в строку
- `serializeJsonPretty(doc, Serial)` - красивый вывод для отладки

### Типы данных:
- Строки автоматически экранируются
- Числа форматируются точно
- Булевы значения поддерживаются
- Массивы и объекты могут быть вложенными

## Производительность

- **Размер кода:** +4KB Flash (библиотека ArduinoJson)
- **RAM:** Минимальное увеличение использования
- **Скорость:** Сопоставимая с ручным формированием
- **Надежность:** Значительно выше

## Заключение

Использование ArduinoJson делает код более профессиональным, читаемым и надежным. Библиотека автоматически обрабатывает многие подводные камни JSON формирования и предоставляет чистый API для работы со структурированными данными.
