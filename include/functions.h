#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <globals.h>

//--------------------------------------------------------------------------------------------------
// Function that will draw the banner at the top and display the version and airtag counter
//--------------------------------------------------------------------------------------------------

void drawAirTagCounter(TFT_eSPI &tft, int airTagCount) {

    int rectHeight = 30; // Height of the rectangle
    int screenWidth = tft.width();

    // Set the text properties
    tft.setTextColor(TFT_YELLOW, TFT_BLUE); // Red text on white background

    // only draw the rectangle and write the version the first time
    if (airTagCount == 0) {
        // Draw the rectangle at the top
        tft.fillRect(0, 0, screenWidth, rectHeight, TFT_BLUE); // White rectangle

        // Display software version on the left side
        tft.setTextDatum(ML_DATUM); // Middle-left alignment
        tft.drawString(VER, 5, rectHeight / 2); // Left-aligned at (x=5)
    }

    // Display the Airtag counter at the right hand side of the screen with the count in a larger red font
    tft.setTextDatum(MR_DATUM); // Right align the text

    int countX = screenWidth - 5; // AirTag count aligned to the right edge
    int labelX = countX - 40;     // "AirTags:" positioned slightly to the left

    tft.drawString("AirTags: ", labelX, rectHeight / 2);
    tft.setTextColor(TFT_RED, TFT_BLUE);
    tft.setTextSize(2);
    tft.drawString(String(airTagCount), countX, rectHeight / 2);
    // Restore the original text size
    tft.setTextSize(1);
}

//--------------------------------------------------------------------------------------------------
// Function to convert the payLoad to a hex string
//--------------------------------------------------------------------------------------------------

String convertPayloadToString(uint8_t* payLoad, size_t payLoadLength) {
    String payloadString = "";
    for (size_t i = 0; i < payLoadLength; i++) {
        // Convert each byte to a two-character hex value
        payloadString += String(payLoad[i], HEX);
        payloadString += " ";  // Add a space between bytes
    }
    return payloadString;
}

#endif // FUNCTIONS_H  <-- Make sure this is present!