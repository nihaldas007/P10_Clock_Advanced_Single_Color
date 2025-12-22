//......................ESP32 only 1.0.6 Board (Preferred)............//
#include <Arduino.h>
#include <DMD32.h>
#include <WiFi.h>
#include <WiFiManager.h> // Install "WiFiManager" by tzapu
#include <SPI.h>
#include <time.h>
#include <Preferences.h> // NEW: For saving settings
#include "Clocks.h"

// Pins
#define TRIGGER_PIN 0 // The BOOT button
Preferences preferences; // NEW: Create preferences object

// Panel OE / Brightness
const int PANEL_OE = 4;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;
int brightnessNumber = 5;

// FreeRTOS task handles
TaskHandle_t refreshTaskHandle = NULL;
TaskHandle_t clockTaskHandle = NULL;

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

// ------------------- Setup -------------------
void setup()
{
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // 1. Init Preferences (Storage)
  preferences.begin("clock-app", false);
  // Read saved mode, default to 0 if nothing saved
  currentMode = preferences.getInt("mode", 0);
  Serial.print("Restored Clock Mode: ");
  Serial.println(currentMode);

  // 2. PWM & Brightness
  pinMode(PANEL_OE, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PANEL_OE, PWM_CHANNEL);
  setBrightness(brightnessNumber);

  // 3. Start Display Refresh (Core 0)
  xTaskCreatePinnedToCore(refreshDisplay, "refreshDisplay", 4096, NULL, 10, &refreshTaskHandle, 1);

  // 4. Show "INIT"
  dmd.clearScreen(true);
  dmd.selectFont(SystemFont5x7);
  dmd.drawString(4, 4, "INIT", 4, GRAPHICS_NORMAL);

  // 5. WiFi Manager
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  wm.setConfigPortalTimeout(180);

  if (!wm.autoConnect("Clock_Setup"))
  {
    Serial.println("Failed to connect, restarting...");
    ESP.restart();
  }

  Serial.println("WiFi Connected!");
  dmd.clearScreen(true);
  dmd.drawString(2, 4, "TIME", 4, GRAPHICS_NORMAL);

  // 6. Time Sync
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo, 0) && retry < 10)
  {
    dmd.drawString(25, 4, ".", 1, GRAPHICS_NORMAL);
    delay(500);
    dmd.drawString(25, 4, " ", 1, GRAPHICS_NORMAL);
    delay(500);
    retry++;
  }

  // 7. Start the saved clock mode
  changeClockMode(currentMode);
}

// ------------------- Main Loop -------------------
void loop()
{
  static int lastState = HIGH;
  static unsigned long pressStart = 0;
  int currentState = digitalRead(TRIGGER_PIN);

  // Button Pressed
  if (lastState == HIGH && currentState == LOW)
  {
    pressStart = millis();
  }

  // Button Released
  if (lastState == LOW && currentState == HIGH)
  {
    unsigned long duration = millis() - pressStart;

    if (duration > 3000)
    {
      // --- LONG PRESS: RESET WIFI ---
      dmd.clearScreen(true);
      dmd.selectFont(SystemFont5x7);
      dmd.drawString(2, 4, "RESET", 5, GRAPHICS_NORMAL);
      Serial.println("Resetting WiFi Settings...");

      if (clockTaskHandle != NULL)
        vTaskDelete(clockTaskHandle);

      WiFiManager wm;
      wm.resetSettings();
      delay(1000);
      ESP.restart();
    }
    else if (duration > 50)
    {
      // --- SHORT PRESS: SWITCH CLOCK ---
      currentMode++;
      if (currentMode > 7)
        currentMode = 0;

      // SAVE THE NEW MODE TO MEMORY
      preferences.putInt("mode", currentMode);
      Serial.print("Saved New Mode: ");
      Serial.println(currentMode);

      changeClockMode(currentMode);
    }
  }

  lastState = currentState;
  vTaskDelay(20 / portTICK_PERIOD_MS);
}
