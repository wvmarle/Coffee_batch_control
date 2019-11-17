#include <Encoder.h>
#include <LiquidCrystal.h>

/*******************************************************************************
   Timing and other settings.
 *******************************************************************************/
const uint32_t LCD_UPDATE_INTERVAL = 500;                   // How often to update the LCD display (interval in ms).
const uint32_t BATCH_DISCHARGE_TIME = 90000;                // For how long to keep buttefly valve 5 open (in ms). The discharge valve + contactor K4,K5
const uint32_t FILL_PAUSE_TIME = 5000;                      // 5 second delay between filling bins, to allow scale to stabilise.
const uint32_t CANCEL_DELAY = 2000;                         // Time in ms to hold the Stop button to cancel the process. 2 seconds.
const uint8_t MIN_WEIGHT = 0;                               // Minimum weight that can be selected for a bin (in kg).
const uint8_t MAX_WEIGHT = 100;                             // Maximum weight that can be selected for a bin (in kg).
const uint8_t MAX_BATCHES = 50;                             // Maximum number of batches that can be set.
const uint32_t TIMER_DELAY = 90000;                         // For how long to keep the timer high. Set for roasted beans transportation K6

const uint32_t DISCHARGE_RELAY_DELAY = 5000;                // After the batch is complete, wait for this time (in ms) before activating the complete relay.
const uint32_t DISCHARGE_RELAY_ONTIME = 5000;               // When the complete relay is activated, keep it activated for this period.
const uint32_t SCALE_TIMEOUT = 500;                         // If this long (in ms) no weight received, scale is disconnected. It normally sends the weight 10x per second.

//#define SHOW_TARGET                                       // Uncomment to not show target weight when filling.

/*******************************************************************************
   Pin definitions.

   Scale connection is Serial1: 19(RX), 18(TX)
   Other pins can be changed freely.
 *******************************************************************************/
const uint8_t NBINS = 4;                                    // Number of bins in the system.
Encoder setWeightEncoder(2, 3);                             // Rotary encoder for changing variables. At least one of the pins should be one of 2, 3, 20, 21.
                                                            //Connect a 100 nF capacitor between pin and GND for debouncing!!
const uint8_t encoderPushPin = 4;                           // The encoder's push button - for confirming a selection.
const uint8_t binSelectionButtonPin[NBINS] = {5, 6, 7, 8};  // The bin selection buttons (active LOW).
const uint8_t binSelectionLEDPin[NBINS] = {9, 10, 11, 12};  // The LEDs inside the buttons (lit when selected).
const uint8_t startButtonPin = 13;                          // Start button: starts the mixing process.
const uint8_t startButtonLEDPin = 34;                       // The LED inside the start button.
const uint8_t batchSelectionButtonPin = 35;                 // The batch selection button: select number of batches to run.
const uint8_t batchSelectionLEDPin = 36;                    // The LED inside the batch selection button (lit when selected).
const uint8_t stopButtonPin = 37;                           // The stop button (halts the process).
const uint8_t stopButtonLEDPin = 38;                        // The stop button (halts the process).

const uint8_t butterflyValvePin[NBINS] = {50, 51, 52, 53};  // Connections to the four butterfly valves handling the input hoppers.
const uint8_t dischargeValvePin = 47;                       // Connection to the fifth butterfly valve, handling the discharge.
const uint8_t INPUT1 = 40;                                  // INPUT1: if High, a batch can start running, if Low remain in standby.Connected with reley 83K1
const uint8_t valveOpenIndicatorPin = 42;                   // Pin goes high when any of the butterfly bin valves is open, low otherwise. Need for allow inverter start.


// Display pin assignments.
const uint8_t LCD_RS_PIN = 27;
const uint8_t LCD_E_PIN = 28;
const uint8_t LCD_D4_PIN = 29;
const uint8_t LCD_D5_PIN = 30;
const uint8_t LCD_D6_PIN = 31;
const uint8_t LCD_D7_PIN = 32;
const uint8_t LCD_BACKLIGHT_PIN = 39;                       // No connected on the hardware


LiquidCrystal lcd(LCD_RS_PIN, LCD_E_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

const uint8_t INPUT1LEDPin = 48;                            // Pin LED when System ready to receive more green beans.83K1
const uint8_t completeLEDPin = 49;                          // Pin LED when the all the batches are finish.

const uint8_t timerInput = 43;                              // INPUT2: Monitor when Gate K is open
const uint8_t timerOutput = 46;                             // Pin goes high when input 43 is high. This pin stay high for a specific time set.

const uint8_t dischargeSignalRelayPin = 54;                 // Relay goes HIGH for some time as the batch is being discharged.

/*******************************************************************************
   Various global variables.
 *******************************************************************************/
int16_t binTargetWeight[NBINS];                             // The individual target weights of the bins (in kg).
int16_t binWeight[NBINS];                                   // The individual weights of the bins (in kg) (not unsigned: instability in weight reading may drop it below 0!).
char systemStatus[21];                                      // A string, to be displayed on the LCD display.
bool updateDisplay = true;                                  // Request immediate display update.
uint16_t scaleWeight;                                       // The latest reading of the scale.
uint8_t selectedBin = NBINS;                                // The bin selected for setting target weight. Set to NBINS: no bin selected.
uint8_t nBatches = 1;                                       // The number of batches to run.
uint8_t nBatch;                                             // The current batch number.
uint32_t lastFillCompleteTime;                              // When the last bin filling was completed, or when the discharge started.
bool isDischarging;                                         // Indicate a batch has started discharge.
uint32_t isDischargingTime;                                 // When the batch started discharge.
uint32_t latestWeightReceivedTime;                          // Keep track of when we last got a weight.

enum ProcessStates {                                        // The various states the process can be in:
  SET_WEIGHTS,                                              // Setting the weight (no batch process active).
  STANDBY,                                                  // Batch starting: waiting for INPUT1 to go HIGH.
  FILLING_BIN,                                              // A bin is being filled.
  FILLING_PAUSE,                                            // After filling a bin, take a short break to let the scale stabilise.
  DISCHARGE_BATCH,                                          // Final stage of the process: discharge the batch.
  STOPPED,                                                  // Stop button pressed - process halted.
  COMPLETED,                                                // All batches done; process complete.
  WDT_TIMEOUT,                                              // Watchdog timer timeout: scale disconnected.
} processState;

void setup() {
  Serial1.begin(9600);
  initDisplay();
  initInputs();
  processState = SET_WEIGHTS;
  initTimer();
}

void loop() {
  readWeight();                                             // Get the latest weight from the scale (transmitted 10 times a second)
  handleDisplay();                                          // Keep the display up to date.
  handleInputs();                                           // Handle the button and encoder inputs.
  handleProcess();                                          // Handle the dispensing and batch mixing process.
  handleTimer();
}
