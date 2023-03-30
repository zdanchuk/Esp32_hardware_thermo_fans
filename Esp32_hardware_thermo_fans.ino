// #include <G433_crc.h>
// #include <Gyver433.h>

#include <pgmspace.h>  // PROGMEM support header
#include <FS.h>        // Filesystem support header
#include <SPIFFS.h>    // Filesystem support header
//#include <SD.h>
#include <Preferences.h>  // Used to store states before sleep/reboot
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include <HTTPClient.h>
#include <Math.h>
// #include <PNGdec.h>
#include "NotoSansBold15.h"
#include "time.h"
#include <Relay.h>

#include <AsyncTCP.h>          //Async Webserver support header
#include <ESPAsyncWebSrv.h> //Async Webserver support header

#include <Preferences.h> // Used to store states before sleep/reboot

Preferences savedStates;
AsyncWebServer webserver(80);
// #define AA_FONT_SMALL "NotoSansBold15"

#define SSID1 "Your_ssid_1"
#define PASSWORD1 "password_for_ssid1"
#define SSID2 "Your_ssid_2"
#define PASSWORD2 "password_for_ssid2"
#define RX2 16
#define TX2 17
#define LED_PIN GPIO_NUM_32

#define CPU_PWM GPIO_NUM_14
#define GPU_PWM GPIO_NUM_12
#define SSD_PWM GPIO_NUM_22

#define CPU_RELAY GPIO_NUM_33
#define GPU_RELAY GPIO_NUM_25
#define SSD_RELAY GPIO_NUM_26

#define CPU_TACHO GPIO_NUM_36
#define GPU_TACHO GPIO_NUM_39
#define SSD_TACHO GPIO_NUM_34

#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define MAX_IMAGE_WDITH 60

// Font size multiplier
#define KEY_TEXTSIZE 1

// Text Button Label Font
// #define LABEL_FONT &FreeSans9pt7b

// The pin where the IRQ from the touch screen is connected uses ESP-style GPIO_NUM_* instead of just pinnumber
#define touchInterruptPin GPIO_NUM_27

// Define the storage to be used.
#define FILESYSTEM SPIFFS
// PNG png;
// File pngfile;
int16_t pngXpos = 0;
int16_t pngYpos = 0;

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The FILESYSTEM file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData"

char cpuLogo[32] = "/cpu_logo.bmp";
char gpuLogo[32] = "/gpu_logo.bmp";
char ramLogo[32] = "/ram_logo.bmp";
char ssdLogo[32] = "/ssd_logo3.bmp";
char settingsLogo[32] = "/settings.bmp";
char micOnLogo[32] = "/mic_green_2.bmp";
char micOffLogo[32] = "/mic_red_2.bmp";
const char deserializeFailed[32] = "deserialize failed";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

char emptyHardwareName[32] = "";
// structure for save details for CPU and GPU
struct CGPU {
  float load = 0;
  float temperature = 0;
  float power = 0;
  char hardwareName[32] = "";
};

// structure for save details for MEMORY
struct RAM {
  float usedPercentage = 0;
  float usedMemory = 0;
  float availableMemory = 0;
  float allMemory = 0;
};

struct NVMe {
  float usedSpace = 0;
  float temperature = 0;
  float activity = 0;
  char hardwareName[32] = "";
};

CGPU gpu;
CGPU cpu;
RAM memory;
NVMe ssd;

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

// Set the width and height of your screen here:
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

//path to the directory the logo are in ! including leading AND trailing / !
char logopath[64] = "/logos/";

const String serverName = "http://192.168.1.2:8080";
const String shortUrl = "/sensors/short";
const String fullUrl = "/sensors/full";
const String getMicState = "/Multimedia/getMicState";
const String setMicState = "/Multimedia/changeMicState2/?state=";

unsigned long loopTime = 0;
unsigned long loopTime2 = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastTimeTouch = 0;
unsigned long lastTimeCheckWifi = 0;
bool isEnabled = true;
int setFanSpeed = 0;
int micState = 0;
bool isNeedRedrawMainInfo = false;

// Invoke the TFT_eSPI button class and create all the button objects
TFT_eSPI_Button key[10];


const double cpuMin = 35.0;
const double cpuMax = 90.0;
const double gpuMin = 35.0;
const double gpuMax = 90.0;
const double ssdMin = 35.0;
const double ssdMax = 60.0;
const int pwm_max = 255;

int SPEED_DIFF = 0;  // -100(%) - +100(%)

//--------- Internal references ------------
// (this needs to be below all structs etc..)
#include "ScreenHelper.h"
// #include "ConfigLoad.h"
#include "DrawHelper.h"
// #include "ConfigHelper.h"
// #include "UserActions.h"
// #include "Action.h"
// #include "Webserver.h"
#include "Touch.h"
#include "RestfullHelper.h"  // work with microservices on server
#include "fans.h"
//-------------------------------- SETUP --------------------------------------------------------------


void setup() {
  // Initialize serial port
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial2.begin(19200);
  delay(10);
  

  // --------------- init TFT ----------------------
  // ledcAttachPin(LED_PIN, 1);
  // ledcSetup(1, 12000, 8);  // 12 kHz PWM, 8-bit resolution
  // ledcWrite(1, 100);
  pinMode(LED_PIN, OUTPUT);  //TFT_LED
  digitalWrite(LED_PIN, HIGH);
  tft.init();
  tft.setAttribute(UTF8_SWITCH, true);
  tft.setRotation(1);  // Must be setRotation(0) for this sketch to work correctly
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Set the font colour AND the background colour so the anti-aliasing works
  tft.setCursor(0, 0);
  // tft.loadFont(NotoSansBold15);  // Must load the font first
  // Setup the Font used for plain text
  // tft.setFreeFont(LABEL_FONT);
  // Calibrate the touch screen and retrieve the scaling factors
  touch_calibrate();

  Serial.println(tft.getTextPadding());

  // --------------- init and connect Wi-Fi ----------------------
  tft.println();
  tft.print("Connecting to " + String(SSID1));
  bool isConnected = false;

  int count = 0;
  WiFi.begin(SSID1, PASSWORD1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
    count++;
    if (count > 50) {
      Serial.println("[WARNING]: " + String(SSID1) + ": Connection exception. Restarting");
      break;
    }
  }

  if (count > 50) {
    count = 0;
    tft.println();
    tft.print("Connecting to " + String(SSID2));
    WiFi.begin(SSID2, PASSWORD2);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      tft.print(".");
      count++;
      if (count > 50) {
        Serial.println("[WARNING]: " + String(SSID2) + ": Connection exception. Restarting");
        ESP.restart();
      }
    }
  }

  // Start to listen
  // server.begin();

  tft.println("");
  tft.println("WiFi connected.");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());


  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();



  // --------------- init SD card ----------------------
    if (!FILESYSTEM.begin()) {
    Serial.println("Formatting file system");
    FILESYSTEM.format();
    FILESYSTEM.begin();
  }
  // if (!FILESYSTEM.begin()) {
  //   Serial.println("Card Mount Failed");
  //   return;
  // }
  // uint8_t cardType = FILESYSTEM.cardType();

  Serial.print("[INFO]: Free Space: ");
  Serial.println(FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes());
  // if (cardType == CARD_NONE) {
  //   Serial.println("No SD card attached");
  //   return;
  // }

  // Serial.print("SD Card Type: ");
  // if (cardType == CARD_MMC) {
  //   Serial.println("MMC");
  // } else if (cardType == CARD_SD) {
  //   Serial.println("SDSC");
  // } else if (cardType == CARD_SDHC) {
  //   Serial.println("SDHC");
  // } else {
  //   Serial.println("UNKNOWN");
  // }

  // uint64_t cardSize = FILESYSTEM.cardSize() / (1024 * 1024);
  // Serial.printf("SD Card Size: %lluMB\n", cardSize);

  loopTime = millis();
  loopTime2 = millis() - 1000;
  lastTimeCheckWifi = millis();
  isEnabled = true;
  drawMainWindow();




  // --------------- init fans pwm ----------------------
  setupFans();

}

void loop() {
  uint16_t t_x = 0, t_y = 0;

  //At the beginning of a new loop, make sure we do not use last loop's touch.
  boolean pressed = tft.getTouch(&t_x, &t_y);

  if (pressed) {
    Serial.println("isTouched");
    if (isEnabled == true) {
      if (millis() - lastTimeTouch > 200) {
        for (int i = 0; i < 10; i++) {
          key[i].press(false);
        }
      }

      for (int i = 0; i < 10; i++) {
        if (key[i].contains(t_x, t_y)) {
          if (!key[i].isPressed()) {
            Serial.println(" pressed button " + String(i));
            key[i].press(true);

            if (i == 5) {
              int newState = abs(micState - 1);
              Serial.println(" !! Current state = " + String(micState) + ",  new state = " + String(newState));
              if (setNewMicState(newState) == true) {
                micState = newState;
              }
            }
          }
        } else {
          key[i].press(false);
        }
      }
    } else {
      Serial.println("Backlight set true");
      isEnabled = true;
      loopTime = millis();
    }
    lastTimeTouch = millis();
  }
  // --------------- control tft backlight ----------------------
  if (isEnabled == false) {
    digitalWrite(LED_PIN, LOW);
    // ledcWrite(1, 0);
  } else {
    // ledcWrite(1, 100);
    digitalWrite(LED_PIN, HIGH);
  }
  // Wait for an incomming connection
  if (millis() - loopTime2 > 1000) {
    // tft.println(WiFi.status());
    // --------------- request short info and parse answer ----------------------

    if (updateShortSernsorsInfo() == true) {
      lastTimeUpdate = millis();
    }
    loopFans();
    updateMicStateInfo();
    // --------------- display result ----------------------
    loopTime2 = millis();
    reprintMainLabels();
    printCpuToTFT();
    printGpuToTFT();
    printSsdToTFT();
    printMemoryToTFT();
  }

  if ((millis() - loopTime > 300000) and isEnabled == true) {
    isEnabled = false;
    Serial.println("Backlight set false");
    // tft.fillScreen(TFT_BLACK);
    // tft.setCursor(0, 0);  // Set cursor at top left of screen
  }

  if ((millis() - lastTimeUpdate > 300000) and cpu.temperature > 0) {
    clearValues();
  }

  if (millis() - lastTimeCheckWifi > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi.status() = " + String(WiFi.status()));
      ESP.restart();
    } else {
      lastTimeCheckWifi = millis();
    }
  }
}
