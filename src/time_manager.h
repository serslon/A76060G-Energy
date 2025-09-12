/**
 * @file      time_manager.h
 * @author    A7670G Energy Project
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-09-01
 * @brief     Time manager for A7670G Energy Project with NTP synchronization
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "Arduino.h"
#include <time.h>

// Определяем тип модема перед включением TinyGSM
#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif

#include <TinyGsmClient.h>

// NTP Configuration
#define DEFAULT_NTP_SERVER      "pool.ntp.org"
#define BACKUP_NTP_SERVER       "time.nist.gov"
#define DEFAULT_TIMEZONE        2  // UTC+2 для Украины (летнее время +3, зимнее +2)
#define TIME_SYNC_INTERVAL      3600000  // 1 час в миллисекундах
#define INITIAL_SYNC_RETRIES    5
#define PERIODIC_SYNC_RETRIES   3

struct TimeData {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    float timezone;
    bool is_valid;
    unsigned long last_sync_time;
    String last_sync_status;
};

class TimeManager {
private:
    TinyGsm* modem;
    TimeData current_time;
    unsigned long last_sync_attempt;
    unsigned long sync_interval;
    bool time_synchronized;
    int timezone_offset;
    String ntp_server;
    String backup_ntp_server;
    
    // Internal methods
    bool syncWithNTPServer(const String& server, int retries = 3);
    bool updateTimeFromModem();
    bool updateTimeFromModemWithRetries(int max_retries = 3);
    bool forceModemTimeSync();
    bool initializeModemTimeSettings();
    String sendATCommand(const String& command, unsigned long timeout = 2000);
    String formatTime(const TimeData& time_data);
    String formatDate(const TimeData& time_data);
    bool isTimeValid(const TimeData& time_data);
    TimeData getLocalTime(); // Получить время с учетом локальной временной зоны
    
public:
    /**
     * @brief Constructor
     * @param gsm Pointer to TinyGSM modem object
     * @param tz_offset Timezone offset from UTC (default: 2 for Ukraine)
     * @param sync_interval_ms Sync interval in milliseconds (default: 1 hour)
     */
    TimeManager(TinyGsm* gsm, int tz_offset = DEFAULT_TIMEZONE, 
                unsigned long sync_interval_ms = TIME_SYNC_INTERVAL);
    
    /**
     * @brief Initialize time manager
     * @return true if initialization successful
     */
    bool begin();
    
    /**
     * @brief Perform initial time synchronization with NTP server
     * This should be called after internet connection is established
     * @param force Force synchronization even if recently synced
     * @return true if synchronization successful
     */
    bool performInitialSync(bool force = false);
    
    /**
     * @brief Update time manager (should be called in loop)
     * Performs periodic synchronization every sync_interval
     * @return true if any action was performed
     */
    bool update();
    
    /**
     * @brief Force time synchronization with NTP server
     * @return true if synchronization successful
     */
    bool forceSync();
    
    /**
     * @brief Force time update from modem with retries
     * @param max_retries Maximum number of retry attempts
     * @return true if time update successful
     */
    bool forceTimeUpdate(int max_retries = 5);
    
    /**
     * @brief Initialize modem time settings with proper AT command sequence
     * @return true if initialization successful
     */
    bool initializeModemTime();
    
    /**
     * @brief Set timezone offset
     * @param tz_offset Timezone offset from UTC
     */
    void setTimezone(int tz_offset);
    
    /**
     * @brief Set NTP server
     * @param server Primary NTP server
     * @param backup_server Backup NTP server (optional)
     */
    void setNTPServer(const String& server, const String& backup_server = "");
    
    /**
     * @brief Set synchronization interval
     * @param interval_ms Interval in milliseconds
     */
    void setSyncInterval(unsigned long interval_ms);
    
    /**
     * @brief Check if time is synchronized and valid
     * @return true if time is synchronized and valid
     */
    bool isTimeSynchronized();
    
    /**
     * @brief Get current time data
     * @return TimeData structure with current time
     */
    TimeData getCurrentTime();
    
    /**
     * @brief Get formatted current time string (HH:MM:SS)
     * @return Time string in HH:MM:SS format
     */
    String getTimeString();
    
    /**
     * @brief Get formatted current date string (DD/MM/YYYY)
     * @return Date string in DD/MM/YYYY format
     */
    String getDateString();
    
    /**
     * @brief Get formatted date and time string (DD/MM/YYYY HH:MM:SS)
     * @return DateTime string in DD/MM/YYYY HH:MM:SS format
     */
    String getDateTimeString();
    
    /**
     * @brief Get formatted date and time string for display (YYYY-MM-DD HH:MM:SS)
     * @return DateTime string in YYYY-MM-DD HH:MM:SS format
     */
    String getDisplayTimeString();
    
    /**
     * @brief Get ISO8601 formatted timestamp
     * @return ISO8601 timestamp string (YYYY-MM-DDTHH:MM:SS+TZ)
     */
    String getISO8601String();
    
    /**
     * @brief Get Unix timestamp
     * @return Unix timestamp (seconds since 1970-01-01)
     */
    unsigned long getUnixTimestamp();
    
    /**
     * @brief Get time since last synchronization
     * @return Time in milliseconds since last successful sync
     */
    unsigned long getTimeSinceLastSync();
    
    /**
     * @brief Get synchronization status
     * @return Status string describing last sync attempt
     */
    String getSyncStatus();
    
    /**
     * @brief Get timezone offset
     * @return Current timezone offset
     */
    int getTimezone();
    
    /**
     * @brief Print time information to Serial
     */
    void printTimeInfo();
};

#endif // TIME_MANAGER_H
