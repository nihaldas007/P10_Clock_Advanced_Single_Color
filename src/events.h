#pragma once
#include <Arduino.h>

// =========================================================================
// EVENT BEHAVIOR SETTINGS
// =========================================================================
// Interval between event messages (in minutes). 
// 1 = every minute, 30 = every 30 mins, 60 = every 1 hour, 120 = every 2 hours
const int EVENT_INTERVAL_MINUTES = 1; 

// Scroll speed (in milliseconds per pixel shift).
// Lower number = FASTER. Higher number = SLOWER.
// Examples: 20 = Fast, 40 = Normal, 80 = Slow
const int EVENT_SCROLL_SPEED_MS = 40; 

// =========================================================================
// SPECIAL EVENTS CONFIGURATION
// You can add, remove, or modify events in this list dynamically.
// Format: { Day, Month, "Message to Scroll" }
// =========================================================================

struct SpecialEvent {
    int day;
    int month;
    const char* message;
};

SpecialEvent specialEvents[] = {
    {21, 7, "Happy Birthday Arpita !!"},     // Example: July 22
    {15, 8, "Happy Anniversary Mom & Dad"},  // Example: August 15
    {1, 1, "Happy New Year !!"}              // Example: January 1
    // Add more members or events here...
};

const int numSpecialEvents = sizeof(specialEvents) / sizeof(specialEvents[0]);
const char* currentEventMessage = "";
