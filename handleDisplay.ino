void initDisplay() {
  lcd.begin(20, 4);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);                    // Switch backlight on. We won't switch it off anytime during operation.
  updateDisplay = true;                                     // Have it updated right away.
}

void handleDisplay() {
  static char linebuff[21];                                 // General buffer for a line of text.
  static uint32_t lastUpdate;                               // When the display was last updated (millis() value)
  if (millis() - lastUpdate > LCD_UPDATE_INTERVAL || updateDisplay) {
    lastUpdate = millis();
    updateDisplay = false;
    switch (processState) {
      case SET_WEIGHTS:
        sprintf_P(linebuff, PSTR("B1: %3u   B2: %3u"), binTargetWeight[0], binTargetWeight[1]);
        printLine(linebuff, 0);                             // Line 0: the target weights of bins 1 and 2.
        sprintf_P(linebuff, PSTR("B3: %3u   B4: %3u"), binTargetWeight[2], binTargetWeight[3]);
        printLine(linebuff, 1);                             // Line 1: the target weights of bins 3 and 4.
        break;

      case FILLING_BIN:
      case FILLING_PAUSE:
        {
          sprintf_P(linebuff, PSTR("B1: %3u   B2: %3u"), binWeight[0], binWeight[1]);
          printLine(linebuff, 0);                             // Line 0: the current weights of bins 1 and 2.
          sprintf_P(linebuff, PSTR("B3: %3u   B4: %3u"), binWeight[2], binWeight[3]);
          printLine(linebuff, 1);                             // Line 1: the current weights of bins 2 and 3.
        }
        break;

      case STANDBY:
      case DISCHARGE_BATCH:                                 // Don't touch the first two lines in these modes.
      case STOPPED:
        break;
    }
    switch (processState) {
      case SET_WEIGHTS:
        sprintf_P(linebuff, PSTR("Total: %u kg x %u"), totalWeight(), nBatches);
        break;

      case STANDBY:
      case FILLING_BIN:
      case FILLING_PAUSE:
        sprintf_P(linebuff, PSTR("%3u kg, #%u"), scaleWeight, nBatch);
        break;

      case DISCHARGE_BATCH:
        uint32_t timePassed = millis() - lastFillCompleteTime;
        uint32_t timeToGo = BATCH_DISCHARGE_TIME - timePassed;
        uint8_t minutes = int((float)timeToGo / (60 * 1000));
        uint8_t seconds = int(timeToGo / 1000.0) % 60;
        sprintf_P(linebuff, PSTR("%3u kg, #%u %2u:%2u"), scaleWeight, nBatch, minutes, seconds);
        break;

      case STOPPED:
        strcpy_P(linebuff, PSTR("Start: continue."));
        break;
    }
    printLine(linebuff, 2);                                 // Line 2: weights & batches depending on the status.
    printLine(systemStatus, 3);                             // Line 3: the system status message.
  }
}

////////////////////////////////////////////////////////////////////
// Print line to display; pad with spaces to 16 characters to remove stray characters.
void printLine(const char *lineToPrint, uint8_t line) {
  char printBuffer[21];
  lcd.setCursor(0, line);
  sprintf_P(printBuffer, PSTR("%-20s"), lineToPrint);       // Pad with trailing spaces to total length of 20 characters.
  lcd.print(printBuffer);
}
