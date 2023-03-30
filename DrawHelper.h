
void drawKey(int16_t keyNum, char* filename, int16_t x, int16_t y, int16_t w, int16_t h) {
  // tft.setFreeFont(LABEL_FONT);
  key[keyNum].initButton(&tft, x + w / 2,
                         y + h / 2,  // x_centre, y_centre, w, h, outline, fill, text
                         w, h, TFT_WHITE, getBMPColor(filename), TFT_WHITE,
                         "", 1);
  key[keyNum].drawButton();

  drawBmp(filename, x, y);
}

void redrawMicLogo() {
  // todo: check it'c main screen or not

  if (micState == 1) {
    drawBmp(micOnLogo, 288, 8);
  } else {
    drawBmp(micOffLogo, 288, 8);
  }
}


void drawMainWindow() {
  Serial.println("!!!! drawMainWindow()");
  // todo: check it'c main screen or not
  tft.fillScreen(TFT_BLACK);
  drawKey(0, cpuLogo, 6, 6, 48, 48);
  tft.drawLine(0, 59, 240, 59, TFT_GREEN);
  drawKey(1, gpuLogo, 6, 66, 48, 48);
  tft.drawLine(0, 119, 240, 119, TFT_GREEN);
  drawKey(2, ssdLogo, 6, 126, 48, 48);
  tft.drawLine(0, 179, 240, 179, TFT_GREEN);
  drawKey(3, ramLogo, 6, 186, 48, 48);

  drawKey(4, settingsLogo, 272, 192, 48, 48);

  if (micState == 1) {
    drawKey(5, micOnLogo, 288, 8, 32, 32);
  } else {
    drawKey(5, micOffLogo, 288, 8, 32, 32);
  }
  drawKey(5, micOnLogo, 288, 8, 32, 32);


  reprintMainLabels();
}