uint8_t fillingBin;                                         // Which bin we're currently filling (or going to fill).
ProcessStates oldState;                                     // For the watchdog timer: keep track of what we were doing, so we can easily recover.
uint32_t blinkTimer;                                        // For blinking specific LEDs.
bool blinkState;                                            // Whether we're blinking on or off.

void handleProcess() {
  static uint16_t startWeight;                              // Scale's weight indication at the start of a fill process.
  //  static uint32_t lastPrint;
  switch (processState) {
    case SET_WEIGHTS:                                       // User is setting weights and number of batches. Nothing to do here.
      break;

    case STANDBY:                                           // Waiting for INPUT1 to go HIGH.
      if (digitalRead(INPUT1) == HIGH) {                    // INPUT1 HIGH: can start the batch.
        fillingBin = 0;                                     // Start filling the first bin.
        startWeight = scaleWeight;                          // The weight at the start of this process.
        for (uint8_t i = 0; i < NBINS; i++) {               // Reset the bin weights for proper display.
          binWeight[i] = 0;
        }
        setState(FILLING_BIN);                              // We start filling the bin.
      }
      checkWDT();
      break;

    case FILLING_BIN:                                       // Open the valves one by one; add material.
      binWeight[fillingBin] = scaleWeight - startWeight;    // Record the weight added to this bin.
      if (binWeight[fillingBin] >= binTargetWeight[fillingBin]) { // This one is complete.
        lastFillCompleteTime = millis();                    // Record when we completed this one.
        setState(FILLING_PAUSE);                            // Take a break for the scale to stabilise.
      }
      checkWDT();
      break;

    case FILLING_PAUSE:                                     // Wait a bit... allow scale to stabilise.
      binWeight[fillingBin] = scaleWeight - startWeight;    // Continue to record the weight of this bin.
      if (millis() - lastFillCompleteTime > FILL_PAUSE_TIME) { // When time's up:
        fillingBin++;                                       // Next bin (if any).
        if (fillingBin < NBINS) {                           // We have bins left to fill.
          startWeight = scaleWeight;
          setState(FILLING_BIN);
        }
        else {                                              // All bins filled.
          lastFillCompleteTime = millis();                  // Record when the pause was over.
          setState(DISCHARGE_BATCH);                        // Continue the process: discharge the batch through the discharge valve.
        }
      }
      checkWDT();
      break;

    case DISCHARGE_BATCH:                                   // Empty the thing through the discharge valve.
      if (millis() - lastFillCompleteTime > BATCH_DISCHARGE_TIME) { // If open long enough,
        digitalWrite(dischargeValvePin, LOW);               // Close the valve again.
        nBatch++;                                           // Go to the next batch.
        dischargeTimerState = DISCHARGED;                   // A discharge has completed; start the timer thing.
        if (nBatch == nBatches) {                           // If we're done,
          nBatches = 1;                                     // reset the number of batches to 1.
          setState(COMPLETED);                              // set process to "completed" state, and
        }
        else {                                              // Otherwise go standby for the next batch.
          setState(STANDBY);
        }
      }
      break;

    case STOPPED:                                           // User paused. Nothing to do until user takes further action.
      break;

    case COMPLETED:                                         // Process completed. Nothing to do until user takes further action.
      break;

    case WDT_TIMEOUT:
      if (millis() - blinkTimer > BLINK_SPEED) {
        blinkState = !blinkState;
        digitalWrite(stopButtonLEDPin, blinkState);
        blinkTimer = millis();
      }
      if (millis() - latestWeightReceivedTime < SCALE_TIMEOUT) { // We got communications again.
        setState(WDT_WAITING_FOR_START);
      }
      break;

    case WDT_WAITING_FOR_START:
      checkWDT();
      if (millis() - blinkTimer > BLINK_SPEED) {
        blinkState = !blinkState;
        digitalWrite(startButtonLEDPin, blinkState);
        blinkTimer = millis();
      }
      if (digitalRead(startButtonPin) == LOW) {             // Start button pressed: check if we can start the batch.
        setState(oldState);
      }
      break;
  }
}

void checkWDT() {
  if (millis() - latestWeightReceivedTime > SCALE_TIMEOUT) { // Watchdog: scale does not send data.
    oldState = processState;                                // Remember what we were doing, so we can recover.
    setState(WDT_TIMEOUT);                                  // Timeout state!
  }
}

void openValve(uint8_t valve) {
  for (uint8_t i = 0; i < NBINS; i++) {
    if (i == valve) {                                       // Open this valve.
      digitalWrite(butterflyValvePin[i], HIGH);
    }
    else {
      digitalWrite(butterflyValvePin[i], LOW);              // Close all the others.
    }
  }
  digitalWrite(valveOpenIndicatorPin, HIGH);
}

void closeValves() {                                        // Close all the valves.
  for (uint8_t i = 0; i < NBINS; i++) {
    digitalWrite(butterflyValvePin[i], LOW);
  }
  digitalWrite(valveOpenIndicatorPin, LOW);
}

uint16_t totalWeight() {                                    // Calculate the total weight of the batch as currently set.
  uint16_t total = 0;
  for (uint8_t i = 0; i < NBINS; i++) {
    total += binTargetWeight[i];
  }
  return total;
}

// Set the system up for the requested process state: this includes setting the appropriate LEDs, open/close valves, etc.
void setState(ProcessStates state) {
  switch (state) {
    case SET_WEIGHTS:
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(startButtonLEDPin, LOW);                 // Switch off the LED of the Start button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      strcpy_P(systemStatus, PSTR(""));
      break;

    case STANDBY:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch on the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      sprintf_P(systemStatus, PSTR("Standby batch %u..."), nBatch + 1);
      break;

    case FILLING_BIN:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch on the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      openValve(fillingBin);                                // Open valve for the bin we're going to fill.
      sprintf_P(systemStatus, PSTR("Filling hopper %u..."), fillingBin + 1);
      break;

    case FILLING_PAUSE:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch on the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      closeValves();                                        // Close the valves.
      sprintf_P(systemStatus, PSTR("Hopper %u filled."), fillingBin + 1);
      break;

    case DISCHARGE_BATCH:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch on the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      sprintf_P(systemStatus, PSTR("Discharge batch %u."), nBatch + 1);
      digitalWrite(dischargeValvePin, HIGH);                // Open the discharge valve (butterfly valve 5).
      break;

    case STOPPED:
      digitalWrite(startButtonLEDPin, LOW);                 // Switch off the LED in the start button.
      digitalWrite(stopButtonLEDPin, HIGH);                 // Switch on the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      closeValves();                                        // Close all input valves,
      digitalWrite(dischargeValvePin, LOW);                 // and the output valve.
      strcpy_P(systemStatus, PSTR("Paused."));
      break;  

    case COMPLETED:
      digitalWrite(startButtonLEDPin, LOW);                 // Switch off the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, HIGH);                   // Switch on the "complete" LED.
      strcpy_P(systemStatus, PSTR("Complete."));
      break;

    case WDT_TIMEOUT:
      digitalWrite(startButtonLEDPin, LOW);                 // Switch off the LED of the Start button.
      digitalWrite(stopButtonLEDPin, HIGH);                 // Switch on the LED in the stop button - it'll blink.
      blinkState = HIGH;
      blinkTimer = millis();
      closeValves();
      strcpy_P(systemStatus, PSTR("Scale disconnected."));
      break;

    case WDT_WAITING_FOR_START:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch on the LED of the Start button - it'll blink.
      digitalWrite(stopButtonLEDPin, HIGH);                 // Switch on the LED in the stop button, showing we're still stopped.
      blinkState = HIGH;
      blinkTimer = millis();
      strcpy_P(systemStatus, PSTR("START to continue."));
      break;

  }
  processState = state;
  updateDisplay = true;
}
