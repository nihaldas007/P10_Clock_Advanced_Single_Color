//......................ESP32 only 1.0.6 Board (Preferred)............//
#include <Arduino.h>
#include "functions/functions.h"

// Define the global menu variables
MenuState menuState = MENU_IDLE;
int currentMenuItem = 0;
int menuEditHH = 0;
int menuEditMM = 0;
int menuEditDD = 1;
int menuEditMon = 1;
int menuEditYY = 2024;
// Pins
#define UP_PIN 33       // The BOOT button
#define DOWN_PIN 0     // The BOOT button
#define RTC_POWERPIN 5 // The BOOT button

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
    // while (1)
    //   ;
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
  // EVENT TRIGGER CHECK (Uses EVENT_INTERVAL_MINUTES)
  // ==========================================
  static int lastEventMinute = -1;
  static int minutesSinceLastTrigger = 9999; // Starts high to trigger immediately on special days

  if (_second == 0 && _minute != lastEventMinute) {
      lastEventMinute = _minute;
      minutesSinceLastTrigger++;
      
      if (minutesSinceLastTrigger >= EVENT_INTERVAL_MINUTES) {
          DateTime rtcNow;
          if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
              rtcNow = rtc.now();
              xSemaphoreGive(i2cMutex);
              
              if (rtcNow.year() > 2000) {
                  for (int i = 0; i < numSpecialEvents; i++) {
                      if (rtcNow.day() == specialEvents[i].day && rtcNow.month() == specialEvents[i].month) {
                          currentEventMessage = specialEvents[i].message;
                          Serial.print("Triggering Special Event: ");
                          Serial.println(currentEventMessage);
                          minutesSinceLastTrigger = 0; // Reset timer
                          changeClockMode(99); // 99 triggers EventScrollTask
                          break;
                      }
                  }
              }
          }
      }
  }

  // ==========================================
  // BUTTON UP_PIN: MENU (Long) & NEXT/UP (Short)
  // ==========================================
  static int lastStateUp = HIGH;
  static unsigned long pressStartUp = 0;
  int currentStateUp = digitalRead(UP_PIN);
  
  if (lastStateUp == HIGH && currentStateUp == LOW) {
      pressStartUp = now;
  }
  if (lastStateUp == LOW && currentStateUp == HIGH) {
      unsigned long duration = now - pressStartUp;
      if (duration > 800) {
          // Long Press: Toggle Menu
          toggleMenu();
      } else if (duration > 50) {
          // Short Press
          if (menuState != MENU_IDLE) {
              menuUpPress();
          } else {
              modeChange(); // Normal clock operation
          }
      }
  }
  lastStateUp = currentStateUp;

  // ==========================================
  // AUTO BRIGHTNESS
  // ==========================================
  if(_hour24 >= 0 && _hour24 <= 7)
      setBrightness(5);
  else if(_hour24 >= 8 && _hour24 <= 23)
      setBrightness(brightnessValues[brightnessIndex]);

  // ==========================================
  // BUTTON DOWN_PIN: SELECT/DOWN (Short)
  // ==========================================
  static int lastStateDown = HIGH;
  static unsigned long pressStartDown = 0;
  int currentStateDown = digitalRead(DOWN_PIN);
  
  if (lastStateDown == HIGH && currentStateDown == LOW) {
      pressStartDown = now;
  }
  if (lastStateDown == LOW && currentStateDown == HIGH) {
      unsigned long duration = now - pressStartDown;
      if (duration > 50) {
          if (menuState != MENU_IDLE) {
              menuDownPress();
          } else {
              bright(); // Normal clock operation
          }
      }
  }
  lastStateDown = currentStateDown;
  // Small delay to prevent CPU hogging and assist debounce
  vTaskDelay(pdMS_TO_TICKS(20));
}