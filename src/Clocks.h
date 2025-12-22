#pragma once


#include <Arduino.h>
#include <DMD32.h>
#include <SPI.h>
#include <time.h>

// Fonts
#include "fonts/SystemFont5x7.h"
#include "fonts/Font_12x6.h"
#include "fonts/Font5x7Nbox.h"
#include "fonts/Font5x7NboxC.h"
#include "fonts/Font6x16.h"
// ================================================================
//                        CLOCK TASKS
// ================================================================
DMD dmd(1, 1);

// Variables
int _hour12, _hour24, _minute, _second;
int currentMode = 0; // 0=Clock1, 1=Clock2, 2=Clock3

// --- Config ---
// Bangladesh Time (UTC +6)
const long gmtOffset_sec = 6 * 3600;
const int daylightOffset_sec = 0;

// NTP Servers
const char *ntpServer1 = "time.google.com";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "pool.ntp.org";

// ------------------- Helper: Get Time -------------------
bool myGetLocalTime(struct tm *timeinfo)
{
  return getLocalTime(timeinfo, 100);
}
// --- Clock 1 ---
void Clock1Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3];
  struct tm timeinfo;

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font12x6);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(1, 2, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, 2, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0)
        {
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
          dmd.drawFilledBox(15, 10, 16, 11, GRAPHICS_OR);
        }
        else
        {
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
          dmd.drawFilledBox(15, 10, 16, 11, GRAPHICS_NOR);
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
// --- Clock 2 ---
void Clock2Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3];
  struct tm timeinfo;

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font6x16);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(1, 0, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, 0, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0)
        {
          dmd.drawFilledBox(15, 2, 16, 3, GRAPHICS_OR);
          dmd.drawFilledBox(15, 12, 16, 13, GRAPHICS_OR);
        }
        else
        {
          dmd.drawFilledBox(15, 2, 16, 3, GRAPHICS_NOR);
          dmd.drawFilledBox(15, 12, 16, 13, GRAPHICS_NOR);
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void Clock3Task(void *pvParameters)
{
  
  char hr_24[3], mn[3];
  char tens_str[2], units_str[2];
  
  struct tm timeinfo;
  int last_second = -1; 

  // --- Configuration ---
  const int fontHeight = 12; 
  const int secX_Tens = 18;  
  const int secX_Units = 25; 
  const int secY = 2;        
  const int gap = 2;         

  for (;;)
  {
    
    if (myGetLocalTime(&timeinfo))
    {
       _second = timeinfo.tm_sec;

       if (_second != last_second) 
       {
          _hour24 = timeinfo.tm_hour;
          _minute = timeinfo.tm_min;

          // 1. PREPARE STRINGS
          dmd.selectFont(Font5x7Nbox);
          _hour12 = _hour24 % 12;
          if (_hour12 == 0) _hour12 = 12;
          sprintf(hr_24, "%02d", _hour12);
          sprintf(mn, "%02d", _minute);

          // 2. DRAW STATIC PART (Hours, Mins, Dots) ONE TIME
          // We do this OUTSIDE the loop to stop flickering
          dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);
          dmd.drawString(3, 8, mn, 2, GRAPHICS_NORMAL);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
          dmd.drawFilledBox(15, 10, 16, 11, GRAPHICS_OR);

          // 3. CALCULATE DIGITS
          int new_tens = _second / 10;
          int new_units = _second % 10;
          
          int old_tens = (last_second == -1) ? new_tens : last_second / 10;
          int old_units = (last_second == -1) ? new_units : last_second % 10;

          // 4. DETERMINE ANIMATION AREA
          // If the Left digit (Tens) is the same, we draw it ONCE and don't touch it again.
          // We start clearing/animating only from the Right digit (Units).
          
          int clearStart_X; 

          dmd.selectFont(Font12x6);

          if (new_tens == old_tens) {
             // Tens didn't change: Draw it static NOW
             sprintf(tens_str, "%d", new_tens);
             dmd.drawString(secX_Tens, secY, tens_str, 1, GRAPHICS_NORMAL); // Use NORMAL to ensure it overwrites cleanly
             
             // Only animate the right side
             clearStart_X = secX_Units; 
          } else {
             // Tens changed: We must animate from the left side
             clearStart_X = secX_Tens;
          }

          // --- ANIMATION LOOP ---
          for (int i = 0; i <= fontHeight + gap; i++) 
          {
            // A. Clear ONLY the area that is moving
            // If tens are static, this only clears X=25 to 31. The '0' at X=18 is safe.
            dmd.drawFilledBox(clearStart_X, 0, 31, 15, GRAPHICS_NOR); 

            dmd.selectFont(Font12x6);

            // B. Animate TENS (Only if changed)
            if (new_tens != old_tens) {
               sprintf(tens_str, "%d", old_tens);
               dmd.drawString(secX_Tens, secY - i, tens_str, 1, GRAPHICS_OR);
               
               sprintf(tens_str, "%d", new_tens);
               dmd.drawString(secX_Tens, (secY + fontHeight + gap) - i, tens_str, 1, GRAPHICS_OR);
            }

            // C. Animate UNITS (Always animate)
            sprintf(units_str, "%d", old_units);
            dmd.drawString(secX_Units, secY - i, units_str, 1, GRAPHICS_OR);
            
            sprintf(units_str, "%d", new_units);
            dmd.drawString(secX_Units, (secY + fontHeight + gap) - i, units_str, 1, GRAPHICS_OR);

            vTaskDelay(20 / portTICK_PERIOD_MS); // Slightly faster to look smoother
          }

          last_second = _second;
       }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
// --- Clock 4 ---
void Clock4Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  struct tm timeinfo;
  const char *dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, -1, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0)
        {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_OR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
        }
        else
        {
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

// --- Clock 5 ---
void Clock5Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  struct tm timeinfo;
  const char *dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

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
void Clock6Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  struct tm timeinfo;

  // CHANGED: Defined month names instead of day names
  const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);
        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

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
void Clock7Task(void *pvParameters)
{
  const long interval = 1000;      // Update clock every 1 second
  const long scrollInterval = 100; // Scroll speed (lower is faster)

  unsigned long previousMillis = 0;
  unsigned long lastScrollMillis = 0;

  // Buffers
  char hr_24[3], mn[3];
  char dateScrollBuffer[80]; // Buffer for long date string

  // Scroll Variables
  int scrollX = 32;    // Start off-screen to the right
  int textWidth = 0;   // Will be calculated based on the text
  int currentDay = -1; // To detect date changes

  struct tm timeinfo;

  const char *fullDayNames[] = {
      "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char *fullMonthNames[] = {
      "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"};

  for (;;)
  {
    unsigned long currentMillis = millis();

    // ============================================================
    // 1. CLOCK UPDATE (Runs once per second)
    // ============================================================
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        // --- Update Time Variables ---
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        // --- Prepare Date String (Only once per day) ---
        if (timeinfo.tm_mday != currentDay)
        {
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
          for (int i = 0; i < strlen(dateScrollBuffer); i++)
          {
            // Add width of character + 1 pixel for spacing
            textWidth += dmd.charWidth(dateScrollBuffer[i]) + 1;
          }
        }

        // --- Draw Static Clock (Top Row) ---
        dmd.selectFont(Font5x7Nbox); // Select Large Font for Clock
        // dmd.selectFont(System5x7);

        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, -1, mn, 2, GRAPHICS_NORMAL);

        // Blinking Colon
        if (_second % 2 == 0)
        {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_OR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
        }
        else
        {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_NOR); // Erase dots
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
        }
      }
    }

    // ============================================================
    // 2. SCROLL ANIMATION (Runs every 50ms)
    // ============================================================
    if (currentMillis - lastScrollMillis >= scrollInterval)
    {
      lastScrollMillis = currentMillis;

      // 1. Clear only the bottom area (Pixels Y=9 to Y=15)
      // FIX: Changed GRAPHICS_OFF to GRAPHICS_NOR (which erases pixels)
      dmd.drawFilledBox(0, 9, 31, 15, GRAPHICS_NOR);

      // 2. Update Position
      scrollX--;

      // If text has gone completely off the left side, reset to right side
      if (scrollX < -textWidth)
      {
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
//==============Clock7===============//
void Clock8Task(void *pvParameters)
{
  const long interval = 1000;
  const long switchInterval = 2000; // Change text every 2 seconds

  unsigned long previousMillis = 0;
  unsigned long lastSwitchMillis = 0;

  char hr_24[3], mn[3];
  // We don't need a single global 'bottomBuffer' anymore
  // because we will create specific buffers inside the states.

  int displayState = 0;
  struct tm timeinfo;

  const char *dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  for (;;)
  {
    unsigned long currentMillis = millis();

    // ============================================================
    // 1. CLOCK UPDATE (Top Row)
    // ============================================================
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      if (myGetLocalTime(&timeinfo))
      {
        _hour24 = timeinfo.tm_hour;
        _minute = timeinfo.tm_min;
        _second = timeinfo.tm_sec;

        dmd.selectFont(Font5x7Nbox);

        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(3, -1, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(18, -1, mn, 2, GRAPHICS_NORMAL);

        if (_second % 2 == 0)
        {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_OR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_OR);
        }
        else
        {
          dmd.drawFilledBox(15, 1, 16, 2, GRAPHICS_NOR);
          dmd.drawFilledBox(15, 4, 16, 5, GRAPHICS_NOR);
        }
      }
    }

    // ============================================================
    // 2. BOTTOM TEXT SWITCHER (Fully Manual Control)
    // ============================================================
    if (currentMillis - lastSwitchMillis >= switchInterval)
    {
      lastSwitchMillis = currentMillis;

      // 1. Clear the bottom area
      dmd.drawFilledBox(0, 9, 31, 15, GRAPHICS_NOR);

      dmd.selectFont(System5x7);

      // --------------------------------------------------------
      // STATE 0: WEEK DAY NAME (e.g., "MON")
      // --------------------------------------------------------
      if (displayState == 0)
      {
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
      else if (displayState == 1)
      {

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
      else
      {
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
