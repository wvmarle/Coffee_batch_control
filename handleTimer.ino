uint32_t timerStarted;

void initTimer() {
  pinMode(timerInput, INPUT);
  pinMode(timerOutput, OUTPUT);
  pinMode(dischargeSignalRelayPin, OUTPUT);
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
  if (isDischarging) {
    if (isDischargingTime - millis() > DISCHARGE_RELAY_DELAY) {
      digitalWrite(dischargeSignalRelayPin, HIGH);
    }
    if (isDischargingTime - millis() > DISCHARGE_RELAY_DELAY + DISCHARGE_RELAY_ONTIME) {
      digitalWrite(dischargeSignalRelayPin, LOW);
      isDischarging = false;
    }    
  }
}
