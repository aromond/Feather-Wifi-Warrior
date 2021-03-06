// Include the libraries for the OLED display, RTC, SD Card, and WiFi
#include "RTClib.h"
#include <SPI.h>
#include <SdFat.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SH110X.h>
#include "WiFi.h"
#include "Adafruit_ThinkInk.h"

//   --- EPD Display Setup ---
#define EPD_DC      33  // can be any pin, but required!
#define EPD_CS      15  // can be any pin, but required!
#define EPD_BUSY    -1  // can set to -1 to not use a pin (will wait a fixed delay)
#define SRAM_CS     -1  // can set to -1 to not use a pin (uses a lot of RAM!)
#define EPD_RESET   -1  // can set to -1 and share with chip Reset (can't deep sleep)
#define SPI_SCK   5      /* hardware SPI SCK pin      */
#define SPI_MOSI  18      /* hardware SPI SID/MOSI pin   */
#define SPI_MISO  19       /* hardware SPI MISO pin   */

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#define COLOR1 EPD_BLACK
#define COLOR2 EPD_LIGHT
#define COLOR3 EPD_DARK

/*
//   --- SD Card Variables ---
// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
const uint8_t chipSelect = SS;

// Interval between data records in milliseconds.
// The interval must be greater than the maximum SD write latency plus the
// time to acquire and write data to the SD to avoid overrun errors.
// Run the bench example to check the quality of your SD card.
const uint32_t SAMPLE_INTERVAL_MS = 1000;

// Log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "Data"
//------------------------------------------------------------------------------
// File system object.
SdFat sd;

// Log file.
SdFile file;

// Time in micros for next data record.
uint32_t logTime;

//==============================================================================
// User functions.  Edit writeHeader() and logData() for your requirements.

const uint8_t ANALOG_COUNT = 4;

//   --- SD Card Functions ---
// Write data header.
void writeHeader() {
  file.print(F("micros"));
  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
    file.print(F(",adc"));
    file.print(i, DEC);
  }
  file.println();
}
//------------------------------------------------------------------------------
// Log a data record.
void logData() {
  uint16_t data[ANALOG_COUNT];

  // Read all channels to avoid SD write latency between readings.
  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
    data[i] = analogRead(i);
  }
  // Write data to file.  Start with log time in micros.
  file.print(logTime);

  // Write ADC data to CSV record.
  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
    file.write(',');
    file.print(data[i]);
  }
  file.println();
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))
*/

//   --- OLED Variables ---
//Define the OLED Display
//Adafruit_SH110X display = Adafruit_SH110X(64, 128, &Wire);

// Some strings we will need for formatting SSID's to fit the OLED later
String readString;
String newString;

/*
// Stuff for the buttons
// OLED FeatherWing buttons map to different pins depending on board:
#if defined(ESP8266)
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2
#elif defined(ESP32)
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
#define BUTTON_A PA15
#define BUTTON_B PC7
#define BUTTON_C PC5
#elif defined(TEENSYDUINO)
#define BUTTON_A  4
#define BUTTON_B  3
#define BUTTON_C  8
#elif defined(ARDUINO_NRF52832_FEATHER)
#define BUTTON_A 31
#define BUTTON_B 30
#define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840 and 328p
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
#endif

//lets set some state variables for our button presses
int stateA = 0;
int stateB = 0;
int stateC = 0;
*/

//   --- WIFI Variables ---
//Lets set some variables to store info from our WiFi Scans
int netNum = 0;
int totalNet = 0;
typedef struct Single_Network {
  String SSID;
  uint8_t encryptionType;
  int32_t RSSI;
  uint8_t* BSSID;
  String BSSIDstr;
  int32_t channel;
  bool isHidden;
};
// Hold 50 networks
Single_Network Network[50];

//   --- RTC Variables ---
//Declare which RTC module is being used
RTC_PCF8523 rtc;

void setup () {

  //Lets Start up the Serial Connection
  Serial.begin(115200);

  //Lets put the WiFi card into station mode and be sure we aren't connected to any networks
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Wifi Setup Complete");


//#ifndef ESP8266
//  while (!Serial); // wait for serial port to connect. Needed for native USB
//#endif

  //If the RTC can't be found lets abort
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  //If the RTC has not been initialized or if it has lost power we need to set the time.
  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    //January 30, 2 at 3am you would call:
    //rtc.adjust(DateTime(2021, 1, 30, 0, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  // The PCF8523 can be calibrated for:
  //        - Aging adjustment
  //        - Temperature compensation
  //        - Accuracy tuning
  // The offset mode to use, once every two hours or once every minute.
  // The offset Offset value from -64 to +63. See the Application Note for calculation of offset values.
  // https://www.nxp.com/docs/en/application-note/AN11247.pdf
  // The deviation in parts per million can be calculated over a period of observation. Both the drift (which can be negative)
  // and the observation period must be in seconds. For accuracy the variation should be observed over about 1 week.
  // Note: any previous calibration should cancelled prior to any new observation period.
  // Example - RTC gaining 43 seconds in 1 week
  float drift = 43; // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
  float period_sec = (7 * 86400);  // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
  float deviation_ppm = (drift / period_sec * 1000000); //  deviation in parts per million (μs)
  float drift_unit = 4.34; // use with offset mode PCF8523_TwoHours
  //float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
  int offset = round(deviation_ppm / drift_unit);
  // rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct
  // rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration
  Serial.print("Offset is "); Serial.println(offset); // Print to control offset

/*
  //Lets start up the display for the OLED
  display.begin(0x3C, true); // Address 0x3C default
  display.display();
  display.clearDisplay();
  //Sets the OLED to a vertical orientation, can be set to 1 for horizontal display
  display.setRotation(2);

  //Serial.println("Button test");
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // Set our Font size, also this is a monochrome display so we only have one color
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
*/
}

//The main program Loop
void loop () {
  display.begin(THINKINK_GRAYSCALE4);
  display.clearBuffer();
  display.setTextSize(1);
  display.setTextColor(COLOR1);
  display.setRotation(3);
  //Lets clear the display
  //display.clearBuffer();
  //display.println("test");
  //display.println("test2");
  //display.display();
  //Lets set the time to a variable called time
  DateTime time = rtc.now();

  //Lets define some format options for printing our timestamps later using toString() from the RTC library.
  char bigtime[] = "hh:mmAP";
  char timestring[] = "hh:mm:ssAP";
  char datestring[] = "MM/DD/YYYY";

  //Set our Cursor at the very Top of the Display
  display.setCursor(0, 0);

  //Lets initiate a Wifi Scan and print any results we get to the OLED
  int n = WiFi.scanNetworks();
  if (n == 0) {
    display.println("no networks found");
  } else {
    display.setTextSize(2);
    display.print(n);
    display.println("networks");
    display.println();
    display.setTextSize(1);
    display.println("RSSI SSID");
    for (int i = 0; i < n; ++i) {
      readString = (WiFi.SSID(i));
      newString = readString.substring(0, 16);
      display.print(WiFi.RSSI(i));
      display.print("  ");
      display.println(newString);
      delay(10);
    }
  }


  //Lets move our cursor to the bottom of the Display and display the time
  display.setTextSize(3);
  display.setCursor(0, 270);
  display.print(time.toString( bigtime ));
  // display.println(time.toString( datestring ));
  // display.println(time.toString( timestring ));
  display.display();

  //Lets print the time out to the serial buffer as well
  Serial.println( time.toString( timestring ));
  delay(5000);
}

void epdprint(char *text, uint16_t color){
    display.setCursor(0,0);
      display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
}
