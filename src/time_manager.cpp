/**
 * @file      time_manager.cpp
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     Time manager implementation with NTP synchronization for A7670G Energy Project
 */

#include "time_manager.h"

TimeManager::TimeManager(TinyGsm* gsm, int tz_offset, unsigned long sync_interval_ms) 
    : modem(gsm), last_sync_attempt(0), sync_interval(sync_interval_ms), 
      time_synchronized(false), timezone_offset(tz_offset),
      ntp_server(DEFAULT_NTP_SERVER), backup_ntp_server(BACKUP_NTP_SERVER) {
    
    // Initialize time data structure
    current_time = {0, 0, 0, 0, 0, 0, 0.0, false, 0, "Not synchronized"};
    
    Serial.println("[TIME] Time Manager initialized");
    Serial.printf("[TIME] Timezone: UTC%+d, Sync interval: %lu ms\n", 
                  timezone_offset, sync_interval);
}

bool TimeManager::begin() {
    if (!modem) {
        Serial.println("[TIME] Error - modem not initialized");
        return false;
    }
    
    Serial.println("[TIME] Time Manager starting...");
    
    // Initialize modem time settings with proper AT command sequence
    Serial.println("[TIME] Initializing modem time settings...");
    initializeModemTimeSettings();
    
    // Try to get initial time from modem's internal clock
    if (updateTimeFromModem()) {
        Serial.println("[TIME] Initial time obtained from modem clock");
        return true;
    } else {
        Serial.println("[TIME] Could not get initial time from modem, but initialization completed");
        return true; // Return true anyway, we'll get time later
    }
}

bool TimeManager::performInitialSync(bool force) {
    if (!modem) {
        Serial.println("[TIME] Error - modem not available for sync");
        return false;
    }
    
    // Check if we need to sync (force or never synced or time too old)
    unsigned long time_since_sync = millis() - current_time.last_sync_time;
    if (!force && time_synchronized && time_since_sync < sync_interval) {
        Serial.printf("[TIME] Sync not needed, last sync %lu ms ago\n", time_since_sync);
        return true;
    }
    
    Serial.println("[TIME] Performing initial NTP synchronization...");
    
    // Try primary NTP server first
    if (syncWithNTPServer(ntp_server, INITIAL_SYNC_RETRIES)) {
        Serial.printf("[TIME] Initial sync successful with %s\n", ntp_server.c_str());
        time_synchronized = true;
        current_time.last_sync_status = "Initial sync successful";
        return true;
    }
    
    // Try backup NTP server if primary failed
    if (backup_ntp_server.length() > 0) {
        Serial.printf("[TIME] Primary server failed, trying backup %s\n", backup_ntp_server.c_str());
        if (syncWithNTPServer(backup_ntp_server, INITIAL_SYNC_RETRIES)) {
            Serial.printf("[TIME] Initial sync successful with backup server %s\n", backup_ntp_server.c_str());
            time_synchronized = true;
            current_time.last_sync_status = "Initial sync successful (backup)";
            return true;
        }
    }
    
    Serial.println("[TIME] Initial NTP synchronization failed");
    current_time.last_sync_status = "Initial sync failed";
    return false;
}

bool TimeManager::update() {
    if (!modem) return false;
    
    unsigned long current_millis = millis();
    
    // Check if current time is invalid and needs recovery
    if (!current_time.is_valid && time_synchronized) {
        Serial.println("[TIME] Time became invalid, attempting recovery...");
        if (updateTimeFromModemWithRetries(2)) {
            Serial.println("[TIME] Time recovery successful");
            current_time.last_sync_status = "Time recovered from modem";
        } else {
            Serial.println("[TIME] Time recovery failed, will try NTP on next sync");
            time_synchronized = false; // Mark as needing full resync
        }
        return true;
    }
    
    // Check if it's time for periodic sync
    if (time_synchronized && (current_millis - current_time.last_sync_time >= sync_interval)) {
        Serial.println("[TIME] Performing periodic NTP synchronization...");
        
        if (syncWithNTPServer(ntp_server, PERIODIC_SYNC_RETRIES)) {
            Serial.println("[TIME] Periodic sync successful");
            current_time.last_sync_status = "Periodic sync successful";
        } else if (backup_ntp_server.length() > 0 && 
                   syncWithNTPServer(backup_ntp_server, PERIODIC_SYNC_RETRIES)) {
            Serial.println("[TIME] Periodic sync successful with backup server");
            current_time.last_sync_status = "Periodic sync successful (backup)";
        } else {
            Serial.println("[TIME] Periodic sync failed, keeping current time");
            current_time.last_sync_status = "Periodic sync failed";
            // Try to update from modem as fallback
            if (updateTimeFromModemWithRetries(2)) {
                Serial.println("[TIME] Fallback time update from modem successful");
                current_time.last_sync_status = "Periodic sync failed, updated from modem";
            }
        }
        return true;
    }
    
    // Update time from modem's internal clock (less accurate but better than nothing)
    if (current_millis - last_sync_attempt > 30000) { // Every 30 seconds
        // Use single attempt for regular updates to avoid delays in main loop
        if (!updateTimeFromModem()) {
            Serial.println("[TIME] Regular time update failed");
            // If we haven't had valid time for too long, try recovery
            if (!current_time.is_valid || 
                (current_millis - current_time.last_sync_time > sync_interval * 2)) {
                Serial.println("[TIME] Time seems stale, attempting recovery...");
                updateTimeFromModemWithRetries(2);
            }
        }
        last_sync_attempt = current_millis;
        return true;
    }
    
    return false;
}

bool TimeManager::forceSync() {
    return performInitialSync(true);
}

bool TimeManager::forceTimeUpdate(int max_retries) {
    Serial.printf("[TIME] Force time update requested with %d retries\n", max_retries);
    
    // First try to update from modem with retries
    if (updateTimeFromModemWithRetries(max_retries)) {
        Serial.println("[TIME] Force time update successful from modem");
        return true;
    }
    
    // If modem time update fails, try NTP sync as fallback
    Serial.println("[TIME] Modem time update failed, falling back to NTP sync...");
    return performInitialSync(true);
}

bool TimeManager::initializeModemTime() {
    return initializeModemTimeSettings();
}

bool TimeManager::syncWithNTPServer(const String& server, int retries) {
    if (!modem) return false;
    
    Serial.printf("[TIME] Syncing with NTP server: %s (retries: %d)\n", server.c_str(), retries);
    
    for (int i = 0; i < retries; i++) {
        // Use TinyGSM's NTP synchronization
        int result = modem->NTPServerSync(server, timezone_offset);
        
        if (result == 0) {
            Serial.printf("[TIME] NTP sync successful (attempt %d/%d)\n", i+1, retries);
            
            // Get updated time from modem with retries
            if (updateTimeFromModemWithRetries(3)) {
                current_time.last_sync_time = millis();
                current_time.is_valid = true;
                return true;
            } else {
                Serial.println("[TIME] NTP sync succeeded but couldn't read valid time after retries");
            }
        } else {
            Serial.printf("[TIME] NTP sync failed with code %d (attempt %d/%d)\n", result, i+1, retries);
            if (i < retries - 1) {
                delay(2000); // Wait before retry
            }
        }
    }
    
    return false;
}

bool TimeManager::updateTimeFromModem() {
    if (!modem) return false;
    
    // Get network time from modem
    int year, month, day, hour, minute, second;
    float timezone;
    
    if (modem->getNetworkTime(&year, &month, &day, &hour, &minute, &second, &timezone)) {
        current_time.year = year;
        current_time.month = month;
        current_time.day = day;
        current_time.hour = hour;
        current_time.minute = minute;
        current_time.second = second;
        current_time.timezone = timezone;
        
        // Validate the time
        if (isTimeValid(current_time)) {
            current_time.is_valid = true;
            return true;
        } else {
            Serial.printf("[TIME] Invalid time received: %04d-%02d-%02d %02d:%02d:%02d TZ:%.1f\n",
                         year, month, day, hour, minute, second, timezone);
            
            // If year is way off (like 2070), modem clock is not synchronized
            if (year > 2050 || year < 2025) {
                Serial.println("[TIME] Modem clock appears to be not synchronized with network");
            }
            
            current_time.is_valid = false;
        }
    } else {
        Serial.println("[TIME] Failed to get time from modem");
        current_time.is_valid = false;
    }
    
    return false;
}

String TimeManager::sendATCommand(const String& command, unsigned long timeout) {
    if (!modem) return "";
    
    Serial.printf("[TIME] Sending AT command: %s\n", command.c_str());
    
    // Clear any pending data
    while (modem->stream.available()) {
        modem->stream.read();
    }
    
    // Send command
    modem->stream.println(command);
    
    // Wait for response
    unsigned long start = millis();
    String response = "";
    
    while (millis() - start < timeout) {
        if (modem->stream.available()) {
            char c = modem->stream.read();
            response += c;
            
            // Check for common response endings
            if (response.indexOf("OK") != -1 || 
                response.indexOf("ERROR") != -1 || 
                response.indexOf("+CNTP:") != -1 ||
                response.indexOf("+CCLK:") != -1) {
                break;
            }
        }
        delay(10);
    }
    
    response.trim();
    Serial.printf("[TIME] AT Response: %s\n", response.c_str());
    return response;
}

bool TimeManager::initializeModemTimeSettings() {
    if (!modem) return false;
    
    Serial.println("[TIME] Initializing modem time settings with proper AT command sequence...");
    
    // Step 1: Reset parameters
    Serial.println("[TIME] Step 1: Resetting modem parameters...");
    String response = sendATCommand("AT&F", 3000);
    if (response.indexOf("OK") == -1) {
        Serial.println("[TIME] Warning: AT&F command failed");
    }
    delay(1000);
    
    // Step 2: Enable automatic timezone update from network
    Serial.println("[TIME] Step 2: Enabling automatic timezone update...");
    response = sendATCommand("AT+CTZU=1", 2000);
    if (response.indexOf("OK") == -1) {
        Serial.println("[TIME] Warning: AT+CTZU=1 command failed");
    }
    delay(500);
    
    // Step 3: Set NTP server with timezone
    Serial.println("[TIME] Step 3: Setting NTP server...");
    String ntpCommand = "AT+CNTP=\"" + ntp_server + "\"," + String(timezone_offset);
    response = sendATCommand(ntpCommand, 2000);
    if (response.indexOf("OK") == -1) {
        Serial.printf("[TIME] Warning: NTP server setup failed, trying backup server...\n");
        
        // Try backup server
        if (backup_ntp_server.length() > 0) {
            ntpCommand = "AT+CNTP=\"" + backup_ntp_server + "\"," + String(timezone_offset);
            response = sendATCommand(ntpCommand, 2000);
        }
    }
    delay(500);
    
    // Step 4: Sync the time
    Serial.println("[TIME] Step 4: Synchronizing time...");
    response = sendATCommand("AT+CNTP", 5000);
    
    // Check for success response
    if (response.indexOf("+CNTP: 1") != -1) {
        Serial.println("[TIME] NTP sync command executed successfully");
    } else if (response.indexOf("ERROR") != -1) {
        Serial.println("[TIME] NTP sync command failed");
        return false;
    }
    
    // Step 5: Wait for sync to complete
    Serial.println("[TIME] Step 5: Waiting for sync completion...");
    delay(5000); // Wait 5 seconds for sync to complete
    
    // Step 6: Retrieve updated time
    Serial.println("[TIME] Step 6: Retrieving updated time...");
    response = sendATCommand("AT+CCLK?", 2000);
    
    if (response.indexOf("+CCLK:") != -1) {
        Serial.println("[TIME] Modem time initialization completed successfully");
        Serial.printf("[TIME] Current modem time response: %s\n", response.c_str());
        return true;
    } else {
        Serial.println("[TIME] Failed to retrieve time after initialization");
        return false;
    }
}

bool TimeManager::forceModemTimeSync() {
    if (!modem) return false;
    
    Serial.println("[TIME] Forcing modem time synchronization with network...");
    
    // First, try the proper AT command sequence
    if (initializeModemTimeSettings()) {
        Serial.println("[TIME] Modem time initialization successful");
        return true;
    }
    
    // Fallback: Try the old TinyGSM method
    Serial.println("[TIME] Fallback: Attempting TinyGSM NTP sync...");
    int result = modem->NTPServerSync(ntp_server, timezone_offset);
    
    if (result == 0) {
        Serial.println("[TIME] TinyGSM NTP sync successful");
        delay(2000); // Give time for modem to update internal clock
        return true;
    } else {
        Serial.printf("[TIME] TinyGSM NTP sync failed with code: %d\n", result);
        
        // Try backup server
        if (backup_ntp_server.length() > 0) {
            Serial.println("[TIME] Trying backup server...");
            result = modem->NTPServerSync(backup_ntp_server, timezone_offset);
            if (result == 0) {
                Serial.println("[TIME] Backup NTP sync successful");
                delay(2000);
                return true;
            }
        }
    }
    
    Serial.println("[TIME] All modem time sync methods failed");
    return false;
}

bool TimeManager::updateTimeFromModemWithRetries(int max_retries) {
    if (!modem) return false;
    
    Serial.printf("[TIME] Updating time from modem with up to %d retries...\n", max_retries);
    
    // First, check if we need to force modem time sync
    bool tried_sync = false;
    
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        Serial.printf("[TIME] Time update attempt %d/%d\n", attempt, max_retries);
        
        if (updateTimeFromModem()) {
            Serial.printf("[TIME] Time update successful on attempt %d\n", attempt);
            return true;
        }
        
        // If we get invalid time (like year 2070), try to sync modem time first
        if (!tried_sync && current_time.year > 2050) {
            Serial.println("[TIME] Detected invalid modem time, attempting to sync modem with network...");
            if (forceModemTimeSync()) {
                tried_sync = true;
                Serial.println("[TIME] Modem time sync completed, retrying time update...");
                continue; // Try again immediately after sync
            }
        }
        
        if (attempt < max_retries) {
            Serial.printf("[TIME] Attempt %d failed, waiting 2 seconds before retry...\n", attempt);
            delay(2000);
        }
    }
    
    Serial.printf("[TIME] All %d attempts failed to get valid time from modem\n", max_retries);
    return false;
}

bool TimeManager::isTimeValid(const TimeData& time_data) {
    // Detailed validation of time data with logging
    bool valid = true;
    String error_msg = "[TIME] Time validation errors: ";
    
    // Check for obviously wrong years (like 2070 from unsynced modem)
    if (time_data.year < 2025 || time_data.year > 2040) {
        error_msg += "Year(" + String(time_data.year);
        if (time_data.year > 2050) {
            error_msg += " - modem not synced";
        } else if (time_data.year < 2025) {
            error_msg += " - too old";
        }
        error_msg += ") ";
        valid = false;
    }
    
    if (time_data.month < 1 || time_data.month > 12) {
        error_msg += "Month(" + String(time_data.month) + ") ";
        valid = false;
    }
    
    if (time_data.day < 1 || time_data.day > 31) {
        error_msg += "Day(" + String(time_data.day) + ") ";
        valid = false;
    }
    
    if (time_data.hour < 0 || time_data.hour >= 24) {
        error_msg += "Hour(" + String(time_data.hour) + ") ";
        valid = false;
    }
    
    if (time_data.minute < 0 || time_data.minute >= 60) {
        error_msg += "Minute(" + String(time_data.minute) + ") ";
        valid = false;
    }
    
    if (time_data.second < 0 || time_data.second >= 60) {
        error_msg += "Second(" + String(time_data.second) + ") ";
        valid = false;
    }
    
    if (!valid) {
        Serial.println(error_msg);
        
        // Special handling for common issues
        if (time_data.year > 2050) {
            Serial.println("[TIME] DIAGNOSIS: Modem internal clock not synchronized with cellular network");
            Serial.println("[TIME] SOLUTION: Will attempt NTP synchronization or manual time sync");
        }
    }
    
    return valid;
}

TimeData TimeManager::getLocalTime() {
    TimeData local_time = current_time;
    
    if (!current_time.is_valid) return local_time;
    
    // Применяем смещение временной зоны к часам
    local_time.hour += timezone_offset;
    
    // Обработка переполнения часов
    if (local_time.hour >= 24) {
        local_time.hour -= 24;
        local_time.day++;
        
        // Простая проверка на переход к следующему дню/месяцу
        int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if ((local_time.year % 4 == 0 && local_time.year % 100 != 0) || (local_time.year % 400 == 0)) {
            days_in_month[1] = 29; // Високосный год
        }
        
        if (local_time.day > days_in_month[local_time.month - 1]) {
            local_time.day = 1;
            local_time.month++;
            if (local_time.month > 12) {
                local_time.month = 1;
                local_time.year++;
            }
        }
    } else if (local_time.hour < 0) {
        local_time.hour += 24;
        local_time.day--;
        
        if (local_time.day < 1) {
            local_time.month--;
            if (local_time.month < 1) {
                local_time.month = 12;
                local_time.year--;
            }
            
            // Получаем количество дней в предыдущем месяце
            int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if ((local_time.year % 4 == 0 && local_time.year % 100 != 0) || (local_time.year % 400 == 0)) {
                days_in_month[1] = 29;
            }
            local_time.day = days_in_month[local_time.month - 1];
        }
    }
    
    return local_time;
}

String TimeManager::formatTime(const TimeData& time_data) {
    if (!time_data.is_valid) return "Invalid";
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
             time_data.hour, time_data.minute, time_data.second);
    return String(buffer);
}

String TimeManager::formatDate(const TimeData& time_data) {
    if (!time_data.is_valid) return "Invalid";
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", 
             time_data.day, time_data.month, time_data.year);
    return String(buffer);
}

void TimeManager::setTimezone(int tz_offset) {
    timezone_offset = tz_offset;
    Serial.printf("[TIME] Timezone set to UTC%+d\n", timezone_offset);
}

void TimeManager::setNTPServer(const String& server, const String& backup_server) {
    ntp_server = server;
    if (backup_server.length() > 0) {
        backup_ntp_server = backup_server;
    }
    Serial.printf("[TIME] NTP servers set - Primary: %s, Backup: %s\n", 
                  ntp_server.c_str(), backup_ntp_server.c_str());
}

void TimeManager::setSyncInterval(unsigned long interval_ms) {
    sync_interval = interval_ms;
    Serial.printf("[TIME] Sync interval set to %lu ms\n", sync_interval);
}

bool TimeManager::isTimeSynchronized() {
    return time_synchronized && current_time.is_valid;
}

TimeData TimeManager::getCurrentTime() {
    return current_time;
}

String TimeManager::getTimeString() {
    return formatTime(getLocalTime());
}

String TimeManager::getDateString() {
    return formatDate(getLocalTime());
}

String TimeManager::getDateTimeString() {
    if (!current_time.is_valid) return "Invalid";
    
    TimeData local_time = getLocalTime();
    return formatDate(local_time) + " " + formatTime(local_time);
}

String TimeManager::getDisplayTimeString() {
    if (!current_time.is_valid) return "Invalid";
    
    TimeData local_time = getLocalTime();
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             local_time.year, local_time.month, local_time.day,
             local_time.hour, local_time.minute, local_time.second);
    return String(buffer);
}

String TimeManager::getISO8601String() {
    if (!current_time.is_valid) return "Invalid";
    
    char buffer[32];
    char tz_buffer[8];
    
    // Format timezone
    if (timezone_offset >= 0) {
        snprintf(tz_buffer, sizeof(tz_buffer), "+%02d:00", timezone_offset);
    } else {
        snprintf(tz_buffer, sizeof(tz_buffer), "-%02d:00", -timezone_offset);
    }
    
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d%s",
             current_time.year, current_time.month, current_time.day,
             current_time.hour, current_time.minute, current_time.second,
             tz_buffer);
    
    return String(buffer);
}

unsigned long TimeManager::getUnixTimestamp() {
    if (!current_time.is_valid) return 0;
    
    // Simple calculation - not accounting for leap years properly
    // This is a rough approximation
    unsigned long days_since_1970 = 0;
    
    // Years since 1970
    for (int year = 1970; year < current_time.year; year++) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days_since_1970 += 366; // Leap year
        } else {
            days_since_1970 += 365;
        }
    }
    
    // Months this year
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Adjust February for leap year
    if ((current_time.year % 4 == 0 && current_time.year % 100 != 0) || 
        (current_time.year % 400 == 0)) {
        days_in_month[1] = 29;
    }
    
    for (int month = 1; month < current_time.month; month++) {
        days_since_1970 += days_in_month[month - 1];
    }
    
    // Days this month (minus 1 because we count from day 1)
    days_since_1970 += (current_time.day - 1);
    
    // Convert to seconds and add time of day
    unsigned long timestamp = days_since_1970 * 86400; // 24 * 60 * 60
    timestamp += current_time.hour * 3600;
    timestamp += current_time.minute * 60;
    timestamp += current_time.second;
    
    // Adjust for timezone (subtract because we want UTC)
    timestamp -= (timezone_offset * 3600);
    
    return timestamp;
}

unsigned long TimeManager::getTimeSinceLastSync() {
    if (current_time.last_sync_time == 0) return 0;
    return millis() - current_time.last_sync_time;
}

String TimeManager::getSyncStatus() {
    return current_time.last_sync_status;
}

int TimeManager::getTimezone() {
    return timezone_offset;
}

void TimeManager::printTimeInfo() {
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│           TIME STATUS               │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Date/Time:   %-20s │\n", getDateTimeString().c_str());
    Serial.printf("│ ISO8601:     %-20s │\n", getISO8601String().c_str());
    Serial.printf("│ Unix Time:   %-20lu │\n", getUnixTimestamp());
    Serial.printf("│ Timezone:    UTC%+d                 │\n", timezone_offset);
    Serial.printf("│ Synchronized: %-19s │\n", time_synchronized ? "Yes" : "No");
    Serial.printf("│ Valid:       %-20s │\n", current_time.is_valid ? "Yes" : "No");
    Serial.printf("│ NTP Server:  %-20s │\n", ntp_server.c_str());
    
    if (current_time.last_sync_time > 0) {
        unsigned long sync_age = getTimeSinceLastSync();
        Serial.printf("│ Last Sync:   %-8lu ms ago       │\n", sync_age);
    } else {
        Serial.println("│ Last Sync:   Never                  │");
    }
    
    Serial.printf("│ Status:      %-20s │\n", current_time.last_sync_status.c_str());
    Serial.println("└─────────────────────────────────────┘");
}
