#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SH1106G display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

const int TOUCH_32 = 32;
const int TOUCH_33 = 33;

const int UMBRAL_TOUCH = 450;

void setup() {

  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!display.begin(OLED_ADDRESS, true)) {
    Serial.println("Error al iniciar la pantalla OLED");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
}

void loop() {

  int valorTouch32 = touchRead(TOUCH_32);
  int valorTouch33 = touchRead(TOUCH_33);

  // Mostrar también los valores en el monitor serial
  Serial.print("Touch 32: ");
  Serial.print(valorTouch32);

  Serial.print(" | Touch 33: ");
  Serial.println(valorTouch33);

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(26, 5);
  display.println("ESP32 TOUCH");

  bool tocado32 = valorTouch32 < UMBRAL_TOUCH;
  bool tocado33 = valorTouch33 < UMBRAL_TOUCH;

  if (tocado32 && tocado33) {

    display.setCursor(15, 28);
    display.println("TOUCH 32 Y 33");

    display.fillCircle(20, 52, 5, SH110X_WHITE);
    display.fillCircle(105, 52, 5, SH110X_WHITE);

  } 
  else if (tocado32) {

    display.setCursor(27, 28);
    display.println("TOUCH 32");

    display.fillCircle(20, 52, 5, SH110X_WHITE);

  } 
  else if (tocado33) {

    display.setCursor(27, 28);
    display.println("TOUCH 33");

    display.fillCircle(105, 52, 5, SH110X_WHITE);

  } 
  else {

    display.setCursor(32, 28);
    display.println("SIN TOCAR");

  }

  // Mostrar valores abajo
  display.setCursor(5, 55);
  display.print("32:");
  display.print(valorTouch32);

  display.setCursor(65, 55);
  display.print("33:");
  display.print(valorTouch33);

  display.display();

  delay(100);
}