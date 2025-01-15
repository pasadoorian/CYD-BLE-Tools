#include <globals.h>
#include <TFT_eSPI.h>
#include <SPIFFS.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <set>
#include <SD.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
int scanTime = 1;
BLEScan* pBLEScan;
std::set<String> foundDevices;
static unsigned int airTagCount = 0;
static unsigned int previousAirTagCount = 0;
int yPosition = 30; // Starting y position for the first AirTag display
String tagstatus = "";

void drawAirTagCounter(TFT_eSPI &tft, int airTagCount) {
    int xCenter = tft.width() - 20; // Adjust as necessary for your screen size
    int yCenter = 20;
    int radius = 15;

    // Clear the area around the circle
    tft.fillRect(xCenter - radius, yCenter - radius, 2 * radius, 2 * radius, TFT_BLACK);

    // Draw the circle
    tft.fillCircle(xCenter, yCenter, radius, TFT_WHITE); // Circle with white color

    // Set the text properties
    //tft.setTextColor(TFT_BLACK, TFT_YELLOW); // Black text with yellow background
    tft.setTextColor(TFT_RED, TFT_WHITE); // Test with red text and white background
    tft.setTextFont(2); // Set font size
    tft.setTextDatum(MC_DATUM); // Middle Center text alignment

    // Convert airTagCount to String and draw it inside the circle
    tft.drawString(String(airTagCount), xCenter, yCenter);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
private:

    bool sdInitialized; // Store the SD card initialization status

public:

    // Constructor to initialize the flag
    MyAdvertisedDeviceCallbacks(bool sdInit) : sdInitialized(sdInit) {}

    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // raw data
        uint8_t* payLoad = advertisedDevice.getPayload();
        size_t payLoadLength = advertisedDevice.getPayloadLength();

        // searches both "1E FF 4C 00" and "4C 00 12 19" as the payload can differ slightly
        bool patternFound = false;
        // Make sure the payload is 4 or greater to avoid crashing
        if (payLoadLength >= 4) {
            for (int i = 0; i <= payLoadLength - 4; i++) {
                if (payLoad[i] == 0x4C && payLoad[i+1] == 0x00 && payLoad[i+2] == 0x07 && payLoad[i+3] == 0x19) {
                    patternFound = true;
                    tagstatus = "Registered ";
                    break;
                }           
                if (payLoad[i] == 0x4C && payLoad[i+1] == 0x00 && payLoad[i+2] == 0x12 && payLoad[i+3] == 0x19) {
                    patternFound = true;
                    tagstatus = "Unregistered ";
                    break;
                }
            }
        }

        if (patternFound) {
            String macAddress = advertisedDevice.getAddress().toString().c_str();
            macAddress.toUpperCase();

            if (foundDevices.find(macAddress) == foundDevices.end()) {
                foundDevices.insert(macAddress);
                airTagCount++;

                int rssi = advertisedDevice.getRSSI();
                
                // Print information to Serial Out
                Serial.print(tagstatus);
                Serial.println("AirTag found!");
                Serial.print("Tag: ");
                Serial.println(airTagCount);
                Serial.print("MAC Address: ");
                Serial.println(macAddress);
                Serial.print("RSSI: ");
                Serial.print(rssi);
                Serial.println(" dBm");
                Serial.print("Payload Data: ");
                for (size_t i = 0; i < payLoadLength; i++) {
                    Serial.printf("%02X ", payLoad[i]);
                }
                Serial.println("\n");

                // Save the tag info to SD card
                if (sdInitialized) {
                    File dataFile = SD.open("/Tag_Info.txt", FILE_WRITE);

                    if (dataFile) {
                        dataFile.print(tagstatus);
                        dataFile.print("Tag ");
                        dataFile.print(airTagCount);
                        dataFile.print(": ");
                        dataFile.print(macAddress);
                        dataFile.print(", RSSI: ");
                        dataFile.print(rssi);
                        dataFile.println(" dBm");
                        dataFile.close();
                        Serial.println("Tag info written to SD card.");
                    } else {
                        dataFile.close();
                        Serial.println("Failed to open or create Tag_Info.txt for writing.");
                    }
                }

                // Display on the screen
                // Print Tag number in white
                tft.setTextFont(1);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.print(String(tagstatus) + "Tag " + String(airTagCount) + ": ");

                // Print tag information in yellow on the same line
                tft.setTextColor(TFT_YELLOW, TFT_BLACK);
                tft.println(macAddress + ", RSSI: " + String(rssi) + " dBm");

                // Print "Payload:" in white on the next line
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.println("Payload:  ");

                // Print payload data in blue on the next line
                tft.setTextColor(TFT_BLUE, TFT_BLACK);
                tft.setTextFont(1);
                String payload = "";
                for (size_t i = 0; i < payLoadLength; i++) {
                    payload += String(payLoad[i], HEX) + " ";
                }
                tft.println(payload);
                tft.println("");
            }
        }
    }
};

void setup() {
    // Set the version - should read from .env in the future
    String version = "0.02-Beta";

    // Setup the serial port
    Serial.begin(115200);
    Serial.println("Scanning for AirTags...");

    //------------------------------------------------------------------------------------------------
    // Setup the display
    //------------------------------------------------------------------------------------------------
    tft.init();
    tft.setRotation(0); // This is the display in landscape
    tft.fillScreen(TFT_BLACK);
    int fontNum = 2; 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // Set starting position for text
    int startX = 1; // 1 pixels from the left
    int startY = 25; // 25 pixels from the top
    tft.setCursor(startX, startY); // Set the cursor position
    
    tft.println("Airtag Scanner Ver: " + version);
    tft.println("Scanning for BLE Trackers...");
    tft.println("");
    tft.println("");

    //------------------------------------------------------------------------------------------------
    // Initialize SD card
    //------------------------------------------------------------------------------------------------
    bool sdInitialized = false;

    if (!SD.begin()) {
        Serial.println("SD Card not found or initialization failed!");
        sdInitialized = false;
    } else {
        Serial.println("SD Card initialized.");
        sdInitialized = true;
    }

    //------------------------------------------------------------------------------------------------
    // Init the BLE device and start scanning
    //------------------------------------------------------------------------------------------------
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(sdInitialized), true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    delay(1000);
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.equals("rescan")) {
            foundDevices.clear();
            airTagCount = 0;
            yPosition = 30; // Reset the y position when rescanning
            tft.fillScreen(TFT_BLACK); // Clear the display
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString("Rescanning for AirTags...", 5, 10, 2);
            Serial.println("Rescanning for AirTags...");
        }
    }

    BLEScanResults foundDevicesScan = pBLEScan->start(scanTime, false);
    pBLEScan->clearResults();
    // Update the AirTag counter on the screen - but only if it has changed
    if (airTagCount != previousAirTagCount) {
        drawAirTagCounter(tft, airTagCount);
        previousAirTagCount = airTagCount;
    }
    delay(50);
}
