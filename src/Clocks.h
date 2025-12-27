#pragma once

#include <Arduino.h>
#include <DMD32.h>
#include <SPI.h>
#include <time.h>
#include <RTClib.h>

// Fonts
#include "fonts/SystemFont5x7.h"
#include "fonts/Font_12x6.h"
#include "fonts/Font5x7Nbox.h"
#include "fonts/Font5x7NboxC.h"
#include "fonts/Font6x16.h"
#include "fonts/SystemFont3x5.h"
#include "fonts/Font5x10Nbox.h"
#include "fonts/Font5x10Sbox.h"
// ================================================================
//                        CLOCK TASKS
// ================================================================
DMD dmd(1, 1);
RTC_DS3231 rtc;

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

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      DateTime now = rtc.now();
      _hour24 = now.hour();
      _minute = now.minute();
      _second = now.second();

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
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
// --- Clock 2 ---
void Clock2Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3];

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      DateTime now = rtc.now();
      _hour24 = now.hour();
      _minute = now.minute();
      _second = now.second();

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
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void Clock3Task(void *pvParameters)
{
  char hr_24[3], mn[3];
  char tens_str[2], units_str[2];

  int last_second = -1;

  // --- Configuration ---
  const int fontHeight = 12;
  const int secX_Tens = 18;
  const int secX_Units = 25;
  const int secY = 2;
  const int gap = 2;

  for (;;)
  {
    DateTime now = rtc.now();
    _second = now.second();

    if (_second != last_second)
    {
      _hour24 = now.hour();
      _minute = now.minute();

      // 1. PREPARE STRINGS
      dmd.selectFont(Font5x7Nbox);
      _hour12 = _hour24 % 12;
      if (_hour12 == 0)
        _hour12 = 12;
      sprintf(hr_24, "%02d", _hour12);
      sprintf(mn, "%02d", _minute);

      // 2. DRAW STATIC PART
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
      int clearStart_X;

      dmd.selectFont(Font12x6);

      if (new_tens == old_tens)
      {
        // Tens didn't change: Draw it static NOW
        sprintf(tens_str, "%d", new_tens);
        dmd.drawString(secX_Tens, secY, tens_str, 1, GRAPHICS_NORMAL);
        clearStart_X = secX_Units;
      }
      else
      {
        clearStart_X = secX_Tens;
      }

      // --- ANIMATION LOOP ---
      // 15 steps * 60ms = 900ms Total Duration
      for (int i = 0; i <= fontHeight + gap; i++)
      {
        dmd.drawFilledBox(clearStart_X, 0, 31, 15, GRAPHICS_NOR);

        dmd.selectFont(Font12x6);

        // B. Animate TENS (Only if changed)
        if (new_tens != old_tens)
        {
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

        // >>> UPDATED DELAY HERE <<<
        // Increased to 60ms to make animation take ~0.9 seconds total
        vTaskDelay(60 / portTICK_PERIOD_MS);
      }

      last_second = _second;
    }
    // Reduced outer delay slightly to ensure we catch the next second immediately after animation
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
// --- Clock 4 ---
void Clock4Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];
  const char *dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      DateTime now = rtc.now();
      _hour24 = now.hour();
      _minute = now.minute();
      _second = now.second();

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
        dmd.drawString(0, 9, dayNames[now.dayOfTheWeek()], 3, GRAPHICS_NORMAL);

        dmd.selectFont(Font5x7Nbox);
        sprintf(dateStr, "%02d", now.day());
        dmd.drawString(20, 8, dateStr, 2, GRAPHICS_NORMAL);
      }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void Clock5Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], date[3], month[3];
  const char *dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      DateTime now = rtc.now();
      _hour24 = now.hour();
      _minute = now.minute();
      _second = now.second();

        // --- TOP ROW: TIME ---
        dmd.selectFont(Font5x10Nbox); // Large font for numbers
        _hour12 = _hour24 % 12;
        if (_hour12 == 0)
          _hour12 = 12;

        sprintf(hr_24, "%02d", _hour12);
        dmd.drawString(0, 0, hr_24, 2, GRAPHICS_NORMAL);

        sprintf(mn, "%02d", _minute);
        dmd.drawString(15, 0, mn, 2, GRAPHICS_NORMAL);

        // --- AM/PM ADDITION ---
        // We switch to the smaller font to fit 'A' or 'P' on the edge (x=28)
        dmd.selectFont(SystemFont3x5);
        if (_hour24 >= 12)
        {
          dmd.drawString(27, 3, "P", 1, GRAPHICS_NORMAL); // Draw 'P' at bottom-right of numbers
        }
        else
        {
          dmd.drawString(28, 3, "A", 1, GRAPHICS_NORMAL); // Draw 'A'
        }

        // --- BLINKING COLON ---
        // Switch logic back to fill boxes (Graphics mode doesn't depend on font, but good to keep organized)
        if (_second % 2 == 0)
        {
          dmd.drawFilledBox(12, 1, 13, 2, GRAPHICS_OR);
          dmd.drawFilledBox(12, 7, 13, 8, GRAPHICS_OR);
        }
        else
        {
          dmd.drawFilledBox(12, 1, 13, 2, GRAPHICS_NOR);
          dmd.drawFilledBox(12, 7, 13, 8, GRAPHICS_NOR);
        }

        // --- BOTTOM ROW: DATE.MONTH & WEEKDAY ---
        dmd.selectFont(SystemFont3x5); // Ensure small font is selected for date

        // 1. Format Date as DD.MM
        sprintf(date, "%02d", now.day());
        dmd.drawString(0, 11, date, 3, GRAPHICS_NORMAL);

        dmd.drawFilledBox(8, 15, 8, 15, GRAPHICS_OR); // The dot

        sprintf(month, "%02d", now.month() + 1);
        dmd.drawString(10, 11, month, 3, GRAPHICS_NORMAL);

        // 2. Draw Week Day Name
        dmd.drawString(21, 11, dayNames[now.dayOfTheWeek()], 3, GRAPHICS_NORMAL);
      }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
// --- Clock 6 ---
void Clock6Task(void *pvParameters)
{
  const long interval = 1000;
  unsigned long previousMillis = 0;
  char hr_24[3], mn[3], dateStr[3];

  // CHANGED: Defined month names instead of day names
  const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  for (;;)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      DateTime now = rtc.now();
      _hour24 = now.hour();
      _minute = now.minute();

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
        dmd.drawString(15, 0, monthNames[now.month() - 1], 3, GRAPHICS_NORMAL);

        dmd.selectFont(Font5x7Nbox);
        sprintf(dateStr, "%02d", now.day());
        dmd.drawString(18, 8, dateStr, 2, GRAPHICS_NORMAL);
      }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
//...............Clock7...................//
void Clock7Task(void* pvParameters) {
  const long interval = 1000;       
  const long scrollInterval = 100;  
  unsigned long previousMillis = 0;
  unsigned long lastScrollMillis = 0;

  char hr_24[3], mn[3];
  char dateScrollBuffer[80];

  int scrollX = 32;     
  int textWidth = 0;    
  int currentDay = -1;  

  const char* fullDayNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
  };
  const char* fullMonthNames[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };

  for (;;) {
    unsigned long currentMillis = millis();

    // 1. CLOCK UPDATE
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      DateTime now = rtc.now();
      _hour24 = now.hour();
      _minute = now.minute();
      _second = now.second();

      if (now.day() != currentDay) {
        currentDay = now.day();
        sprintf(dateScrollBuffer, "%s %02d-%s %d",
                fullDayNames[now.dayOfTheWeek()],
                now.day(),
                fullMonthNames[now.month() - 1], 
                now.year());

        dmd.selectFont(System5x7);
        textWidth = 0;
        for (int i = 0; i < strlen(dateScrollBuffer); i++) {
          textWidth += dmd.charWidth(dateScrollBuffer[i]) + 1;
        }
      }

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

    // 2. SCROLL ANIMATION
    if (currentMillis - lastScrollMillis >= scrollInterval) {
      lastScrollMillis = currentMillis;
      dmd.drawFilledBox(0, 9, 31, 15, GRAPHICS_NOR); 
      scrollX--;
      if (scrollX < -textWidth) {
        scrollX = 32;
      }
      dmd.selectFont(System5x7);
      dmd.drawString(scrollX, 9, dateScrollBuffer, strlen(dateScrollBuffer), GRAPHICS_NORMAL);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
//==============Clock8===============//
void Clock8Task(void *pvParameters)
{
  const long interval = 1000;
  const long switchInterval = 2000; // Change text every 2 seconds

  unsigned long previousMillis = 0;
  unsigned long lastSwitchMillis = 0;

  char hr_24[3], mn[3];
  int displayState = 0;

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

      // GET TIME FROM RTC
      DateTime now = rtc.now(); 
      _hour24 = now.hour();
      _minute = now.minute();
      _second = now.second();

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

    // ============================================================
    // 2. BOTTOM TEXT SWITCHER (RTC Based)
    // ============================================================
    if (currentMillis - lastSwitchMillis >= switchInterval)
    {
      lastSwitchMillis = currentMillis;
      
      // Get time again for bottom section ensuring sync
      DateTime now = rtc.now();

      // 1. Clear the bottom area
      dmd.drawFilledBox(0, 9, 31, 15, GRAPHICS_NOR);

      // --------------------------------------------------------
      // STATE 0: WEEK DAY NAME (e.g., "MON")
      // --------------------------------------------------------
      if (displayState == 0)
      {
        dmd.selectFont(System5x7);
        char weekBuf[5];
        sprintf(weekBuf, "%s", dayNames[now.dayOfTheWeek()]);

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
        sprintf(dateBuf, "%02d", now.day());

        // >>> CONTROL: Set Position for Date Number <<<
        dmd.selectFont(Font5x7Nbox);
        int dateX = 0;
        int dateY = 8;
        dmd.drawString(dateX, dateY, dateBuf, strlen(dateBuf), GRAPHICS_NORMAL);

        // --- PART B: Draw Month (e.g., "DEC") ---
        char monthBuf[4];
        // Note: RTC month is 1-12, Array is 0-11. So subtract 1.
        sprintf(monthBuf, "%s", monthNames[now.month() - 1]);

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
        sprintf(yearBuf, "%d", now.year());

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