// Include the libraries for the OLED display, RTC, SD Card, and WiFi
#include "RTClib.h"
#include <SPI.h>
//#include <SdFat.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SH110X.h>
#include "WiFi.h"
#include "Adafruit_ThinkInk.h"
#include "SD.h"
#include "FS.h"

//   --- EPD Display Setup ---
#define EPD_DC      33  // can be any pin, but required!
#define EPD_CS      15  // can be any pin, but required!
#define EPD_BUSY    -1  // can set to -1 to not use a pin (will wait a fixed delay)
#define SRAM_CS     -1  // can set to -1 to not use a pin (uses a lot of RAM!)
#define EPD_RESET   -1  // can set to -1 and share with chip Reset (can't deep sleep)
#define SPI_SCK   5      /* hardware SPI SCK pin      */
#define SPI_MOSI  18      /* hardware SPI SID/MOSI pin   */
#define SPI_MISO  19       /* hardware SPI MISO pin   */

#define BUTTON_A 27
#define BUTTON_B 12
#define BUTTON_C 13

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#define COLOR1 EPD_BLACK
#define COLOR2 EPD_LIGHT
#define COLOR3 EPD_DARK

//   --- OLED Variables ---
//Define the OLED Display
//Adafruit_SH110X display = Adafruit_SH110X(64, 128, &Wire);

// Some strings we will need for formatting SSID's to fit the OLED later
String readString;
String newString;
float voltage;


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

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void setup () {

  
  //Lets Start up the Serial Connection
  Serial.begin(115200);


  // More SD Stuff

  if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

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


  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
}

//The main program Loop
void loop () {
  
 // writeFile(SD, "/SSIDs.txt", "SSIDs/n");
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
/*
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
    display.println();
    for (int i = 0; i < n; ++i) {
      readString = (WiFi.SSID(i));
      newString = readString.substring(0, 16);
      display.print(WiFi.RSSI(i));
      display.print("  ");
      display.println(newString);
      appendFile(SD, "/SSIDs.txt", WiFi.SSID(i).c_str());
      appendFile(SD, "/SSIDs.txt", "/n");
      delay(10);
    }
  }
*/


  

  //Lets move our cursor to the bottom of the Display and display the battery voltage
  //display.clearBuffer();
  display.setTextSize(1);
  display.setCursor(95, 285);
  voltage = analogRead(A13);
  display.print((voltage/4095)*2*3.3);
  display.println("v");

  //lets print the time at the bottom too
  display.setTextSize(2);
  display.setCursor(0,280);
  display.print(time.toString( bigtime ));
  display.setCursor(0,0);
    if (!digitalRead(BUTTON_A)) {
    display.print("a");
    display.display();
  }
    if (!digitalRead(BUTTON_B)) {
    display.print("b");
    display.display();
  }
    if (!digitalRead(BUTTON_C)) {
    display.print("c");
    display.display();
  }

  //Lets print the time out to the serial buffer as well
  Serial.println( time.toString( timestring ));
  //readFile(SD, "/SSIDs.txt");
  //delay(20000);
}

void epdprint(char *text, uint16_t color){
    display.setCursor(0,0);
      display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
}
