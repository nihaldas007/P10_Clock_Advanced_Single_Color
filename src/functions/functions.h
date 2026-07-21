#pragma once
#include <Arduino.h>
#include <DMD32.h>
#include <RTClib.h>
#include <WiFi.h>
#include <WiFiManager.h> // Install "WiFiManager" by tzapu
#include <SPI.h>
#include <time.h>
#include <Preferences.h> // NEW: For saving settings

// --- MENU SYSTEM ---
enum MenuState {
  MENU_IDLE = 0,
  MENU_ROOT,
  MENU_WIFI,
  MENU_TIME_HH,
  MENU_TIME_MM,
  MENU_DATE_DD,
  MENU_DATE_MM,
  MENU_DATE_YY
};
extern MenuState menuState;
extern int currentMenuItem;
extern int menuEditHH;
extern int menuEditMM;
extern int menuEditDD;
extern int menuEditMon;
extern int menuEditYY;
extern int brightnessIndex;

extern void toggleMenu();
extern void menuUpPress();
extern void menuDownPress();

#include "Clocks.h"

// ------------------- Global Variables -------------------
Preferences preferences; // NEW: Create preferences object

// Panel OE / Brightness
const int PANEL_OE = 4;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 1000;
const int PWM_RES = 8;

// --- NEW VARIABLES FOR BUTTON & BRIGHTNESS ---
int brightnessIndex = 0;              // 0=Low, 1=Mid, 2=High
int brightnessValues[] = {5, 30, 50}; // The 3 levels you requested
int clickCount = 0;                   // Counts how many times button is clicked
unsigned long lastClickTime = 0;      // Timer for double click speed
const int DOUBLE_CLICK_GAP = 400;     // Time (ms) to wait for a second click
// FreeRTOS task handles
TaskHandle_t refreshTaskHandle = NULL;
TaskHandle_t clockTaskHandle = NULL;
TaskHandle_t ntpTaskHandle = NULL;
SemaphoreHandle_t i2cMutex = NULL; // NEW: protect Wire/RTC access

// Forward Declarations
void Clock1Task(void *pvParameters);
void Clock2Task(void *pvParameters);
void Clock3Task(void *pvParameters);
void Clock4Task(void *pvParameters);
void Clock5Task(void *pvParameters);
void Clock6Task(void *pvParameters);
void Clock7Task(void *pvParameters);
void Clock8Task(void *pvParameters);
void EventScrollTask(void *pvParameters);
bool myGetLocalTime(struct tm *timeinfo);
void changeClockMode(int mode);
void backgroundSyncTask(void *pvParameters);
void bright();
void wifimanager();
void modeChange();

// ------------------- Refresh Task -------------------
void refreshDisplay(void *pvParameters)
{
    for (;;)
    {
        dmd.scanDisplayBySPI();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
// ------------------- Brightness -------------------
void setBrightness(int level)
{
    level = constrain(level, 0, 255);
    ledcWrite(PWM_CHANNEL, map(level, 0, 255, 0, (1 << PWM_RES) - 1));
}

// ------------------- Helper: Switch Clock Mode -------------------
void changeClockMode(int mode)
{
    // 1. Save old task handle and set to NULL
    TaskHandle_t oldTask = clockTaskHandle;
    clockTaskHandle = NULL;

    // 2. Clear Screen
    dmd.clearScreen(true);

    // 3. Start the new task
    if (mode == 0)
    {
        xTaskCreatePinnedToCore(Clock1Task, "Clock1", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 1)
    {
        xTaskCreatePinnedToCore(Clock2Task, "Clock2", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 2)
    {
        xTaskCreatePinnedToCore(Clock3Task, "Clock3", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 3)
    {
        xTaskCreatePinnedToCore(Clock4Task, "Clock4", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 4)
    {
        xTaskCreatePinnedToCore(Clock5Task, "Clock5", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 5)
    {
        xTaskCreatePinnedToCore(Clock6Task, "Clock6", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 6)
    {
        xTaskCreatePinnedToCore(Clock7Task, "Clock7", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 7)
    {
        xTaskCreatePinnedToCore(Clock8Task, "Clock8", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 99)
    {
        xTaskCreatePinnedToCore(EventScrollTask, "Event", 4096, NULL, 1, &clockTaskHandle, 1);
    }
    else if (mode == 100)
    {
        extern void MenuTask(void *pvParameters);
        xTaskCreatePinnedToCore(MenuTask, "Menu", 4096, NULL, 1, &clockTaskHandle, 1);
    }

    // 4. Delete the old task safely
    if (oldTask != NULL)
    {
        vTaskDelete(oldTask);
    }
}

// ------------------- BACKGROUND WIFI/NTP TASK -------------------
// Runs FOREVER. Checks connection every 10 seconds.
void backgroundSyncTask(void *pvParameters)
{
    // Set NTP config once at the start
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);

    // Ensure we are in Station mode
    WiFi.mode(WIFI_STA);

    for (;;)
    { // Infinite Loop

        // 1. CHECK CONNECTION
        if (WiFi.status() == WL_CONNECTED)
        {
            // --- We are Connected ---
            struct tm timeinfo;
            // Try to get time with short timeout (wait up to 200ms)
            Serial.println("[Background] WiFi Connected. Fetching NTP...");
            if (getLocalTime(&timeinfo, 200))
            {
                // Update RTC (try to take i2c mutex; skip if bus busy)
                if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(500)))
                {
                    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
                    xSemaphoreGive(i2cMutex);
                    Serial.println("[Background] WiFi OK. RTC Updated.");
                }
                else
                {
                    Serial.println("[Background] I2C busy: skipping RTC update.");
                }
            }
        }
        else
        {
            // --- We are Disconnected ---
            Serial.println("[Background] WiFi lost! Attempting Reconnect...");
            // Try to reconnect using credentials saved by WiFiManager
            WiFi.begin();
        }

        // 2. WAIT 10 SECONDS
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    // We never reach here, so no vTaskDelete needed.
}

void modeChange()
{
    currentMode++;
    if (currentMode > 7)
        currentMode = 0;

    preferences.putInt("mode", currentMode);
    Serial.println("Action: Switch Clock Mode");
    
    // Only apply immediately if we are not in the menu system
    if (menuState == MENU_IDLE) {
        changeClockMode(currentMode);
    }
}
void bright()
{
    brightnessIndex++;
    if (brightnessIndex > 2)
        brightnessIndex = 0; // Cycle 0->1->2->0

    int newLevel = brightnessValues[brightnessIndex];
    setBrightness(newLevel);

    preferences.putInt("brightIdx", brightnessIndex);
    Serial.print("Action: Brightness Level ");
    Serial.println(newLevel);
    if (newLevel != 5){
        if(_hour24 >= 0 && _hour24 <= 7)vTaskDelay(pdMS_TO_TICKS(1000));
    }
        
}

// ------------------- WIFI AP MODE -------------------
void startWiFiAPMode() {
    Serial.println("Starting WiFi AP for configuration...");
    WiFiManager wm;
    wm.setConfigPortalTimeout(180); // 3 minutes timeout
    // Blocks here while the AP is active
    if (!wm.startConfigPortal("P10_CLOCK")) {
        Serial.println("Failed to connect or hit timeout");
    }
    // Once done, restart
    ESP.restart();
}

// ------------------- MENU SYSTEM LOGIC -------------------
void toggleMenu() {
    if (menuState == MENU_IDLE) {
        menuState = MENU_ROOT;
        currentMenuItem = 0;
        changeClockMode(100); // 100 triggers MenuTask
    } else {
        menuState = MENU_IDLE;
        changeClockMode(currentMode); // Restore current clock dial
    }
}

void menuUpPress() {
    if (menuState == MENU_ROOT) {
        currentMenuItem++;
        if (currentMenuItem > 3) currentMenuItem = 0; // 0=WIFI, 1=TIME, 2=DATE, 3=EXIT
    }
    else if (menuState == MENU_TIME_HH) {
        menuState = MENU_TIME_MM; // Move to MM
    }
    else if (menuState == MENU_TIME_MM) {
        // Save Time and Exit
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
            DateTime now = rtc.now();
            rtc.adjust(DateTime(now.year(), now.month(), now.day(), menuEditHH, menuEditMM, now.second()));
            xSemaphoreGive(i2cMutex);
        }
        toggleMenu(); // Exit menu
    }
    else if (menuState == MENU_DATE_DD) {
        menuState = MENU_DATE_MM; // Move to MM
    }
    else if (menuState == MENU_DATE_MM) {
        menuState = MENU_DATE_YY; // Move to YY
    }
    else if (menuState == MENU_DATE_YY) {
        // Save Date and Exit
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
            DateTime now = rtc.now();
            rtc.adjust(DateTime(menuEditYY, menuEditMon, menuEditDD, now.hour(), now.minute(), now.second()));
            xSemaphoreGive(i2cMutex);
        }
        toggleMenu(); // Exit menu
    }
    else if (menuState == MENU_WIFI) {
        menuState = MENU_ROOT; // Back to main menu
    }
}

void menuDownPress() {
    if (menuState == MENU_ROOT) {
        if (currentMenuItem == 0) { // WiFi
            menuState = MENU_WIFI;
            startWiFiAPMode();
        } else if (currentMenuItem == 1) { // Time
            menuState = MENU_TIME_HH;
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
                DateTime now = rtc.now();
                menuEditHH = now.hour();
                menuEditMM = now.minute();
                xSemaphoreGive(i2cMutex);
            }
        } else if (currentMenuItem == 2) { // Date
            menuState = MENU_DATE_DD;
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
                DateTime now = rtc.now();
                menuEditDD = now.day();
                menuEditMon = now.month();
                menuEditYY = now.year();
                xSemaphoreGive(i2cMutex);
            }
        } else if (currentMenuItem == 3) { // Exit
            toggleMenu();
        }
    }
    else if (menuState == MENU_TIME_HH) {
        menuEditHH++;
        if (menuEditHH > 23) menuEditHH = 0;
    }
    else if (menuState == MENU_TIME_MM) {
        menuEditMM++;
        if (menuEditMM > 59) menuEditMM = 0;
    }
    else if (menuState == MENU_DATE_DD) {
        menuEditDD++;
        if (menuEditDD > 31) menuEditDD = 1;
    }
    else if (menuState == MENU_DATE_MM) {
        menuEditMon++;
        if (menuEditMon > 12) menuEditMon = 1;
    }
    else if (menuState == MENU_DATE_YY) {
        menuEditYY++;
        if (menuEditYY > 2099) menuEditYY = 2024;
    }
}

void wifimanager()
{
    Serial.println("Long Press: Entering WiFi Config Portal");

    // Stop other tasks
    if (clockTaskHandle != NULL)
        vTaskDelete(clockTaskHandle);
    if (ntpTaskHandle != NULL)
        vTaskDelete(ntpTaskHandle);

    dmd.clearScreen(true);
    dmd.selectFont(SystemFont5x7);
    dmd.drawString(5, 0, "WIFI", 4, GRAPHICS_NORMAL);
    dmd.drawString(1, 8, "SETUP", 5, GRAPHICS_NORMAL);

    WiFiManager wm;
    wm.setClass("invert");
    wm.setTitle("Setup WiFi");
    wm.setConnectTimeout(20);

    const char *customHead = "<style>body{background:#e0ffe0;}button{background-color:red !important;}</style>";
    wm.setCustomHeadElement(customHead);

    std::vector<const char *> menu = {"wifi", "restart", "exit"};
    wm.setMenu(menu);
    wm.setConfigPortalTimeout(120);

    if (!wm.startConfigPortal("Smart Clock"))
    {
        Serial.println("Setup timed out");
    }
    else
    {
        Serial.println("WiFi Saved");
    }
    ESP.restart();
}