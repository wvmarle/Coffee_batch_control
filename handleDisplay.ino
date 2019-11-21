void initDisplay() {
  lcd.begin(20, 4);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);                    // Switch backlight on. We won't switch it off anytime during operation.

  // A 4-line welcome message will be displayed for 5 seconds.
  // 20 chars: "--------------------" Don't exceed this length!
  lcd.setCursor(0, 0);
  lcd.print(F("      Welcome!"));
  lcd.setCursor(0, 1);
  lcd.print(F(" Coffee batch mixer"));
  lcd.setCursor(0, 3);
  lcd.print(F("   Made in Italy!"));
  delay(5000);                                              // Show message for 5 seconds.
  updateDisplay = true;                                     // Have it updated right away.
}

void handleDisplay() {
  static char linebuff[30];                                 // General buffer for a line of text - with some space for overrun, which will be truncated later.
  static uint32_t lastUpdate;                               // When the display was last updated (millis() value)
  if (millis() - lastUpdate > LCD_UPDATE_INTERVAL || updateDisplay) {
    lastUpdate = millis();
    updateDisplay = false;
    switch (processState) {
      case SET_WEIGHTS:
        sprintf_P(linebuff, PSTR("B1: %3i   B2: %3i"), binTargetWeight[0], binTargetWeight[1]);
        printLine(linebuff, 0);                             // Line 0: the target weights of bins 1 and 2.
        sprintf_P(linebuff, PSTR("B3: %3i   B4: %3i"), binTargetWeight[2], binTargetWeight[3]);
        printLine(linebuff, 1);                             // Line 1: the target weights of bins 3 and 4.
        break;

      case FILLING_BIN:
      case FILLING_PAUSE:
#ifdef SHOW_TARGET
        sprintf_P(linebuff, PSTR("B1: %2i/%2i B2: %2i/%2i"), binWeight[0], binTargetWeight[0], binWeight[1], binTargetWeight[1]);
        printLine(linebuff, 0);                           // Line 0: the current/target weights of bins 1 and 2.
        sprintf_P(linebuff, PSTR("B3: %2i/%2i B4: %2i/%2i"), binWeight[2], binTargetWeight[2], binWeight[3], binTargetWeight[3]);
        printLine(linebuff, 1);                           // Line 1: the current/target weights of bins 3 and 4.
#else
        sprintf_P(linebuff, PSTR("B1: %3i  B2: %3i"), binWeight[0], binWeight[1]);
        printLine(linebuff, 0);                           // Line 0: the current weights of bins 1 and 2.
        sprintf_P(linebuff, PSTR("B3: %3i  B4: %3i"), binWeight[2], binWeight[3]);
        printLine(linebuff, 1);                           // Line 1: the current weights of bins 3 and 4.
#endif
        break;

      case STANDBY:
      case DISCHARGE_BATCH:                                 // Don't touch the first two lines in these modes.
      case STOPPED:
        break;
    }
    switch (processState) {
      case SET_WEIGHTS:
        sprintf_P(linebuff, PSTR("Total: %i kg x %i"), totalWeight(), nBatches);
        break;

      case STANDBY:
      case FILLING_BIN:
      case FILLING_PAUSE:
        sprintf_P(linebuff, PSTR("%4i kg, #%i/%i"), scaleWeight, nBatch + 1, nBatches);
        break;

      case DISCHARGE_BATCH:
        {
          uint32_t timePassed = millis() - lastFillCompleteTime;
          uint32_t timeToGo = BATCH_DISCHARGE_TIME - timePassed;
          uint8_t minutes = timeToGo / 60000ul;
          uint8_t seconds = uint32_t(timeToGo / 1000ul) % 60;
          sprintf_P(linebuff, PSTR("%4i kg, #%i/%i %2u:%02u"), scaleWeight, nBatch + 1, nBatches, minutes, seconds);
        }
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
// Print line to display; pad with spaces to 20 characters to remove stray characters.
void printLine(const char *lineToPrint, uint8_t line) {
  char printBuffer[21];
  lcd.setCursor(0, line);
  snprintf_P(printBuffer, 21, PSTR("%-20s"), lineToPrint);  // Pad with trailing spaces to total length of 20 characters, but also not more than that.
  lcd.print(printBuffer);
}
