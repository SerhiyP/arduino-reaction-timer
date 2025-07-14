// Піни для 74HC595
const int dataPin = 2;   // DS
const int clockPin = 3;  // SH_CP
const int latchPin = 4;  // ST_CP

// Кнопка
const int stopButtonPin = 6;

// Timer variables
unsigned long timerStartTime = 0;
unsigned long pausedTime = 0;
bool timerRunning = false;
bool timerPaused = false;

// Display variables
int digits[4] = {0, 0, 0, 0};
unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 10; // Update every 10ms for smooth display

// Button state and debounce variables
int buttonState = 0; // 0=start, 1=stop, 2=reset
unsigned long lastStopButtonTime = 0;
const unsigned long debounceDelay = 200;
bool lastButtonReading = HIGH;
bool buttonPressed = false;

// Таблиця сегментів (A-G, без DP)
const byte numberTable[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};


void displayNumber(int d1, int d2, int d3, int d4) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[d4]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[d3]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[d2]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[d1]);
  digitalWrite(latchPin, HIGH);
}

void displayError(int errorCode) {
  // Display "Err" followed by error code
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, errorCode);
  shiftOut(dataPin, clockPin, MSBFIRST, 0x79); // E
  shiftOut(dataPin, clockPin, MSBFIRST, 0x50); // r
  shiftOut(dataPin, clockPin, MSBFIRST, 0x50); // r
  digitalWrite(latchPin, HIGH);
}

void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  pinMode(stopButtonPin, INPUT_PULLUP);

  // Initialize display to blank immediately to avoid random patterns
  digitalWrite(latchPin, LOW);
  for (int i = 0; i < 4; i++) {
    shiftOut(dataPin, clockPin, MSBFIRST, 0x00); // All segments off
  }
  digitalWrite(latchPin, HIGH);

  Serial.begin(9600);

  // Wait for serial to be ready
  delay(1000);

  // Display ready pattern - show 0000
  displayTimeDigits(0, 0, 0, 0);
}

void displayTimeDigits(int tens, int units, int decimal1, int decimal2) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[tens]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[units]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[decimal1]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[decimal2]);
  digitalWrite(latchPin, HIGH);
}

void updateTimer() {
  if (!timerRunning) return;

  unsigned long currentTime = millis();
  unsigned long elapsedTime;

  if (timerPaused) {
    elapsedTime = pausedTime;
  } else {
    elapsedTime = currentTime - timerStartTime;
  }

  unsigned long totalCentiseconds = elapsedTime / 10;

  if (totalCentiseconds > 9999) {
    totalCentiseconds = 9999;
  }

  int seconds = totalCentiseconds / 100;
  int centiseconds = totalCentiseconds % 100;

  int tens = seconds / 10;
  int units = seconds % 10;
  int decimal1 = centiseconds / 10;
  int decimal2 = centiseconds % 10;

  displayTimeDigits(tens, units, decimal1, decimal2);
}

void startTimer() {
  if (!timerRunning) {
    timerStartTime = millis();
    timerRunning = true;
    timerPaused = false;
    Serial.println("Timer started");
  } else if (timerPaused) {
    timerStartTime = millis() - pausedTime;
    timerPaused = false;
    Serial.println("Timer resumed");
  }
}

void stopTimer() {
  if (timerRunning && !timerPaused) {
    pausedTime = millis() - timerStartTime;
    timerPaused = true;
    Serial.print("Timer paused at: ");
    Serial.print(pausedTime / 1000.0, 2);
    Serial.println(" seconds");
  }
}

void resetTimer() {
  timerRunning = false;
  timerPaused = false;
  pausedTime = 0;
  displayTimeDigits(0, 0, 0, 0);
  Serial.println("Timer reset");
}

void loop() {
  unsigned long currentTime = millis();

  // Handle single button cycling through states with edge detection
  bool currentButtonReading = digitalRead(stopButtonPin);
  
  // Detect button press (HIGH to LOW transition)
  if (lastButtonReading == HIGH && currentButtonReading == LOW && !buttonPressed) {
    unsigned long stopElapsed = currentTime - lastStopButtonTime;
    if (stopElapsed > debounceDelay) {
      if (buttonState == 0) {
        startTimer();
        buttonState = 1;
        Serial.println("State: START -> STOP");
      } else if (buttonState == 1) {
        stopTimer();
        buttonState = 2;
        Serial.println("State: STOP -> RESET");
      } else if (buttonState == 2) {
        resetTimer();
        buttonState = 0;
        Serial.println("State: RESET -> START");
      }
      lastStopButtonTime = currentTime;
      buttonPressed = true;
    }
  }
  
  // Reset button pressed flag when button is released
  if (currentButtonReading == HIGH) {
    buttonPressed = false;
  }
  
  lastButtonReading = currentButtonReading;

  // Update display every 10ms for smooth animation
  if (currentTime - lastDisplayUpdate >= displayUpdateInterval) {
    updateTimer();
    lastDisplayUpdate = currentTime;
  }
}
