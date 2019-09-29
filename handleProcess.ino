uint8_t fillingBin;                                         // Which bin we're currently filling (or going to fill).

void handleProcess() {
  static uint16_t startWeight;                              // Scale's weight indication at the start of a fill process.
  static uint32_t lastPrint;
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
      break;

    case FILLING_BIN:                                       // Open the valves one by one; add material.
      binWeight[fillingBin] = scaleWeight - startWeight;    // Record the weight added to this bin.
      if (binWeight[fillingBin] >= binTargetWeight[fillingBin]) { // This one is complete.
        lastFillCompleteTime = millis();                    // Record when we completed this one.
        setState(FILLING_PAUSE);                            // Take a break for the scale to stabilise.
      }
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
      break;

    case DISCHARGE_BATCH:                                   // Empty the thing through the discharge valve.
      if (millis() - lastPrint > 1000) {
        lastPrint = millis();
      }
      if (millis() - lastFillCompleteTime > BATCH_DISCHARGE_TIME) { // If open long enough,
        digitalWrite(dischargeValvePin, LOW);               // Close the valve again.
        nBatch++;                                           // Go to the next batch.
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
      digitalWrite(startButtonLEDPin, HIGH);                // Switch off the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      sprintf_P(systemStatus, PSTR("Standby batch %u..."), nBatch + 1);
      break;

    case FILLING_BIN:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch off the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      openValve(fillingBin);                                // Open valve for the bin we're going to fill.
      sprintf_P(systemStatus, PSTR("Filling hopper %u..."), fillingBin + 1);
      break;

    case FILLING_PAUSE:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch off the LED of the Start button.
      digitalWrite(stopButtonLEDPin, LOW);                  // Switch off the LED in the stop button.
      digitalWrite(completeLEDPin, LOW);                    // Switch off the "complete" LED.
      closeValves();                                        // Close the valves.
      sprintf_P(systemStatus, PSTR("Hopper %u filled."), fillingBin + 1);
      break;

    case DISCHARGE_BATCH:
      digitalWrite(startButtonLEDPin, HIGH);                // Switch off the LED of the Start button.
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
  }
  processState = state;
  updateDisplay = true;
}
