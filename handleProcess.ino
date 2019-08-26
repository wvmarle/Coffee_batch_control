void handleProcess() {
  static uint16_t startWeight;                              // Scale's weight indication at the start of a fill process.
  static uint8_t fillingBin;                                // Which bin to fill.
  static uint32_t lastPrint;
  switch (processState) {
    case SET_WEIGHTS:                                       // User is setting weights and number of batches. Nothing to do here.
      break;

    case STANDBY:                                           // Waiting for INPUT1 to go HIGH.
      if (digitalRead(INPUT1) == HIGH) {                    // INPUT1 HIGH: can start the batch.
        processState = FILLING_BIN;                         // We start filling the bin.
        fillingBin = 0;                                     // Start filling the first bin.
        openValve(fillingBin);                              // Open valve for that bin.
        strcpy_P(systemStatus, PSTR("Filling hopper 1..."));
        updateDisplay = true;
        startWeight = scaleWeight;                          // The weight at the start of this process.
        for (uint8_t i = 0; i < NBINS; i++) {               // Reset the bin weights for proper display.
          binWeight[fillingBin] = 0;
        }
      }
      break;

    case FILLING_BIN:                                       // Open the valves one by one; add material.
      binWeight[fillingBin] = scaleWeight - startWeight;    // Record the weight added to this bin.
      if (binWeight[fillingBin] >= binTargetWeight[fillingBin]) { // This one is complete.
        closeValves();                                      // Close the valves.
        processState = FILLING_PAUSE;                       // Take a break for the scale to stabilise.
        lastFillCompleteTime = millis();
        sprintf_P(systemStatus, PSTR("Hopper %u filled."), fillingBin + 1);
        updateDisplay = true;
      }
      break;

    case FILLING_PAUSE:                                     // Wait a bit... allow scale to stabilise.
      binWeight[fillingBin] = scaleWeight - startWeight;    // Continue to record the weight of this bin.
      if (millis() - lastFillCompleteTime > FILL_PAUSE_TIME) { // When time's up:
        fillingBin++;                                       // Next bin (if any).
        if (fillingBin < NBINS) {                           // We have bins left to fill.
          processState = FILLING_BIN;
          startWeight = scaleWeight;
          openValve(fillingBin);                            // Open valve for that bin.
          sprintf_P(systemStatus, PSTR("Filling hopper %u..."), fillingBin + 1);
          updateDisplay = true;
        }
        else {                                              // All bins filled.
          processState = DISCHARGE_BATCH;                   // Continue the process: discharge the batch through the discharge valve.
          lastFillCompleteTime = millis();
          sprintf_P(systemStatus, PSTR("Discharge batch %u."), nBatch + 1);
          updateDisplay = true;
          digitalWrite(dischargeValvePin, HIGH);                // Open the valve.
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
          processState = COMPLETED;                         // set process to "completed" state, and
          nBatches = 1;                                     // reset the number of batches to 1.
          strcpy_P(systemStatus, PSTR("Complete."));
          updateDisplay = true;
        }
        else {                                              // Otherwise go standby for the next batch.
          sprintf_P(systemStatus, PSTR("Standby batch %u..."), nBatch + 1);
          updateDisplay = true;
          processState = STANDBY;
        }
      }
      break;

    case STOPPED:                                           // User interrupt. Close all valves.
      closeValves();                                        // Close all input valves,
      digitalWrite(dischargeValvePin, LOW);                 // and the output valve.
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
}

void closeValves() {                                        // Close all the valves.
  for (uint8_t i = 0; i < NBINS; i++) {
    digitalWrite(butterflyValvePin[i], LOW);
  }
}

uint16_t totalWeight() {                                    // Calculate the total weight of the batch as currently set.
  uint16_t total = 0;
  for (uint8_t i = 0; i < NBINS; i++) {
    total += binTargetWeight[i];
  }
  return total;
}
