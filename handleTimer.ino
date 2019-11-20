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

  if (isDischarging) {                                      // Discharge process started.
    if (millis() - isDischargingTime > DISCHARGE_RELAY_DELAY + DISCHARGE_RELAY_ONTIME) { // Signal time's up,
      digitalWrite(dischargeSignalRelayPin, LOW);           // switch off the relay.
      isDischarging = false;
    }    
    else if (millis() - isDischargingTime > DISCHARGE_RELAY_DELAY) { // After some delay time,
      digitalWrite(dischargeSignalRelayPin, HIGH);          // switch on the relay.
    }
  }
}
