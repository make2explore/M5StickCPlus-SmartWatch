// ---------------------------------- make2explore.com -------------------------------------------------------//
// Project           - Simple Smarwatch using M5StickCPlus Development board
// Created By        - info@make2explore.com
// Last Modified     - 23/04/2025 17:36:00 @admin
// Software          - C/C++, Arduino IDE, Libraries
// Hardware          - M5StickCPlus Development board     
// Sensors Used      - Built-in RTC
// Source Repo       - github.com/make2explore
// ===========================================================================================================//

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp_sleep.h>
#include "time.h"

// WiFi Credentials
const char* ssid = "xxxxx";
const char* password = "xxxxxx";

// NTP Client Setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // UTC offset for your timezone

// Days and Months
const char* daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* monthsOfYear[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Power-saving mode settings
const int SLEEP_TIME_SECONDS = 30;  // Sleep after 30 sec of inactivity
unsigned long lastActivityTime = 0;
bool screenOn = true;

void syncRTCWithNTP() {
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Try for 10 seconds
        delay(500);
        M5.Lcd.setCursor(20, 20);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Connecting...");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        timeClient.begin();
        timeClient.update();

        // Sync RTC with NTP time
        time_t rawTime = timeClient.getEpochTime();
        struct tm *timeInfo = localtime(&rawTime);

        RTC_TimeTypeDef RTC_TimeStruct;
        RTC_TimeStruct.Hours = timeInfo->tm_hour;
        RTC_TimeStruct.Minutes = timeInfo->tm_min;
        RTC_TimeStruct.Seconds = timeInfo->tm_sec;
        M5.Rtc.SetTime(&RTC_TimeStruct);

        RTC_DateTypeDef RTC_DateStruct;
        RTC_DateStruct.WeekDay = timeInfo->tm_wday;
        RTC_DateStruct.Month = timeInfo->tm_mon + 1; // RTC uses 1-based months
        RTC_DateStruct.Date = timeInfo->tm_mday;
        RTC_DateStruct.Year = timeInfo->tm_year + 1900; // RTC needs full year
        M5.Rtc.SetDate(&RTC_DateStruct);
    }

    WiFi.disconnect();  // Save power by disconnecting Wi-Fi
}

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Axp.SetLDO2(true);   // Ensure backlight power is ON
    M5.Axp.ScreenBreath(12);  // Set max brightness
    lastActivityTime = millis();

    // Get RTC Time to check if it's valid
    RTC_DateTypeDef RTC_DateStruct;
    M5.Rtc.GetDate(&RTC_DateStruct);

    // If RTC has incorrect date (Jan 01, 1970), sync with NTP
    if (RTC_DateStruct.Year < 2000) {
        syncRTCWithNTP();
    }
}

void loop() {
    M5.update();

    // Wake up screen on button press
    if (M5.BtnA.wasPressed()) {
        lastActivityTime = millis();
        if (!screenOn) {
            screenOn = true;
        }
    }

    // Enter sleep mode if inactive
    if (millis() - lastActivityTime > SLEEP_TIME_SECONDS * 1000) {
        enterSleepMode();
    }

    // Get current time from RTC
    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_DateTypeDef RTC_DateStruct;
    M5.Rtc.GetTime(&RTC_TimeStruct);
    M5.Rtc.GetDate(&RTC_DateStruct);

    // Convert to 12-hour format
    int hours = RTC_TimeStruct.Hours;
    String am_pm = "AM";
    if (hours >= 12) {
        am_pm = "PM";
        if (hours > 12) hours -= 12;
    }
    if (hours == 0) hours = 12;

    int minutes = RTC_TimeStruct.Minutes;
    int seconds = RTC_TimeStruct.Seconds;
    int day = RTC_DateStruct.Date;
    int month = RTC_DateStruct.Month;
    int year = RTC_DateStruct.Year;
    const char* dayOfWeek = daysOfWeek[RTC_DateStruct.WeekDay];

    // Clear screen
    M5.Lcd.fillScreen(BLACK);

    // Display Date & Day
    M5.Lcd.setCursor(15, 5);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf("%s, %s %02d, %d", dayOfWeek, monthsOfYear[month - 1], day, year);

    // Display Time
    M5.Lcd.setCursor(15, 50);
    M5.Lcd.setTextSize(5);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf("%02d:%02d%s", hours, minutes, am_pm.c_str());

    // Display Battery Percentage
    int battery = M5.Axp.GetBatVoltage() * 100 / 4.2; // Approximate calculation
    M5.Lcd.setCursor(50, 110);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf("Battery: %d%%", battery);

    delay(1000);
}

void enterSleepMode() {
    M5.Lcd.fillScreen(BLACK);  // Clear screen before sleep
    M5.Axp.ScreenBreath(0);  // Turn off display
    screenOn = false;
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, 0);  // Wake up on button press
    esp_deep_sleep_start();  // Enter deep sleep mode
}

// ---------------------------------- make2explore.com----------------------------------------------------//
