#ifndef CYD_DISPLAY_H
#define CYD_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define XPT2046_IRQ  36
#define XPT2046_MOSI 13
#define XPT2046_MISO 12
#define XPT2046_CLK  14
#define XPT2046_CS   33

extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;

// Inicialización de la pantalla y el táctil ya calibrados
inline void iniciarPantalla() {
  tft.init();
  tft.setRotation(1); // Horizontal
  tft.fillScreen(TFT_BLACK);

  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(1);
}

// Función sencilla para obtener el toque corregido
inline bool leerToque(int &x, int &y) {
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    if (p.z > 400) { // Filtro de presión
      // Coordenadas con eje X e Y ya corregidos para la CYD 3.5"
      x = map(p.x, 3800, 300, 0, 480);
      y = map(p.y, 300, 3800, 0, 320);
      x = constrain(x, 0, 480);
      y = constrain(y, 0, 320);
      return true;
    }
  }
  return false;
}

#endif