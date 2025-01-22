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
#include <functions.h>

// Calls to libraries:
TFT_eSPI tft = TFT_eSPI();
BLEScan* pBLEScan;
std::set<String> foundDevices;

// Other global variables:
int scanTime = 1;
static unsigned int airTagCount = 0;
static unsigned int previousAirTagCount = 0;
int yPosition = 50; // Starting y position for the first AirTag display
String tagstatus = "";

// Debug
//Serial.println("tft.width: "+String(tft.width()));
//Serial.println("xcenter: "+String(xCenter));
//Serial.println("ycenter: "+String(yCenter));
//Serial.println("radius: "+String(radius));

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

                // Convert payload to string using the new function
                String payloadString = convertPayloadToString(payLoad, payLoadLength);
                
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
                Serial.println(payloadString);
//                for (size_t i = 0; i < payLoadLength; i++) {
//                    Serial.printf("%02X ", payLoad[i]);
//                }
                Serial.println("\n");

                // Save the tag info to SD card
                if (sdInitialized) {

                    // If the CSV file doesn't exist, create it and write out the CSV header
                    if (!SD.exists("/Tag_Info.csv")) {

                        File dataFile = SD.open("/Tag_Info.csv", FILE_WRITE);

                        if (dataFile) {

                            dataFile.println("count,status,mac_address,rssi,payload"); // Write header
                            Serial.println("SD: Tag_Info.csv did not exist, creating and adding header... ");
                            dataFile.close();

                        } else {

                            Serial.println("SD ERROR: Failed to create CSV file");
                        }
                    }

                    // We can always append here because if the file exists, great, if not, we should have created it above
                    File dataFile = SD.open("/Tag_Info.csv", FILE_APPEND);
                    if (dataFile) {

                        dataFile.print(airTagCount);
                        dataFile.print(",");
                        dataFile.print(tagstatus);
                        dataFile.print(",");
                        dataFile.print(macAddress);
                        dataFile.print(",");
                        dataFile.print(rssi);
                        dataFile.print(",");
                        dataFile.println(payloadString);
                        dataFile.close();
                        Serial.println("Tag info written to SD card.");

                    } else {

                        Serial.println("SD ERROR: Failed to open or create Tag_Info.csv for writing.");

                    }
                }

                // Display on the screen
                // Print Tag number in white
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
                tft.println(payloadString);
//                tft.println(payload);
                tft.println("");
            }
        }
    }
};

void setup() {

    // Setup the serial port
    Serial.begin(115200);
    Serial.println("Scanning for AirTags...");

    //------------------------------------------------------------------------------------------------
    // Setup the display
    //------------------------------------------------------------------------------------------------
    tft.init();
    tft.setRotation(0); // This is the display in landscape
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);

    // Set starting position for text
    int startX = 1; // 1 pixels from the left
    int startY = 35; // 25 pixels from the top
    tft.setCursor(startX, startY); // Set the cursor position
    
    tft.println("Scanning for BLE Trackers...");
    tft.println("");
    tft.println("");

    //------------------------------------------------------------------------------------------------
    // Draw the initial text at the top, should display the version and the airtag count as 0
    //------------------------------------------------------------------------------------------------
    drawAirTagCounter(tft, airTagCount);

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
    // Only update the AirTag counter on the screen if it has changed
    if (airTagCount != previousAirTagCount) {
        drawAirTagCounter(tft, airTagCount);
        previousAirTagCount = airTagCount;
    }
    delay(50);
}
