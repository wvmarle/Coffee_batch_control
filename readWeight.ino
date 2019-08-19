// Format: <STX> <SIGN> <WEIGHT(7)> <STATUS> <ETX>
// STX: Start of transmission character (ASCII 02).
// ETX: End of transmission character (ASCII 03).
// SIGN: The sign of the weight reading (space for positive, dash (-) for negative).
// WEIGHT(7): A seven-character string containing the current weight including the decimal point.
//            If there is no decimal point, then the first character is a space. Leading zero blanking applies.
// STATUS: a status indication?? Probably: G/N/U/O/E representing Gross / Net / Underload / Overload / Error, respectively.

//enum {
//  WAITING,
//  RECEIVING,
//  COMPLETING,
//} receiveState;
//
//void readWeight() {
//  static char buff[8];                                      // Buffer to store the weight.
//  static byte nChars = 0;                                   // Number of characters in buffer.
//  static char stat = 0;
//  if (Serial1.available()) {
//    char c = Serial1.read();
//    switch (receiveState) {
//      case WAITING:
//        if (c == 0x02) {
//          Serial.println(F("Start."));
//          nChars = 0;
//          receiveState = RECEIVING;
//        }
//        break;
//
//      case RECEIVING:
//        if (nChars == 0) {                                  // receive SIGN
//          if (c == '-' || c == ' ') {
//            buff[nChars] = c;
//            nChars++;
//            Serial.println(F("Sign."));
//          }
//          else {
//            Serial.print(F("Scale read error: unexpected first value character received: 0x"));
//            Serial.println(c, HEX);
//            receiveState = WAITING;
//          }
//        }
//        else if (nChars > 0 && nChars < 8) {               // receive WEIGHT
//          if ((c >= '0' && c <= '9') || c == '.' || c == ' ' ) {
//            buff[nChars - 1] = c;
//            nChars++;
//          }
//          else {
//            Serial.print(F("Scale read error: unexpected value character received: 0x"));
//            Serial.println(c, HEX);
//            receiveState = WAITING;
//          }
//          if (nChars == 8) {
//            buff[7] = 0;
//            Serial.print(F("Got data: "));
//            Serial.println(buff);
//          }
//        }
//        else if (nChars == 8) {                           // receive STATUS
//          if (c == 'G' || c == 'N' || c == 'U' || c == 'O' || c == 'E') {
//            stat = c;
//            Serial.println(F("Done."));
//          }
//          else {
//            Serial.print(F("Scale read error: unexpected status character received: 0x"));
//            Serial.println(c, HEX);
//            receiveState = WAITING;
//          }
//          receiveState = COMPLETING;
//        }
//        break;
//
//      case COMPLETING:
//        if (c == 0x03) {
//          scaleWeight = atof(buff);
//          Serial.print(F("Weight reading: "));
//          Serial.print(scaleWeight);
//          Serial.println(F(" kg."));
//        }
//        else {
//          Serial.print(F("Scale read error: unexpected termination character received: 0x"));
//          Serial.println(c, HEX);
//          receiveState = WAITING;
//        }
//        receiveState = WAITING;
//        break;
//    }
//  }
//}


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
//    Serial.print(nChars);
//    Serial.print(F(": 0x"));
//    Serial.print(c, HEX);
//    Serial.print(F(", "));
    switch (receiveState) {
      case WAITING:
        if (c == STX) {
          receiveState = RECEIVING;
//          Serial.println(F("STX received."));
        }
        else {
          Serial.println();
          Serial.print(F("Unexpected STX character 0x"));
          Serial.print(c, HEX);
          Serial.println(F(" received."));
        }
        break;

      case RECEIVING:
        if (nChars == 0) {                                  // receive SIGN
          if (c == '-' || c == ' ') {
            buff[nChars] = c;
            nChars++;
//            Serial.print(F("Sign received: "));
//            Serial.println((c == '-') ? F("-") : F("<none>"));
          }
//          else {
//            Serial.println();
//            Serial.print(F("Unexpected SIGN character 0x"));
//            Serial.println(c, HEX);
//          }
        }
        else if (nChars > 0 && nChars < 8) {               // receive WEIGHT
          if ((c >= '0' && c <= '9') || c == '.' || c == ' ' ) {
            buff[nChars - 1] = c;
            nChars++;
          }
          else {
            Serial.println();
            Serial.print(F("Unexpected WEIGHT character 0x"));
            Serial.print(c, HEX);
            Serial.print(F(" at position "));
            Serial.println(nChars);
          }
          if (nChars == 8) {
            buff[7] = 0;
            Serial.print(F("Weight received: "));
            Serial.println(buff);
          }
        }
        else if (nChars == 8) {                           // receive STATUS
          if (c == 'G' || c == 'N' || c == 'U' || c == 'O' || c == 'E') {
            stat = c;
//            Serial.print(F("Status received: "));
//            if (c == 'G') {
//              Serial.println(F("Gross"));
//            }
//            else if (c == 'N') {
//              Serial.println(F("Net"));
//            }
//            else if (c == 'U') {
//              Serial.println(F("Underload"));
//            }
//            else if (c == 'O') {
//              Serial.println(F("Overload"));
//            }
//            else if (c == 'E') {
//              Serial.println(F("Error"));
//            }
          }
          else {
            Serial.println();
            Serial.print(F("Unexpected STATUS character 0x"));
            Serial.println(c, HEX);
          }
          receiveState = COMPLETING;
        }
        break;

      case COMPLETING:
        if (c != ETX) {
          Serial.println();
          Serial.print(F("Unexpected ETX character 0x"));
          Serial.println(c, HEX);
        }
        else {
//          Serial.println(F("Transmission completed."));
          float f = atof(buff);
//          Serial.print(F("Weight as float: "));
//          Serial.println(f, 2);
//          Serial.println();
//          Serial.println();
//          Serial.println();
          scaleWeight = f;
        }
        receiveState = WAITING;
        nChars = 0;
        break;
    }
  }
}
