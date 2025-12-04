#include "Base64.h"
#include <TJpg_Decoder.h>
#include "SPI.h"
#include <TFT_eSPI.h>
#include <Seeed_Arduino_SSCMA.h>
#include "img.h"

SSCMA AI;

#define MAX_BASE64_LENGTH 1024 * 32
#define MAX_DECODED_LENGTH 1024 * 24

char inputString[MAX_BASE64_LENGTH];
uint8_t decodedImage[MAX_DECODED_LENGTH];

TFT_eSPI tft = TFT_eSPI();

// uint16_t  dmaBuffer1[16*16]; 
// uint16_t  dmaBuffer2[16*16];
// uint16_t* dmaBufferPtr = dmaBuffer1;
// bool dmaBufferSel = 0;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // if ( y >= tft.height() ) return 0;
  // if (dmaBufferSel) dmaBufferPtr = dmaBuffer2;
  // else dmaBufferPtr = dmaBuffer1;
  // dmaBufferSel = !dmaBufferSel;
  // tft.pushImageDMA(x, y, w, h, bitmap, dmaBufferPtr);
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 480, 50, 0x20E4);
  tft.fillRect(260, 60, 220, 240, 0x20E4);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Grove Vision AI Module V2", 20, 15, 1);
  TJpgDec.setJpgScale(1);
  tft.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  TJpgDec.drawJpg(260, 60, img, sizeof(img));
  if (!AI.begin()) {
    Serial.println("Failed to initialize AI module");
    while (1);
  }
}

void loop() {
if (!AI.invoke(1, false, true)) {
        if (AI.last_image().length() > 0) {
            String base64Image = AI.last_image();
            int inputLength = base64Image.length();
            if (inputLength > MAX_BASE64_LENGTH - 1) {
                Serial.println("Base64 input too long");
                return;
            }
            base64Image.toCharArray(inputString, inputLength + 1);

            int decodedLength = Base64.decodedLength(inputString, inputLength);
            if (decodedLength > MAX_DECODED_LENGTH) {
                Serial.println("Decoded image too large");
                return;
            }
            Base64.decode((char*)decodedImage, inputString, inputLength);

            TJpgDec.drawJpg(10, 60, decodedImage, decodedLength);
            draw_box();
        }
    }
    delay(1);
}

void draw_box() {
  if(AI.boxes().size() > 0) {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.drawString("face", (AI.boxes()[0].x - int(AI.boxes()[0].w/3)), AI.boxes()[0].y - 20, 2);
    tft.drawRect(
      switch_handler((AI.boxes()[0].x - int(AI.boxes()[0].w/3)) < 10, 10, (AI.boxes()[0].x - int(AI.boxes()[0].w/3))), 
      switch_handler(AI.boxes()[0].y < 60, 60, AI.boxes()[0].y), 
      switch_handler((AI.boxes()[0].x + AI.boxes()[0].w) > 250, 250 - AI.boxes()[0].x, AI.boxes()[0].w), 
      switch_handler((AI.boxes()[0].y + AI.boxes()[0].h) > 300, 300 - AI.boxes()[0].y, AI.boxes()[0].h), 
      0xf800);
  }
}

uint8_t switch_handler(bool statement, uint8_t value_true, uint8_t value_false) {
  if(statement) {
    return value_true;
  }
  else {
    return value_false;
  }
}