P10 Single Color LED Clock

An advanced, multi-mode digital clock system implemented on an ESP32 microcontroller, driving a 32x16 single-color P10 LED dot-matrix display panel.

This project leverages **FreeRTOS** for smooth multi-tasking, ensuring zero display flicker by isolating high-priority display multiplexing on one CPU core, while handling background WiFi/NTP synchronization on the other. It includes a DS3231 Real-Time Clock (RTC) for battery-backed precision, an **on-screen Interactive Menu System**, and non-volatile storage (NVS) to remember your settings across power cycles.

## 🚀 Key Features

* **Dual-Core FreeRTOS Architecture:** Display multiplexing runs at high priority on Core 1, active clock rendering on Core 1 (lower priority), and background WiFi/NTP syncing on Core 0.
* **On-Screen Interactive Menu:** Set the time, date, or launch the WiFi configuration portal directly from the P10 display without needing external tools or recompilation.
* **8 Interactive Display Modes:** Ranging from standard digital displays to animated rolling seconds (slot-machine effect), scrolling date banners, and multi-state cycling views.
* **Dynamic Special Events System:** Automatically scrolls full-screen customized messages (e.g., "Happy Birthday!") once a minute on configured dates.
* **Automatic Network Time (NTP):** Background WiFi checks and syncs with UTC+6 Bangladesh time automatically.
* **Thread-Safe I2C Access:** DS3231 RTC operations are safely serialized across tasks using FreeRTOS Mutex.
* **Smart Brightness Control:** 8-bit LEDC PWM brightness with 3 manual levels, plus automatic night dimming (drops to minimum brightness between 00:00 and 07:59). Temporarily overrides to normal brightness while in the menu.

---

## 🛠️ Hardware Requirements & Wiring

* **ESP32 Development Board** (esp32dev)
* **P10 Single Color LED Matrix** (32 columns x 16 rows)
* **DS3231 RTC Module** (I2C interface)
* **Two Push Buttons** (UP and DOWN)

### Pin Configuration

| Function / Signal | ESP32 GPIO | Description |
| --- | --- | --- |
| **PANEL_OE** | GPIO 4 | Active LOW Output Enable / LEDC PWM |
| **PANEL_A** | GPIO 16 | Row Select Bit A |
| **PANEL_B** | GPIO 17 | Row Select Bit B |
| **PANEL_CLK (SCK)** | GPIO 18 | VSPI Shift Register Clock |
| **PANEL_SCLK (Latch)** | GPIO 19 | Storage Register Latch Clock |
| **PANEL_R_DATA** | GPIO 23 | VSPI Data Input (MOSI) |
| **UP_PIN (Button 1)** | GPIO 32 | Up Button: Short = Cycle Mode, Long = Menu |
| **DOWN_PIN (Btn 2)** | GPIO 33 | Down Button: Short = Cycle Level / Select |
| **RTC_POWERPIN** | GPIO 5 | VCC power output line for RTC module |
| **RTC SDA** | GPIO 21 | I2C Data line for DS3231 |
| **RTC SCL** | GPIO 22 | I2C Clock line for DS3231 |

---

## ⚙️ Usage & Operation

### Normal Clock Mode

* **Change Display Mode:** Short-press **UP (GPIO 0)** to cycle through the 8 different clock layouts. Your choice is instantly saved to memory.
* **Change Brightness:** Short-press **DOWN (GPIO 33)** to cycle through Low (5), Medium (30), and High (50) brightness levels. *(Note: Between 00:00 and 07:59, the clock automatically forces Low brightness).*

### Interactive Menu System (Mode 100)

To enter the configuration menu, **press and hold the UP button for ~1 second**.

* **Navigate:** Short-press **UP** to cycle through the menu items.
* **Select/Edit:** Short-press **DOWN** to select an item or increment a blinking value.

**Menu Options:**

1. `>WIFI` : Select to immediately launch the captive portal. Connect your phone/PC to the "P10_CLOCK" WiFi network to enter credentials.
2. `>TIME` : Enters manual time editing. The Hour will blink—press DOWN to increase. Press UP to switch to Minutes. Press DOWN to increase. Press UP again to save and exit.
3. `>DATE` : Enters manual date editing. Steps sequentially through Day -> Month -> Year. Press DOWN to increase values and UP to advance. Saves directly to the DS3231 RTC chip.
4. `>EXIT` : Leaves the menu and seamlessly restores your active clock dial.

---

## ⏱️ Display Modes

1. **Large Digital:** 12-hour HH:MM format with 1-second blinking center colon dots.
2. **Medium Digital:** 12-hour HH:MM with double colon dots blinking on even/odd seconds.
3. **Slot Machine Seconds:** HH:MM stacked vertically on the left. The seconds animate with a smooth vertical slot-machine scrolling effect on the right.
4. **Day & Date Split:** Top row HH:MM; bottom row 3-letter weekday + 2-digit day.
5. **Full Details (AM/PM):** Top row HH:MM with AM/PM indicator; bottom row DD.MM + 3-letter weekday.
6. **Vertical Split:** Left side vertical HH/MM; vertical divider line; right side month and day.
7. **Marquee Banner:** Top row static HH:MM; bottom row smooth horizontal scrolling marquee showing the full date string.
8. **Tri-State Switcher:** Top row static HH:MM; bottom row switches every 2 seconds between Weekday, Day+Month, and Year.

**Mode 99: Dynamic Event Scroll** *(Interrupt)*
Triggered automatically at the top of every minute when today's date matches a configured event. It clears the screen, smoothly scrolls a special message, and returns to normal operation.

---

## 📅 Configuring Special Events

You can add birthdays, anniversaries, or custom reminders by modifying the array in `src/events.h` prior to compiling.

```cpp
// 1 = every min, 30 = every 30 mins, 60 = every 1 hour
const int EVENT_INTERVAL_MINUTES = 60; 

// Scroll speed: 20 = Fast, 40 = Normal, 80 = Slow
const int EVENT_SCROLL_SPEED_MS = 40; 

SpecialEvent specialEvents[] = {
    {22, 7, "Happy Birthday Arpita !!"},     // Scrolls on July 22
    {15, 8, "Happy Anniversary Mom & Dad"},  // Scrolls on August 15
    {1, 1, "Happy New Year !!"}              // Scrolls on January 1
};

```

*To disable the feature, leave the array empty: `SpecialEvent specialEvents[] = {};*`

---

## 💻 Build & Upload Instructions

This project is configured for **PlatformIO**.

1. Install **VS Code** with the **PlatformIO IDE** extension.
2. Clone this repository and open the folder in VS Code.
3. The `platformio.ini` file will automatically download the required libraries (`WiFiManager` and `RTClib`).
4. Connect your ESP32 board via USB.
5. Click **Build** (checkmark icon) to compile.
6. Click **Upload** (right-arrow icon) to flash the firmware.
7. Open the Serial Monitor at `115200 baud` to view boot logs and FreeRTOS diagnostics.
