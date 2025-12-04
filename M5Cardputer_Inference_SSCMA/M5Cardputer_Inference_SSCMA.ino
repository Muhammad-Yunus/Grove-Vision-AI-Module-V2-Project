#include "M5Cardputer.h"
#include <Seeed_Arduino_SSCMA.h>
#include "Base64.h"
#include <TJpg_Decoder.h>

SSCMA AI;

#define MAX_BASE64_LENGTH   (1024 * 32)
#define MAX_DECODED_LENGTH  (1024 * 16)

char inputString[MAX_BASE64_LENGTH];
uint8_t decodedImage[MAX_DECODED_LENGTH];

// No DMA buffer needed in LovyanGFX
// We'll draw directly in callback

// display offsets (adjust for aspect ratio)
uint32_t x_offset = 0;
uint32_t y_offset = 0;

// ----------------------------------------
// JPEG DRAW CALLBACK FOR LOVYANGFX
// ----------------------------------------
bool jpg_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= M5Cardputer.Display.height()) return 0;

    M5Cardputer.Display.pushImage(x, y, w, h, bitmap);
    return 1;
}


// ----------------------------------------
// SETUP
// ----------------------------------------
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    // Display settings
    M5Cardputer.Display.setRotation(1);   // Landscape
    M5Cardputer.Display.setSwapBytes(true);

    // JPEG DECODER
    TJpgDec.setCallback(jpg_output);
    TJpgDec.setJpgScale(1);

    // AI CAMERA
    AI.begin();

    M5Cardputer.Display.fillScreen(BLACK);
}


// ----------------------------------------
// MAIN LOOP
// ----------------------------------------
void loop() {
    if (!AI.invoke(1, false, true)) {

        if (AI.last_image().length() > 0) {

            // ---- Base64 → raw JPEG ----
            String base64Image = AI.last_image();
            int inputLength = base64Image.length();

            if (inputLength >= MAX_BASE64_LENGTH) {
                Serial.println("Base64 too large");
                return;
            }

            base64Image.toCharArray(inputString, inputLength + 1);

            int decodedLength = Base64.decodedLength(inputString, inputLength);
            if (decodedLength > MAX_DECODED_LENGTH) {
                Serial.println("Decoded JPEG too large");
                return;
            }

            Base64.decode((char*)decodedImage, inputString, inputLength);

            // ---- DRAW JPEG ----
            M5Cardputer.Display.startWrite();

            TJpgDec.drawJpg(
                0 - x_offset,
                0 - y_offset,
                decodedImage,
                decodedLength
            );

            M5Cardputer.Display.endWrite();

            // ---- DRAW AI BOX ----
            draw_box();
        }
    }

    delay(1);
}


// ----------------------------------------
// DRAW DETECTION BOX
// ----------------------------------------
void draw_box() {

    if (AI.boxes().size() == 0) return;

    uint32_t x = AI.boxes()[0].x;
    uint32_t y = AI.boxes()[0].y;
    uint32_t w = AI.boxes()[0].w;
    uint32_t h = AI.boxes()[0].h;

    x = uint32_t(x - w/2) - x_offset;
    y = uint32_t(y - h/2) - y_offset;

    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setTextSize(1);

    M5Cardputer.Display.drawString("face", x, (y - 20));
    M5Cardputer.Display.drawRect(x, y, w, h, RED);
}
