#include <BluetoothSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

BluetoothSerial SerialBT;

// ====== Joystick ======
#define VRX 34

#define RIGHT_THRESHOLD   1200
#define LEFT_THRESHOLD    2800
#define DEADZONE_LOW      1600
#define DEADZONE_HIGH     2400

// ====== LCD Pins ======
#define TFT_CS   5     
#define TFT_DC   2     
#define TFT_RST  4

#define RectWidth 30
#define RectHeight 10

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ===== Bluetooth Target =====
uint8_t hc05Address[6] = { 0x00, 0x14, 0x03, 0x05, 0x0A, 0xF1 };

enum JoyState { CENTER, LEFT, RIGHT };
JoyState currentState = CENTER;

// blinking timer
unsigned long lastBlink = 0;
bool showArrow = false;

// screen size
const int SCREEN_W = 160;
const int SCREEN_H = 128;

// connection timer
unsigned long lastRX = 0;
bool isConnected = false;

// periodic TX timer (100 ms)
unsigned long lastTX = 0;

// ===== Draw Left Arrow =====
void drawLeftArrow(bool on = 1) {
  int cx = SCREEN_W/2 - 50;   
  int cy = SCREEN_H/2 - 20;   

  if (!on) {
    tft.fillRect(cx-20, cy-10, RectWidth+21, 30, ST77XX_BLACK);
    return;
  }

  tft.fillRect(cx, cy, RectWidth, RectHeight, ST77XX_YELLOW);  
  tft.fillTriangle(cx, cy+RectHeight, cx, cy-10, cx-20, cy+RectHeight, ST77XX_YELLOW);
}

// ===== Draw Right Arrow =====
void drawRightArrow(bool on = 1) {
  int cx = SCREEN_W/2 + 20;  
  int cy = SCREEN_H/2 - 20;  

  if (!on) {
    tft.fillRect(cx, cy-10, RectWidth+21, 30, ST77XX_BLACK);
    return;
  }

  tft.fillRect(cx, cy, RectWidth, RectHeight, ST77XX_YELLOW);
  tft.fillTriangle(cx+RectWidth, cy+RectHeight, cx+RectWidth, cy-10, cx+RectWidth+20, cy+RectHeight, ST77XX_YELLOW);
}

// ===== Bottom “connected” =====
void drawConnected(bool ok) {
  tft.fillRect(0, SCREEN_H - 20, SCREEN_W, 20, ST77XX_BLACK);

  tft.setCursor(10, SCREEN_H - 18);
  tft.setTextSize(2);

  if (ok) {
    tft.setTextColor(ST77XX_BLUE);
    tft.println("connected");
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.println("disconnected");
  }
}

// ===== Print Acceleration =====
void drawAcceleration(float a) {
  tft.fillRect(0, 0, SCREEN_W, 20, ST77XX_BLACK);

  tft.setCursor(10, 2);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);

  tft.printf("%.2f m^2/s", a);
}

void setup() {
  Serial.begin(9600);
  pinMode(VRX, INPUT);

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);

  drawConnected(false);

  if (!SerialBT.begin("ESP32_MASTER", true)) {
    tft.setCursor(10, 20);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("BT FAIL");
    while (1);
  }

  SerialBT.setPin("1234", 4);
  Serial.print("Connecting...");
  if (SerialBT.connect(hc05Address)) {
    Serial.println("connected!");
  }
}

void loop() {
  int x = analogRead(VRX);

  // ====== Receive from ATmega @ HC-05 ======
  if (SerialBT.available()) {
      lastRX = millis();
      if (!isConnected) {
        isConnected = true;
        drawConnected(true);
      }

      String line = SerialBT.readStringUntil('\n');
      Serial.println("RX: " + line);

      // // Parse VAL:xx.xx
      // int valPos = line.indexOf("VAL:");
      // if (valPos != -1) {
      //   float v = line.substring(valPos + 4).toFloat();
      //   float a = v * 9.98;   // scale
      //   drawAcceleration(a);
      // }

      // ===== Parse ax=xxxx g =====
      int axPos = line.indexOf("ax=");
      if (axPos != -1) {
          int start = axPos + 3;                 // 跳过 "ax="
          int end = line.indexOf(" ", start);    // ax 的结束位置（空格前）

          String axStr = line.substring(start, end);
          float ax_g = axStr.toFloat();          // 单位为 g

          float ax_ms2 = ax_g * 9.98;            // 换算成 m^2/s
          drawAcceleration(ax_ms2);
      }

  }

  // ===== STATE CHANGE =====
  if (x < RIGHT_THRESHOLD && currentState != RIGHT) {
    currentState = RIGHT;
    SerialBT.write('L');
    drawRightArrow();
  }
  else if (x > LEFT_THRESHOLD && currentState != LEFT) {
    currentState = LEFT;
    SerialBT.write('R');
    drawLeftArrow();
  }
  else if (x >= DEADZONE_LOW && x <= DEADZONE_HIGH && currentState != CENTER) {
    currentState = CENTER;
    drawLeftArrow(false);
    drawRightArrow(false);
  }

  // ===== periodic TX every 100ms =====
  if (millis() - lastTX > 100) {
    lastTX = millis();

    if (currentState == LEFT) SerialBT.write('R');   // 注意你之前反着写
    else if (currentState == RIGHT) SerialBT.write('L');
    else SerialBT.write('C');   // idle时必须保持发C
  }

  // ===== Blinking at 0.2s =====
  if (millis() - lastBlink > 200) {
    lastBlink = millis();
    showArrow = !showArrow;

    if (currentState == LEFT) drawLeftArrow(showArrow);
    else if (currentState == RIGHT) drawRightArrow(showArrow);
  }

  // ===== Auto reconnect =====
  static unsigned long lastTry = 0;
  if (!SerialBT.connected() && millis() - lastTry > 3000) {
    lastTry = millis();
    SerialBT.connect(hc05Address);
  }

  // ===== Check timeout =====
  if (millis() - lastRX > 1000) {
      if (isConnected) {
          isConnected = false;
          drawConnected(false);
      }
  }
}
