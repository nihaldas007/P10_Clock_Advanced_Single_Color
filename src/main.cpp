//......................ESP32 only 1.0.6 Board (Preferred)............//
#include <Arduino.h>
#include <DMD32.h>
#include <RTClib.h>
#include <WiFi.h>
#include <WiFiManager.h> // Install "WiFiManager" by tzapu
#include <SPI.h>
#include <time.h>
#include <Preferences.h> // NEW: For saving settings
#include "Clocks.h"

// Pins
#define UP_PIN 32   // The BOOT button
#define DOWN_PIN 33   // The BOOT button
#define RTC_POWERPIN 26  // The BOOT button
Preferences preferences; // NEW: Create preferences object

// Panel OE / Brightness
const int PANEL_OE = 4;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 16000;
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
bool myGetLocalTime(struct tm *timeinfo);
void changeClockMode(int mode);
void backgroundSyncTask(void *pvParameters);
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
  // 1. Delete the currently running clock task if it exists
  if (clockTaskHandle != NULL)
  {
    vTaskDelete(clockTaskHandle);
    clockTaskHandle = NULL;
  }

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
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(500))) {
          rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
          xSemaphoreGive(i2cMutex);
          Serial.println("[Background] WiFi OK. RTC Updated.");
        } else {
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
// ------------------- Setup -------------------
void setup()
{
  Serial.begin(115200);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(RTC_POWERPIN, OUTPUT);
  digitalWrite(RTC_POWERPIN, HIGH);
  delay(500);
  // 1. Init Preferences
  preferences.begin("clock-app", false);                // RESTORE MODE
  currentMode = preferences.getInt("mode", 0);          // RESTORE BRIGHTNESS (NEW)
  brightnessIndex = preferences.getInt("brightIdx", 0); // Safety check: ensure index is 0-2
  if (brightnessIndex < 0 || brightnessIndex > 2)
    brightnessIndex = 0;

  Serial.print("Restored Mode: ");
  Serial.println(currentMode);
  Serial.print("Restored Brightness: ");
  Serial.println(brightnessValues[brightnessIndex]);

  // 2. PWM & Brightness
  pinMode(PANEL_OE, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PANEL_OE, PWM_CHANNEL);

  // Apply the restored brightness immediately
  setBrightness(brightnessValues[brightnessIndex]);

  // 3. Start Display Refresh
  xTaskCreatePinnedToCore(refreshDisplay, "refreshDisplay", 4096, NULL, 20, &refreshTaskHandle, 1);

  // 4. Initialize RTC
  Wire.begin();

  // Create I2C mutex to serialize all Wire/RTC usage (prevents heap corruption)
  i2cMutex = xSemaphoreCreateMutex();
  if (i2cMutex == NULL) {
    Serial.println("ERROR: Failed to create I2C mutex");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    dmd.clearScreen(true);
    dmd.selectFont(SystemFont5x7);
    dmd.drawString(0, 4, "NO RTC", 6, GRAPHICS_NORMAL);
    while (1);
  }

  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, setting compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // 4. clear screen
  dmd.clearScreen(true);
  // 5. Start the saved clock mode
  changeClockMode(currentMode);
  // 6. WiFi Manager
  xTaskCreatePinnedToCore(backgroundSyncTask, "bkSync", 8192, NULL, 1, &ntpTaskHandle, 0);
}

void loop()
{
  unsigned long now = millis();

  // ==========================================
  // BUTTON 33: MODE (Short) & WIFI (Long)
  // ==========================================
  static int lastState33 = HIGH;
  static unsigned long pressStart33 = 0;
  int currentState33 = digitalRead(UP_PIN);

  // 1. Detect Press (Falling Edge)
  if (lastState33 == HIGH && currentState33 == LOW)
  {
    pressStart33 = now;
  }

  // 2. Detect Release (Rising Edge)
  if (lastState33 == LOW && currentState33 == HIGH)
  {
    unsigned long duration = now - pressStart33;

    if (duration > 3000)
    {
      // --- LONG PRESS: WIFI CONFIG ---
      Serial.println("Long Press: Entering WiFi Config Portal");

      // Stop other tasks
      if (clockTaskHandle != NULL) vTaskDelete(clockTaskHandle);
      if (ntpTaskHandle != NULL) vTaskDelete(ntpTaskHandle);

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

      if (!wm.startConfigPortal("Smart Clock")) {
        Serial.println("Setup timed out");
      } else {
        Serial.println("WiFi Saved");
      }
      ESP.restart();
    }
    else if (duration > 50)
    {
      // --- SHORT PRESS: CHANGE MODE ---
      // (Moved from double-click logic to here)
      currentMode++;
      if (currentMode > 7) currentMode = 0;

      preferences.putInt("mode", currentMode);
      Serial.println("Action: Switch Clock Mode");
      changeClockMode(currentMode);
    }
  }
  lastState33 = currentState33;

  // ==========================================
  // BUTTON 32: BRIGHTNESS (Short Press)
  // ==========================================
  static int lastState32 = HIGH;
  static unsigned long pressStart32 = 0;
  int currentState32 = digitalRead(DOWN_PIN);

  // 1. Detect Press
  if (lastState32 == HIGH && currentState32 == LOW)
  {
    pressStart32 = now;
  }

  // 2. Detect Release
  if (lastState32 == LOW && currentState32 == HIGH)
  {
    unsigned long duration = now - pressStart32;

    // Simple debounce (> 50ms)
    if (duration > 50)
    {
      // --- ACTION: CHANGE BRIGHTNESS ---
      brightnessIndex++;
      if (brightnessIndex > 2) brightnessIndex = 0; // Cycle 0->1->2->0

      int newLevel = brightnessValues[brightnessIndex];
      setBrightness(newLevel);

      preferences.putInt("brightIdx", brightnessIndex);
      Serial.print("Action: Brightness Level ");
      Serial.println(newLevel);
    }
  }
  lastState32 = currentState32;

  // Small delay to prevent CPU hogging and assist debounce
  vTaskDelay(pdMS_TO_TICKS(20));
}