uint32_t timerStarted;

void initTimer() {
  pinMode(timerInput, INPUT_PULLUP);
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
  static uint32_t isDischargedTime;
  switch (dischargeTimerState) {
    case DISCHARGED:                                        // Batch completely discharged.
      isDischargedTime = millis();                          // Record when this happened.
      dischargeTimerState = DELAY;
      break;

    case DELAY:                                             // Wait some time before setting the signal HIGH.
      if (millis() - isDischargedTime > DISCHARGE_RELAY_DELAY) { // After some delay time,
        digitalWrite(dischargeSignalRelayPin, HIGH);        // switch on the relay.
        dischargeTimerState = WAIT_ROASTER_START;
      }
      break;

    case WAIT_ROASTER_START:                                // Wait until INPUT1 goes low - just in case.
      if (digitalRead(INPUT1) == LOW) {
        dischargeTimerState = WAIT_DEBOUNCE;
        isDischargedTime = millis();
      }
      break;

    case WAIT_DEBOUNCE:                                     // Relay contacts may bounce.
      if (millis() - isDischargedTime > 100) {              // After some delay time,
        dischargeTimerState = WAIT_ROASTER_READY;           // start waiting for the roaster to finish (and INPUT1 to go high again).
      }
      break;

    case WAIT_ROASTER_READY:                                // Wait until INPUT1 goes high, set signal LOW.
      if (digitalRead(INPUT1) == HIGH) {
        digitalWrite(dischargeSignalRelayPin, LOW);         // switch off the relay.
        dischargeTimerState = DONE;
        isDischargedTime = millis();
      }
      break;

    case DONE:                                              // Nothing to do here any more.
      break;

  }
}
