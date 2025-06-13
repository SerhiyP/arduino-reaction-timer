// Піни для 74HC595
const int dataPin = 2;   // DS
const int clockPin = 3;  // SH_CP
const int latchPin = 4;  // ST_CP

// Кнопки
const int startButtonPin = 5;
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

// Debounce variables
unsigned long lastStartButtonTime = 0;
unsigned long lastStopButtonTime = 0;
const unsigned long debounceDelay = 200;

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

// Таблиця сегментів з десятковою крапкою (A-G + DP)
const byte numberTableWithDP[10] = {
  0b10111111, // 0.
  0b10000110, // 1.
  0b11011011, // 2.
  0b11001111, // 3.
  0b11100110, // 4.
  0b11101101, // 5.
  0b11111101, // 6.
  0b10000111, // 7.
  0b11111111, // 8.
  0b11101111  // 9.
};

void testDisplay() {
  // Test all segments on all digits
  for (int i = 0; i < 10; i++) {
    digitalWrite(latchPin, LOW);
    for (int digit = 0; digit < 4; digit++) {
      shiftOut(dataPin, clockPin, MSBFIRST, numberTable[i]);
    }
    digitalWrite(latchPin, HIGH);
    delay(200);
  }
  
  // Test each digit individually
  for (int digit = 0; digit < 4; digit++) {
    digitalWrite(latchPin, LOW);
    for (int d = 0; d < 4; d++) {
      if (d == digit) {
        shiftOut(dataPin, clockPin, MSBFIRST, numberTable[8]); // 8 lights all segments
      } else {
        shiftOut(dataPin, clockPin, MSBFIRST, 0x00); // Turn off
      }
    }
    digitalWrite(latchPin, HIGH);
    delay(300);
  }
}

bool validateButtons() {
  Serial.println("Button validation:");
  Serial.println("Press START button...");
  
  unsigned long startTime = millis();
  bool startPressed = false;
  
  // Wait for start button press with timeout
  while (millis() - startTime < 5000) {
    if (digitalRead(startButtonPin) == LOW) {
      startPressed = true;
      Serial.println("START button detected!");
      delay(500); // Wait for button release
      break;
    }
  }
  
  if (!startPressed) {
    Serial.println("START button not detected!");
    return false;
  }
  
  Serial.println("Press STOP button...");
  startTime = millis();
  bool stopPressed = false;
  
  // Wait for stop button press with timeout
  while (millis() - startTime < 5000) {
    if (digitalRead(stopButtonPin) == LOW) {
      stopPressed = true;
      Serial.println("STOP button detected!");
      delay(500); // Wait for button release
      break;
    }
  }
  
  if (!stopPressed) {
    Serial.println("STOP button not detected!");
    return false;
  }
  
  Serial.println("Button validation successful!");
  return true;
}

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
  
  pinMode(startButtonPin, INPUT_PULLUP);
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
  
  // Display ready pattern - show 00.00
  displayTimeDigits(0, 0, 0, 0);
}

void displayTimeDigits(int tens, int units, int decimal1, int decimal2) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[decimal2]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[decimal1]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTableWithDP[units]);
  shiftOut(dataPin, clockPin, MSBFIRST, numberTable[tens]);
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
  
  // Handle start button
  if (digitalRead(startButtonPin) == LOW) {
    unsigned long startElapsed = currentTime - lastStartButtonTime;
    if (startElapsed > debounceDelay) {
      if (!timerRunning) {
        startTimer();
      } else {
        resetTimer();
      }
      lastStartButtonTime = currentTime;
    }
  }
  
  // Handle stop button
  if (digitalRead(stopButtonPin) == LOW) {
    unsigned long stopElapsed = currentTime - lastStopButtonTime;
    if (stopElapsed > debounceDelay) {
      if (timerRunning) {
        stopTimer();
      }
      lastStopButtonTime = currentTime;
    }
  }
  
  // Update display every 10ms for smooth animation
  if (currentTime - lastDisplayUpdate >= displayUpdateInterval) {
    updateTimer();
    lastDisplayUpdate = currentTime;
  }
}
