uint32_t timerStarted;

void initTimer() {
  pinMode(timerInput, INPUT);
  pinMode(timerOutput, OUTPUT);
  timerStarted = -TIMER_DELAY;                              // Make sure we don't go on right away.
}

void handleTimer() {
  if (millis() - timerStarted < TIMER_DELAY) {
    digitalWrite(timerOutput, HIGH);
  }
  else {
    digitalWrite(timerOutput, LOW);
    if (digitalRead(timerInput) == HIGH) {
      timerStarted = millis();
    }
  }

  // Relay on, starting 
  if (isComplete) {
    if (isCompleteTime - millis() > COMPLETE_RELAY_DELAY) {
      digitalWrite(completeRelayPin, HIGH);
    }
    if (isCompleteTime - millis() > COMPLETE_RELAY_DELAY + COMPLETE_RELAY_ONTIME) {
      digitalWrite(completeRelayPin, LOW);
      isComplete = false;
    }    
  }
}
