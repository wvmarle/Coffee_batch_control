#include <Encoder.h>
#include <LiquidCrystal.h>

/*******************************************************************************
   Timing and other settings.
 *******************************************************************************/
uint32_t LCD_UPDATE_INTERVAL = 500;                         // How often to update the LCD display (interval in ms).
uint32_t BATCH_DISCHARGE_TIME = 4 * 60 * 1000;              // For how long to keep buttefly valve 5 open (in ms).
uint32_t FILL_PAUSE_TIME = 10000;                           // 10 second delay between filling bins, to allow scale to stabilise.
uint32_t CANCEL_DELAY = 1000;                               // Time in ms to hold the Stop button to cancel the process.
const uint8_t MIN_WEIGHT = 0;                               // Minimum weight that can be selected for a bin (in kg).
const uint8_t MAX_WEIGHT = 100;                             // Maximum weight that can be selected for a bin (in kg).
const uint8_t MAX_BATCHES = 255;                            // Maximum number of batches that can be set.

//const uint8_t MIN_RUN_WEIGHT = 50;                        // Minimum weight that must be selected to start a batch (in kg).
//const uint8_t MAX_RUN_WEIGHT = 150;                       // Maximum weight that is allowed to be selected to start a batch (in kg).


/*******************************************************************************
   Pin definitions.

   Scale connection is Serial1: 19(RX), 18(TX)
   Other pins can be changed freely.
 *******************************************************************************/
const uint8_t NBINS = 4;                                    // Number of bins in the system.
Encoder setWeightEncoder(2, 3);                             // Rotary encoder for changing variables. At least one of the pins should be one of 2, 3, 20, 21.
//                                                             Connect a 100 nF capacitor between pin and GND for debouncing!!
const uint8_t encoderPushPin = 4;                           // The encoder's push button - for confirming a selection.
const uint8_t binSelectionButtonPin[NBINS] = {5, 6, 7, 8};  // The bin selection buttons (active LOW).
const uint8_t binSelectionLEDPin[NBINS] = {9, 10, 11, 12};  // The LEDs inside the buttons (lit when selected).
const uint8_t startButtonPin = 13;                          // Start button: starts the mixing process.
const uint8_t startButtonLEDPin = 14;                       // The LED inside the start button.
const uint8_t batchSelectionButtonPin = 35;                 // The batch selection button: select number of batches to run.
const uint8_t batchSelectionLEDPin = 36;                    // The LED inside the batch selection button (lit when selected).
const uint8_t stopButtonPin = 37;                           // The stop button (halts the process).
const uint8_t stopButtonLEDPin = 38;                        // The stop button (halts the process).

const uint8_t butterflyValvePin[NBINS] = {22, 23, 24, 25};  // Connections to the four butterfly valves handling the input hoppers.
const uint8_t dischargeValvePin = 26;                       // Connection to the fifth butterfly valve, handling the discharge.
const uint8_t INPUT1 = 39;                                  // INPUT1: if High, a batch can start running, if Low remain in standby.

// Display pin assignments.
const uint8_t LCD_RS_PIN = 27;
const uint8_t LCD_E_PIN = 28;
const uint8_t LCD_D4_PIN = 29;
const uint8_t LCD_D5_PIN = 30;
const uint8_t LCD_D6_PIN = 31;
const uint8_t LCD_D7_PIN = 32;
const uint8_t LCD_BACKLIGHT_PIN = 44;
LiquidCrystal lcd(LCD_RS_PIN, LCD_E_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

const uint8_t INPUT1LEDPin = 33;
const uint8_t completeLEDPin = 40;

/*******************************************************************************
   Various global variables.
 *******************************************************************************/
int8_t binTargetWeight[NBINS];                              // The individual target weights of the bins (in kg).
//float binWeight[NBINS];                                     // The individual weights of the bins (in kg).
uint16_t binWeight[NBINS];                                     // The individual weights of the bins (in kg).
char systemStatus[21];                                      // A string, to be displayed on the LCD display.
bool updateDisplay = true;                                  // Request immediate display update.
//float scaleWeight;                                          // The latest reading of the scale.
uint16_t scaleWeight;                                       // The latest reading of the scale.
uint8_t selectedBin = NBINS;                                // The bin selected for setting target weight. Set to NBINS: no bin selected.
uint8_t nBatches;                                           // The number of batches to run.
uint8_t nBatch;                                             // The current batch number.
uint32_t lastFillCompleteTime;                              // When the last bin filling was completed, or when the discharge started.

enum ProcessStates {                                        // The various states the process can be in:
  SET_WEIGHTS,                                              // Setting the weight (no batch process active).
  STANDBY,                                                  // Batch starting: waiting for INPUT1 to go HIGH.
  FILLING_BIN,                                              // A bin is being filled.
  FILLING_PAUSE,                                            // After filling a bin, take a short break to let the scale stabilise.
  DISCHARGE_BATCH,                                          // Final stage of the process: discharge the batch.
  STOPPED,                                                  // Stop button pressed - process halted.
  COMPLETED,                                                // All batches done; process complete.
} processState;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  initDisplay();
  initInputs();
  processState = SET_WEIGHTS;
}

void loop() {
  readWeight();                                             // Get the latest weight from the scale (transmitted 10 times a second)
  handleDisplay();                                          // Keep the display up to date.
  handleInputs();                                           // Handle the button and encoder inputs.
  handleProcess();                                          // Handle the dispensing and batch mixing process.
}
