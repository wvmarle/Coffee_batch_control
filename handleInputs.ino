void initInputs() {
  pinMode(encoderPushPin, INPUT_PULLUP);
  
  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(startButtonLEDPin, OUTPUT);
  digitalWrite(startButtonLEDPin, LOW);

  pinMode(batchSelectionButtonPin, INPUT_PULLUP);
  pinMode(batchSelectionLEDPin, OUTPUT);
  digitalWrite(batchSelectionLEDPin, LOW);
  
  pinMode(stopButtonPin, INPUT_PULLUP);
  
  pinMode(INPUT1, INPUT_PULLUP);
  pinMode(INPUT1LEDPin, OUTPUT);
  digitalWrite(INPUT1LEDPin, LOW);
  
  pinMode(completeLEDPin, OUTPUT);
  digitalWrite(completeLEDPin, LOW);
  
  pinMode(stopButtonLEDPin, OUTPUT);
  digitalWrite(stopButtonLEDPin, LOW);
  pinMode(dischargeValvePin, OUTPUT);
  for (uint8_t i = 0; i < NBINS; i++) {
    pinMode(binSelectionButtonPin[i], INPUT_PULLUP);
    pinMode(binSelectionLEDPin[i], OUTPUT);
    digitalWrite(binSelectionLEDPin[i], LOW);
    pinMode(butterflyValvePin[i], OUTPUT);
    digitalWrite(butterflyValvePin[i], LOW);
  }
}

void handleInputs() {
  static long oldPosition;
  static uint32_t lastPress;
  static bool settingBatch;
  static ProcessStates stateWhenInterrupted;
  static uint32_t lastStopPressed;
  static bool lastStopState;
  int32_t encoderPosition = setWeightEncoder.read() / 2;    // Read the current encoder input.
  digitalWrite(INPUT1LEDPin, digitalRead(INPUT1));          // Have INPUT1LED follow the state of INPUT1.
  switch (processState) {
    case SET_WEIGHTS:
      if (selectedBin == NBINS &&                           // No bin selected so not setting a bin quantity,
          settingBatch == false) {                          // and not setting the number of batches to run.
        if (digitalRead(batchSelectionButtonPin) == LOW) {  // Batch selection button pressed: set the number of batches to run.
          settingBatch = true;
          digitalWrite(batchSelectionLEDPin, HIGH);         // Switch on the button's LED.
          strcpy_P(systemStatus, PSTR("Set batches to run."));
          updateDisplay = true;
        }
        else if (digitalRead(startButtonPin) == LOW) {      // Start button pressed: check if we can start the batch.
          processState = STANDBY;                           // All is OK to start, go to Standby mode.
          strcpy_P(systemStatus, PSTR("Standby batch 1..."));
          updateDisplay = true;
          nBatch = 0;                                       // It's the first batch.
          digitalWrite(startButtonLEDPin, HIGH);            // Switch on the button's LED.
          //          }
        }
        else {
          for (uint8_t i = 0; i < NBINS; i++) {             // Check for bin selection button presses.
            if (digitalRead(binSelectionButtonPin[i]) == LOW) { // We have a button pressed.
              selectedBin = i;                              // Record which bin was selected.
              digitalWrite(binSelectionLEDPin[selectedBin], HIGH); // Switch on the button's LED
              sprintf_P(systemStatus, PSTR("Set weight of bin %u."), selectedBin + 1);
              updateDisplay = true;
            }
          }
        }
      }
      else {                                                // A bin has been selected or we're setting the number of batches - read the encoder.
        if (encoderPosition != oldPosition) {               // If encoder position has changed, calculate the batches accordingly.
          if (settingBatch) {
            nBatches += (oldPosition - encoderPosition);    // Calculate the new number of batches.
            nBatches = constrain(nBatches, 1, MAX_BATCHES); // Make sure it's within limits.
          }
          else {
            binTargetWeight[selectedBin] += (oldPosition - encoderPosition); // Calculate the new weight.
            binTargetWeight[selectedBin] = constrain(binTargetWeight[selectedBin], MIN_WEIGHT, MAX_WEIGHT); // Make sure it's within limits.
          }
          updateDisplay = true;
        }
        if (digitalRead(encoderPushPin) == LOW) {           // Encoder pressed to confirm a setting.
          if (settingBatch) {
            settingBatch = false;
            digitalWrite(batchSelectionLEDPin, LOW);        // Switch off the button's LED.
          }
          else {
            digitalWrite(binSelectionLEDPin[selectedBin], LOW); // Switch off the button's LED.
            selectedBin = NBINS;
          }
          strcpy_P(systemStatus, PSTR(""));
          updateDisplay = true;
        }
      }
      break;

    case STANDBY:
    case FILLING_BIN:
    case FILLING_PAUSE:
    case DISCHARGE_BATCH:
      if (digitalRead(stopButtonPin) == LOW) {              // When stop pressed: interrupt the process.
        processState = STOPPED;
        digitalWrite(stopButtonLEDPin, HIGH);               // Switch on the LED in the stop button.
        digitalWrite(startButtonLEDPin, LOW);               // Switch off the LED in the start button.
      }
      stateWhenInterrupted = processState;
      break;

    case STOPPED:
      if (digitalRead(stopButtonPin) == LOW) {              // Check the stop button: long press for total cancel.
        if (lastStopState == HIGH) {                        // Previous state high: it's just pressed.
          lastStopState = LOW;                              // We're pressed now.
          lastStopPressed = millis();                       // Record when it happened.
          strcpy_P(systemStatus, PSTR("Hold stop: cancel."));
          updateDisplay = true;
        }
        else if (millis() - lastStopPressed > CANCEL_DELAY) { // It's pressed long enough: cancel process.
          digitalWrite(stopButtonLEDPin, LOW);              // Switch off the LED in the stop button.
          processState = SET_WEIGHTS;
          strcpy_P(systemStatus, PSTR(""));
        }
      }
      else {
        if (lastStopState == LOW) {
          strcpy_P(systemStatus, PSTR(""));
        }
        lastStopState = HIGH;                               // Button not pressed.
      }
      if (digitalRead(startButtonPin) == LOW) {             // Start button pressed.
        digitalWrite(stopButtonLEDPin, LOW);                // Switch off the LED in the stop button.
        digitalWrite(startButtonLEDPin, HIGH);              // Switch on the LED in the start button.
        processState = stateWhenInterrupted;                // Continue where we were.
        strcpy_P(systemStatus, PSTR(""));
        updateDisplay = true;
      }
      break;

    case COMPLETED:
      bool action = false;
      if (digitalRead(batchSelectionButtonPin) == LOW ||    // Batch selection button pressed, or
          digitalRead(startButtonPin) == LOW ||             // Start button pressed, or
          digitalRead(stopButtonPin) == LOW ||              // Stop button pressed, or
          digitalRead(encoderPushPin) == LOW ||             // encoder button pushed, or
          encoderPosition != oldPosition) {                 // encoder moved:
        action = true;                                      // User action!
      }
      for (uint8_t i = 0; i < NBINS; i++) {                 // Check for bin selection button presses.
        if (digitalRead(binSelectionButtonPin[i]) == LOW) { // We have a button pressed.
          action = true;
        }
      }
      if (action) {                                         // User did something: clear complete status.
        processState = SET_WEIGHTS;
        digitalWrite(completeLEDPin, LOW);                  // Switch off the "complete" LED.
      }
      break;
  }
  oldPosition = encoderPosition;
}
