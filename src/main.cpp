//......................ESP32 only 1.0.6 Board (Preferred)............//
#include <Arduino.h>
#include "Clocks.h"
#include "functions/functions.h"
// Pins
#define UP_PIN 32       // The BOOT button
#define DOWN_PIN 33     // The BOOT button
#define RTC_POWERPIN 26 // The BOOT button

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
  if (i2cMutex == NULL)
  {
    Serial.println("ERROR: Failed to create I2C mutex");
    while (1)
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    dmd.clearScreen(true);
    dmd.selectFont(SystemFont5x7);
    dmd.drawString(0, 4, "NO RTC", 6, GRAPHICS_NORMAL);
    while (1)
      ;
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
    if (duration > 3000)wifimanager();
    else if (duration > 50)modeChange();
  }
  lastState33 = currentState33;
  // ==========================================
  // BUTTON 32: BRIGHTNESS (Short Press)
  // ==========================================

  if(_hour24 >= 0 && _hour24 <= 7)
    setBrightness(5);
  else if(_hour24 >= 8 && _hour24 <= 23)
    setBrightness(brightnessValues[brightnessIndex]);
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
    if (duration > 50)bright();
  }

  lastState32 = currentState32;
  // Small delay to prevent CPU hogging and assist debounce
  vTaskDelay(pdMS_TO_TICKS(20));
}