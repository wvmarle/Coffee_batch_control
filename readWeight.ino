// Format: <STX> <SIGN> <WEIGHT(7)> <STATUS> <ETX>
// STX: Start of transmission character (ASCII 02).
// ETX: End of transmission character (ASCII 03).
// SIGN: The sign of the weight reading (space for positive, dash (-) for negative).
// WEIGHT(7): A seven-character string containing the current weight including the decimal point.
//            If there is no decimal point, then the first character is a space. Leading zero blanking applies.
// STATUS: a status indication, G/N/U/O/E representing Gross / Net / Underload / Overload / Error, respectively.

#define STX 0x02
#define ETX 0x03

enum {
  WAITING,
  RECEIVING,
  COMPLETING,
} receiveState;

void readWeight() {
  static char buff[8];                                      // Buffer to store the weight.
  static byte nChars = 0;                                   // Number of characters in buffer.
  static char stat = 0;
  if (Serial1.available()) {
    char c = Serial1.read();
    switch (receiveState) {
      case WAITING:
        if (c == STX) {
          receiveState = RECEIVING;
        }
        break;

      case RECEIVING:
        if (nChars == 0) {                                  // receive SIGN
          if (c == '-' || c == ' ') {
            buff[nChars] = c;
            nChars++;
          }
        }
        else if (nChars > 0 && nChars < 8) {               // receive WEIGHT
          if ((c >= '0' && c <= '9') || c == '.' || c == ' ' ) {
            buff[nChars - 1] = c;
            nChars++;
          }
          if (nChars == 8) {
            buff[7] = 0;
          }
        }
        else if (nChars == 8) {                           // receive STATUS
          // G = Gross.
          // N = Net.
          // U = Underload.
          // O = Overload.
          // E = Error.
          // M = unknown status - used by scale simulator.
          if (c == 'G' || c == 'N' || c == 'U' || c == 'O' || c == 'E' || c == 'M') {
            stat = c;
          }
          receiveState = COMPLETING;
        }
        break;

      case COMPLETING:
        if (c == ETX) {
          float f = atof(buff);
          scaleWeight = f;
          latestWeightReceivedTime = millis();
        }
        receiveState = WAITING;
        nChars = 0;
        break;
    }
  }
}
