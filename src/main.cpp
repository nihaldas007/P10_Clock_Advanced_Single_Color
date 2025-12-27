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
#define TRIGGER_PIN 32 // The BOOT button
#define RTC_POWERPIN 26 // The BOOT button
Preferences preferences; // NEW: Create preferences object

// Panel OE / Brightness
const int PANEL_OE = 4;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 8000;
const int PWM_RES = 8;


// --- NEW VARIABLES FOR BUTTON & BRIGHTNESS ---
int brightnessIndex = 0;             // 0=Low, 1=Mid, 2=High
int brightnessValues[] = {5, 30, 50}; // The 3 levels you requested
int clickCount = 0;                  // Counts how many times button is clicked
unsigned long lastClickTime = 0;     // Timer for double click speed
const int DOUBLE_CLICK_GAP = 400;    // Time (ms) to wait for a second click

// ... existing forward declarations ...

// FreeRTOS task handles
TaskHandle_t refreshTaskHandle = NULL;
TaskHandle_t clockTaskHandle = NULL;
TaskHandle_t ntpTaskHandle = NULL;

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
void backgroundSyncTask(void* pvParameters);
// ------------------- Refresh Task -------------------
void refreshDisplay(void *pvParameters)
{
  for (;;)
  {
    dmd.scanDisplayBySPI();
    vTaskDelay(1 / portTICK_PERIOD_MS);
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

// ------------------- WiFiManager Callback -------------------
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  dmd.clearScreen(true);
  dmd.selectFont(SystemFont5x7);
  dmd.drawString(2, 0, "SETUP", 5, GRAPHICS_NORMAL);
  dmd.drawString(5, 9, "WIFI", 4, GRAPHICS_NORMAL);
}
// ------------------- BACKGROUND WIFI/NTP TASK -------------------
// Tries to connect silently. If fail, just exits. Does NOT open Portal.
void backgroundSyncTask(void* pvParameters) {
  Serial.println("[Task] Attempting background connection...");
  
  // Use standard WiFi begin (uses credentials stored by WiFiManager previously)
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  // Wait up to 15 seconds for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[Task] WiFi Connected! Fetching NTP...");
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
    
    struct tm timeinfo;
    bool timeReceived = false;
    
    // Try to get time
    for(int i=0; i<10; i++) {
        if (getLocalTime(&timeinfo)) {
            timeReceived = true;
            break;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (timeReceived) {
      Serial.println("[Task] Time synced. Updating RTC.");
      rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    } else {
      Serial.println("[Task] NTP Unreachable.");
    }
  } else {
    Serial.println("[Task] WiFi not found or credentials missing. Running offline.");
  }

  // Task done, delete itself to free memory
  ntpTaskHandle = NULL;
  vTaskDelete(NULL);
}
// ------------------- Setup -------------------
void setup()
{
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(RTC_POWERPIN, OUTPUT);
  digitalWrite(RTC_POWERPIN,HIGH);
  delay(500);
  // 1. Init Preferences
  preferences.begin("clock-app", false);// RESTORE MODE
  currentMode = preferences.getInt("mode", 0);// RESTORE BRIGHTNESS (NEW)
  brightnessIndex = preferences.getInt("brightIdx", 0); // Safety check: ensure index is 0-2
  if(brightnessIndex < 0 || brightnessIndex > 2) brightnessIndex = 0;

  Serial.print("Restored Mode: "); Serial.println(currentMode);
  Serial.print("Restored Brightness: "); Serial.println(brightnessValues[brightnessIndex]);

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
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    dmd.clearScreen(true);
    dmd.selectFont(SystemFont5x7);
    dmd.drawString(0, 4, "NO RTC", 6, GRAPHICS_NORMAL);
    while (1);
  }

  if (rtc.lostPower()) {
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
  static int lastState = HIGH;
  static unsigned long pressStart = 0;
  int currentState = digitalRead(TRIGGER_PIN);
  unsigned long now = millis();

  // --- 1. DETECT PRESS ---
  if (lastState == HIGH && currentState == LOW)
  {
    pressStart = now; // Record when button went down
  }

  // --- 2. DETECT RELEASE ---
  if (lastState == LOW && currentState == HIGH)
  {
    unsigned long duration = now - pressStart;

    if (duration > 3000)
    {
      Serial.println("Long Press: Entering WiFi Config Portal");
      
      // Stop other tasks
      if (clockTaskHandle != NULL) vTaskDelete(clockTaskHandle);
      if (ntpTaskHandle != NULL) vTaskDelete(ntpTaskHandle);
      
      dmd.clearScreen(true);
      dmd.selectFont(SystemFont5x7);
      dmd.drawString(5, 0, "WIFI", 4, GRAPHICS_NORMAL);
      dmd.drawString(1, 8, "SETUP", 5, GRAPHICS_NORMAL);
      
      WiFiManager wm;
      wm.setConfigPortalTimeout(120); // Close after 2 mins if no one connects
      
      // This will block until user connects and saves, or timeout
      if (!wm.startConfigPortal("Nihal_Clock")) {
        Serial.println("Setup timed out");
      } else {
        Serial.println("WiFi Saved");
        // Optional: Sync time immediately here if connected
      } 
      // Restart to apply everything clean
      ESP.restart(); 
      clickCount = 0; // Reset any pending clicks
    }
    else if (duration > 50) 
    {
      clickCount++;
      lastClickTime = now;
    }
  }

  // --- 3. HANDLE CLICKS AFTER TIMEOUT ---
  // If we have clicks waiting, and enough time has passed (400ms) 
  // to ensure the user isn't clicking again:
  if (clickCount > 0 && (now - lastClickTime > DOUBLE_CLICK_GAP))
  {
    if (clickCount == 1)
    {
      // === SINGLE CLICK: CHANGE CLOCK MODE ===
      currentMode++;
      if (currentMode > 7) currentMode = 0;
      
      preferences.putInt("mode", currentMode); // Save Mode
      Serial.println("Action: Switch Clock Mode");
      changeClockMode(currentMode);
    }
    else if (clickCount >= 2)
    {
      // === DOUBLE CLICK: CHANGE BRIGHTNESS ===
      brightnessIndex++;
      if (brightnessIndex > 2) brightnessIndex = 0; // Cycle 0->1->2->0
      
      int newLevel = brightnessValues[brightnessIndex]; // Get 5, 50, or 100
      setBrightness(newLevel);
      
      // Save Brightness to Memory
      preferences.putInt("brightIdx", brightnessIndex); 
      Serial.print("Action: Brightness Level ");
      Serial.println(newLevel);
      
      // Optional: Visual Feedback on screen
      // (Flash the brightness level briefly if you want)
    }

    // Reset count for next time
    clickCount = 0;
  }

  lastState = currentState;
  vTaskDelay(20 / portTICK_PERIOD_MS);
}