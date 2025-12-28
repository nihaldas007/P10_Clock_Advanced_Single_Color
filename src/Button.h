class Button {
  private:
    int _pin;
    unsigned long _threshold;

  public:
    // Constructor
    Button(int pin, unsigned long threshold = 250) {
      _pin = pin;
      _threshold = threshold;
    }

    // 0 = No Press, 1 = Short Press (< 250ms), 2 = Long Press (> 250ms)
    int check() {
      // Logic only runs if button is active (LOW)
      if (digitalRead(_pin) == LOW) {
        unsigned long m1 = millis();
        unsigned long m2 = m1;

        // Wait while holding
        while (digitalRead(_pin) == LOW) {
          m2 = millis();
          delay(10); // Debounce
        }

        unsigned long duration = m2 - m1;

        if (duration > 0 && duration < _threshold) {
          return 1; // Short press
        } 
        else if (duration >= _threshold) {
          return 2; // Long press
        }
      }
      return 0; // No press
    }
};