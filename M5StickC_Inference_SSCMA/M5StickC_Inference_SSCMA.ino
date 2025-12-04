#include <Seeed_Arduino_SSCMA.h>
#include "Base64.h"
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

SSCMA AI;

#define MAX_BASE64_LENGTH 1024 * 32
#define MAX_DECODED_LENGTH 1024 * 16

char inputString[MAX_BASE64_LENGTH];
uint8_t decodedImage[MAX_DECODED_LENGTH];

uint16_t  dmaBuffer1[16*16]; 
uint16_t  dmaBuffer2[16*16];
uint16_t* dmaBufferPtr = dmaBuffer1;
bool dmaBufferSel = 0;

TFT_eSPI tft = TFT_eSPI();

uint32_t x_offset = 53; // width m5stickC 135px, image width 240px, left & right offside is 53px
uint32_t y_offset = 0;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // if ( y >= tft.height() ) return 0;
    if (dmaBufferSel) dmaBufferPtr = dmaBuffer2;
    else dmaBufferPtr = dmaBuffer1;
    dmaBufferSel = !dmaBufferSel;
    tft.pushImageDMA(x, y, w, h, bitmap, dmaBufferPtr);
    // tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

void setup() {
    tft.begin();
    tft.setSwapBytes(true);
    tft.writecommand(TFT_MADCTL);
    tft.writedata(0xA8);  
    tft.initDMA();
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    AI.begin();
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

      tft.startWrite();
      TJpgDec.drawJpg(0 - x_offset, 0 - y_offset, decodedImage, decodedLength);
      tft.endWrite();

      draw_box();
    }
  }
  delay(1);
}

void draw_box() {
  if(AI.boxes().size() > 0) {
    uint32_t x = AI.boxes()[0].x;
    uint32_t y = AI.boxes()[0].y;
    uint32_t w = AI.boxes()[0].w;
    uint32_t h = AI.boxes()[0].h;
    x = uint32_t(x - w/2) - x_offset;
    y = uint32_t(y - h/2) - y_offset;
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.drawString("face", x, (y - 20), 2);
    tft.drawRect(x, y, w, h, TFT_RED);
  }
}
