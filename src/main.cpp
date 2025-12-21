//......................ESP32 only 1.0.6 Board (Preferred)............//
#include <Arduino.h>
#include <DMD32.h>
#include <WiFi.h>
#include <WiFiManager.h>  // Install "WiFiManager" by tzapu
#include <SPI.h>
#include <time.h>
#include <Preferences.h>  // NEW: For saving settings

// --- Config ---
// Bangladesh Time (UTC +6)
const long gmtOffset_sec = 6 * 3600;
const int daylightOffset_sec = 0;

// NTP Servers
const char* ntpServer1 = "time.google.com";
const char* ntpServer2 = "time.nist.gov";
const char* ntpServer3 = "pool.ntp.org";

// Pins
#define TRIGGER_PIN 0  // The BOOT button

// Fonts
#include "fonts/SystemFont5x7.h"
#include "fonts/Font_12x6.h"
#include "fonts/Font5x7Nbox.h"

DMD dmd(1, 1);
Preferences preferences; // NEW: Create preferences object

// Variables
int _hour12, _hour24, _minute, _second;
int currentMode = 0; // 0=Clock1, 1=Clock2, 2=Clock3

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
void Clock1Task(void* pvParameters);
void Clock2Task(void* pvParameters);
void Clock3Task(void* pvParameters);
void Clock4Task(void* pvParameters);
void Clock5Task(void* pvParameters);
void Clock6Task(void* pvParameters);
// ------------------- Refresh Task -------------------
void refreshDisplay(void* pvParameters) {
  for (;;) {
    dmd.scanDisplayBySPI();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// ------------------- Brightness -------------------
void setBrightness(int level) {
  level = constrain(level, 0, 255);
  ledcWrite(PWM_CHANNEL, map(level, 0, 255, 0, (1 << PWM_RES) - 1));
}

// ------------------- Helper: Get Time -------------------
bool myGetLocalTime(struct tm* timeinfo) {
  return getLocalTime(timeinfo, 100);
}

// ------------------- Helper: Switch Clock Mode -------------------
void changeClockMode(int mode) {
  // 1. Delete the currently running clock task if it exists
  if (clockTaskHandle != NULL) {
    vTaskDelete(clockTaskHandle);
    clockTaskHandle = NULL;
  }

  // 2. Clear Screen
  dmd.clearScreen(true);

  // 3. Start the new task
  if (mode == 0) {
    xTaskCreatePinnedToCore(Clock1Task, "Clock1", 4096, NULL, 1, &clockTaskHandle, 1);
  } 
  else if (mode == 1) {
    xTaskCreatePinnedToCore(Clock2Task, "Clock2", 4096, NULL, 1, &clockTaskHandle, 1);
  } 
  else if (mode == 2) {
    xTaskCreatePinnedToCore(Clock3Task, "Clock3", 4096, NULL, 1, &clockTaskHandle, 1);
  }
  else if (mode == 3) {
    xTaskCreatePinnedToCore(Clock4Task, "Clock4", 4096, NULL, 1, &clockTaskHandle, 1);
  }
  else if (mode == 4) {
    xTaskCreatePinnedToCore(Clock5Task, "Clock5", 4096, NULL, 1, &clockTaskHandle, 1);
  }
  else if (mode == 5) {
    xTaskCreatePinnedToCore(Clock6Task, "Clock6", 4096, NULL, 1, &clockTaskHandle, 1);
  }
}

// ------------------- WiFiManager Callback -------------------
void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered config mode");
  dmd.clearScreen(true);
  dmd.selectFont(SystemFont5x7);
  dmd.drawString(2, 0, "SETUP", 5, GRAPHICS_NORMAL);
  dmd.drawString(5, 9, "WIFI", 4, GRAPHICS_NORMAL);
}

// ================================================================
//                        CLOCK TASKS
// ================================================================

// --- Clock 1 ---
void Clock1Task(void* pvParameters) {
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3];
  struct tm timeinfo;

  for (;;) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo)) {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font12x6);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0) _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(1, 2, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, 2, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0) {
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
          dmd.drawFilledBox(15, 10, 16, 11, GRAPHICS_OR);
        } else {
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
          dmd.drawFilledBox(15, 10, 16, 11, GRAPHICS_NOR);
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --- Clock 2 ---
void Clock2Task(void* pvParameters) {
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  struct tm timeinfo;
  const char* dayNames[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

  for (;;) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo)) {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0) _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, -1, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0) {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_OR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
        } else {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_NOR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
        }

        dmd.selectFont(System5x7);
        dmd.drawString(0, 9, dayNames[timeinfo.tm_wday], 3, GRAPHICS_NORMAL);

        dmd.selectFont(Font5x7Nbox);
        sprintf(dateStr, "%02d", timeinfo.tm_mday);
        dmd.drawString(20, 8, dateStr, 2, GRAPHICS_NORMAL);
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --- Clock 3 ---
void Clock3Task(void* pvParameters) {
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  struct tm timeinfo;
  const char* dayNames[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

  for (;;) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo)) {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0) _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(0, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(0, 8, mn, 2, GRAPHICS_NORMAL);
        
        dmd.selectFont(System5x7);
        dmd.drawFilledBox(13, 0, 13, 15, GRAPHICS_OR);
        dmd.drawString(15, 0, dayNames[timeinfo.tm_wday], 3, GRAPHICS_NORMAL);

        dmd.selectFont(Font5x7Nbox);
        sprintf(dateStr, "%02d", timeinfo.tm_mday);
        dmd.drawString(18, 8, dateStr, 2, GRAPHICS_NORMAL);
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
// --- Clock 4 ---
void Clock4Task(void* pvParameters) {
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  struct tm timeinfo;
  
  // CHANGED: Defined month names instead of day names
  const char* monthNames[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

  for (;;) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo)) {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0) _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(0, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(0, 8, mn, 2, GRAPHICS_NORMAL);
        
        dmd.selectFont(System5x7);
        dmd.drawFilledBox(13, 0, 13, 15, GRAPHICS_OR);
        
        // CHANGED: Now printing Month Name based on tm_mon
        dmd.drawString(15, 0, monthNames[timeinfo.tm_mon], 3, GRAPHICS_NORMAL);

        dmd.selectFont(Font5x7Nbox);
        sprintf(dateStr, "%02d", timeinfo.tm_mday);
        dmd.drawString(18, 8, dateStr, 2, GRAPHICS_NORMAL);
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
//...............Clock5...................//
void Clock5Task(void* pvParameters) {
  const long interval = 1000;          // Update clock every 1 second
  const long scrollInterval = 100;      // Scroll speed (lower is faster)
  
  unsigned long previousMillis = 0;
  unsigned long lastScrollMillis = 0;
  
  // Buffers
  char hr_24[3], mn[3];
  char dateScrollBuffer[80]; // Buffer for long date string
  
  // Scroll Variables
  int scrollX = 32;          // Start off-screen to the right
  int textWidth = 0;         // Will be calculated based on the text
  int currentDay = -1;       // To detect date changes
  
  struct tm timeinfo;

  const char* fullDayNames[] = { 
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" 
  };
  const char* fullMonthNames[] = { 
    "January", "February", "March", "April", "May", "June", 
    "July", "August", "September", "October", "November", "December" 
  };

  for (;;) {
    unsigned long currentMillis = millis();

    // ============================================================
    // 1. CLOCK UPDATE (Runs once per second)
    // ============================================================
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo)) {
        // --- Update Time Variables ---
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        // --- Prepare Date String (Only once per day) ---
        if (timeinfo.tm_mday != currentDay) {
          currentDay = timeinfo.tm_mday;
          
          // Format: "Friday January 01 2025"
          sprintf(dateScrollBuffer, "%s %02d-%s %d", 
                  fullDayNames[timeinfo.tm_wday], 
                  timeinfo.tm_mday,
                  fullMonthNames[timeinfo.tm_mon], 
                  timeinfo.tm_year + 1900);
          
          // --- MANUAL WIDTH CALCULATION (Fix for missing stringWidth) ---
          dmd.selectFont(System5x7);
          textWidth = 0;
          for (int i = 0; i < strlen(dateScrollBuffer); i++) {
            // Add width of character + 1 pixel for spacing
            textWidth += dmd.charWidth(dateScrollBuffer[i]) + 1; 
          }
        }

        // --- Draw Static Clock (Top Row) ---
        dmd.selectFont(Font5x7Nbox); // Select Large Font for Clock
        // dmd.selectFont(System5x7);
        
        _hour12 = _hour24 % 12;
        if (_hour12 == 0) _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, -1, mn, 2, GRAPHICS_NORMAL);

        // Blinking Colon
        if (_second % 2 == 0) {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_OR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
        } else {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_NOR); // Erase dots
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
        }
      }
    }

    // ============================================================
    // 2. SCROLL ANIMATION (Runs every 50ms)
    // ============================================================
    if (currentMillis - lastScrollMillis >= scrollInterval) {
      lastScrollMillis = currentMillis;

      // 1. Clear only the bottom area (Pixels Y=9 to Y=15)
      // FIX: Changed GRAPHICS_OFF to GRAPHICS_NOR (which erases pixels)
      dmd.drawFilledBox(0, 9, 31, 15, GRAPHICS_NOR);

      // 2. Update Position
      scrollX--; 
      
      // If text has gone completely off the left side, reset to right side
      if (scrollX < -textWidth) {
        scrollX = 32; 
      }

      // 3. Draw the Text at the new 'scrollX' position
      dmd.selectFont(System5x7); 
      dmd.drawString(scrollX, 9, dateScrollBuffer, strlen(dateScrollBuffer), GRAPHICS_NORMAL);
    }
    
    // Small delay to prevent watchdog crashing
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}
//==============Clock5===============//
void Clock6Task(void* pvParameters) {
  const long interval = 1000;           
  const long switchInterval = 2000;    // Change text every 2 seconds
  
  unsigned long previousMillis = 0;
  unsigned long lastSwitchMillis = 0;
  
  char hr_24[3], mn[3];
  // We don't need a single global 'bottomBuffer' anymore
  // because we will create specific buffers inside the states.
  
  int displayState = 0; 
  struct tm timeinfo;

  const char* dayNames[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
  const char* monthNames[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

  for (;;) {
    unsigned long currentMillis = millis();

    // ============================================================
    // 1. CLOCK UPDATE (Top Row)
    // ============================================================
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo)) {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        
        _hour12 = _hour24 % 12;
        if (_hour12 == 0) _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, -1, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0) {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_OR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
        } else {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_NOR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
        }
      }
    }

    // ============================================================
    // 2. BOTTOM TEXT SWITCHER (Fully Manual Control)
    // ============================================================
    if (currentMillis - lastSwitchMillis >= switchInterval) {
      lastSwitchMillis = currentMillis;

      // 1. Clear the bottom area
      dmd.drawFilledBox(0, 9, 31, 15, GRAPHICS_NOR);

      dmd.selectFont(System5x7); 

      // --------------------------------------------------------
      // STATE 0: WEEK DAY NAME (e.g., "MON")
      // --------------------------------------------------------
      if (displayState == 0) {
        char weekBuf[5];
        sprintf(weekBuf, "%s", dayNames[timeinfo.tm_wday]);
        
        // >>> CONTROL: Set Position for Week Name <<<
        int x = 8; 
        int y = 9;
        dmd.drawString(x, y, weekBuf, strlen(weekBuf), GRAPHICS_NORMAL);
        
        displayState = 1; 
      } 
      // --------------------------------------------------------
      // STATE 1: DATE and MONTH (Separate but same screen)
      // --------------------------------------------------------
      else if (displayState == 1) {
        
        // --- PART A: Draw Date (e.g., "22") ---
        char dateBuf[3];
        sprintf(dateBuf, "%02d", timeinfo.tm_mday);
        
        // >>> CONTROL: Set Position for Date Number <<<
        dmd.selectFont(Font5x7Nbox);
        int dateX = 0; 
        int dateY = 8;
        dmd.drawString(dateX, dateY, dateBuf, strlen(dateBuf), GRAPHICS_NORMAL);

        // --- PART B: Draw Month (e.g., "DEC") ---
        char monthBuf[4];
        sprintf(monthBuf, "%s", monthNames[timeinfo.tm_mon]);
        
        // >>> CONTROL: Set Position for Month Name <<<
        dmd.selectFont(System5x7);
        int monthX = 15; 
        int monthY = 9;
        dmd.drawString(monthX, monthY, monthBuf, strlen(monthBuf), GRAPHICS_NORMAL);

        displayState = 2; 
      } 
      // --------------------------------------------------------
      // STATE 2: YEAR (e.g., "2025")
      // --------------------------------------------------------
      else {
        dmd.selectFont(Font5x7Nbox); // Different font for year
        char yearBuf[5];
        sprintf(yearBuf, "%d", timeinfo.tm_year + 1900);
        
        // >>> CONTROL: Set Position for Year <<<
        int yearX = 4;
        int yearY = 8; 
        dmd.drawString(yearX, yearY, yearBuf, strlen(yearBuf), GRAPHICS_NORMAL);
        
        displayState = 0; 
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS); 
  }
}


// ------------------- Setup -------------------
void setup() {
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

  if (!wm.autoConnect("Clock_Setup")) {
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
  while (!getLocalTime(&timeinfo, 0) && retry < 10) {
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
void loop() {
  static int lastState = HIGH;
  static unsigned long pressStart = 0;
  int currentState = digitalRead(TRIGGER_PIN);

  // Button Pressed
  if (lastState == HIGH && currentState == LOW) {
    pressStart = millis();
  }

  // Button Released
  if (lastState == LOW && currentState == HIGH) {
    unsigned long duration = millis() - pressStart;

    if (duration > 3000) {
      // --- LONG PRESS: RESET WIFI ---
      dmd.clearScreen(true);
      dmd.selectFont(SystemFont5x7);
      dmd.drawString(2, 4, "RESET", 5, GRAPHICS_NORMAL);
      Serial.println("Resetting WiFi Settings...");
      
      if (clockTaskHandle != NULL) vTaskDelete(clockTaskHandle);
      
      WiFiManager wm;
      wm.resetSettings();
      delay(1000);
      ESP.restart();
    } 
    else if (duration > 50) {
      // --- SHORT PRESS: SWITCH CLOCK ---
      currentMode++;
      if (currentMode > 5) currentMode = 0; 
      
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
