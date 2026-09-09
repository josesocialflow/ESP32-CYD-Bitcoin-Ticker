#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "CYD_Display.h"
#include "config.h"

// Servidor NTP y zona horaria (CET/CEST - España/Europa Central)
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;       // UTC +1
const int   daylightOffset_sec = 3600;  // Horario de verano (+1 hora)

// --- OBJETOS GLOBALES (Pantalla y táctil---
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// ESTRUCTURA DE CADA VELA OHLCV
struct Candle {
  float open;
  float high;
  float low;
  float close;
  float volume;
};

Candle candles[7];
float maxWeekHigh = 0;
float minWeekLow = 999999;
float maxWeekVolume = 0;

// Conversion EUR / USD y estado táctil
float eurUsdtRate = 1.0;
bool showInEUR = false;
String connectedSSID = "";

// Formatear el volumen (ej. 15400 -> "15.4K")
String formatVolume(float vol) {
  if (vol >= 1000000) return String(vol / 1000000.0, 1) + "M";
  if (vol >= 1000)    return String(vol / 1000.0, 1) + "K";
  return String(vol, 0);
}

// --- IMPRESION EN PANTALLA ---
void renderUI() {
  tft.fillScreen(TFT_BLACK);

  float rate = showInEUR ? (1.0 / eurUsdtRate) : 1.0;

  // 1. BARRA SUPERIOR - IZQUIERDA: Cotización de BTC principal
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(3);
  if (showInEUR) {
    tft.drawString(String(candles[6].close * rate, 2) + " EUR", 10, 10);
  } else {
    tft.drawString("$" + String(candles[6].close * rate, 2), 10, 10);
  }

  // 2. PANEL DERECHO: Hora/Fecha + Max/Min 24h (Límite de espera de 10ms para evitar congelamientos)
  struct tm timeinfo;
  char strTime[10];
  char strDate[12];
  
  if (getLocalTime(&timeinfo, 10)) { 
    strftime(strTime, sizeof(strTime), "%H:%M", &timeinfo);
    strftime(strDate, sizeof(strDate), "%d/%m", &timeinfo);
  } else {
    strcpy(strTime, "--:--");
    strcpy(strDate, "--/--");
  }

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(String(strDate) + " " + String(strTime), 280, 5);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("H:" + String((int)(candles[6].high * rate)), 280, 26);
  tft.drawString("L:" + String((int)(candles[6].low * rate)),  385, 26);

  // Divisora superior
  tft.drawFastHLine(0, 50, 480, TFT_DARKGREY);

  // 3. ETIQUETA DEL GRÁFICO Y RED CONECTADA
  tft.setTextSize(1);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.drawString("VELAS: SEMANAL (7 DIAS) [" + String(showInEUR ? "EUR" : "USD") + "]", 10, 55);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("WiFi: " + connectedSSID, 330, 55);

  // 4. GRÁFICO DE VELAS (Y = 70 a Y = 210)
  int chartX = 15;
  int chartY = 70;
  int chartWidth = 450;
  int chartHeight = 140;
  int candleSpacing = chartWidth / 7;

  float scaledMinLow = minWeekLow * rate;
  float scaledMaxHigh = maxWeekHigh * rate;

  for (int i = 0; i < 7; i++) {
    int x = chartX + (i * candleSpacing) + (candleSpacing / 2);

    int yHigh  = map(candles[i].high * rate,  scaledMinLow, scaledMaxHigh, chartY + chartHeight, chartY);
    int yLow   = map(candles[i].low * rate,   scaledMinLow, scaledMaxHigh, chartY + chartHeight, chartY);
    int yOpen  = map(candles[i].open * rate,  scaledMinLow, scaledMaxHigh, chartY + chartHeight, chartY);
    int yClose = map(candles[i].close * rate, scaledMinLow, scaledMaxHigh, chartY + chartHeight, chartY);

    uint16_t color = (candles[i].close >= candles[i].open) ? TFT_GREEN : TFT_RED;

    tft.drawFastVLine(x, yHigh, abs(yLow - yHigh) + 1, color);

    int bodyTop = min(yOpen, yClose);
    int bodyHeight = abs(yClose - yOpen);
    if (bodyHeight == 0) bodyHeight = 2;

    tft.fillRect(x - 7, bodyTop, 14, bodyHeight, color);
  }

  // Divisora volumen
  tft.drawFastHLine(0, 220, 480, TFT_DARKGREY);

  // 5. SECCIÓN DE VOLUMEN (Y = 225 a Y = 315)
  tft.setTextSize(1);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.drawString("VOLUMEN (BTC 24h): " + formatVolume(candles[6].volume), 10, 225);

  int volY = 240;
  int volHeight = 70;

  for (int i = 0; i < 7; i++) {
    int x = chartX + (i * candleSpacing) + (candleSpacing / 2);
    int barH = map(candles[i].volume, 0, maxWeekVolume, 0, volHeight);
    uint16_t color = (candles[i].close >= candles[i].open) ? TFT_DARKGREEN : TFT_MAROON;

    tft.fillRect(x - 7, (volY + volHeight) - barH, 14, barH, color);

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawCentreString(formatVolume(candles[i].volume), x, 312, 1);
  }
}

// --- CONEXIÓN MULTIRED WIFI ---
void conectarWiFiMulti() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Escaneando redes...", 20, 20);

  int n = WiFi.scanNetworks();
  
  for (int i = 0; i < n; ++i) {
    String ssidVisible = WiFi.SSID(i);
    for (int j = 0; j < NUM_NETWORKS; ++j) {
      if (ssidVisible == KNOWN_NETWORKS[j].ssid) {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Conectando a:", 20, 20);
        tft.drawString(KNOWN_NETWORKS[j].ssid, 20, 50);

        WiFi.begin(KNOWN_NETWORKS[j].ssid, KNOWN_NETWORKS[j].password);

        int intentos = 0;
        while (WiFi.status() != WL_CONNECTED && intentos < 20) {
          delay(500);
          Serial.print(".");
          intentos++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          connectedSSID = KNOWN_NETWORKS[j].ssid;
          return;
        }
      }
    }
  }
}

// --- CONSULTA A API PÚBLICA DE BINANCE ---
void fetchBinanceData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("https://api.binance.com/api/v3/klines?symbol=BTCUSDT&interval=1d&limit=7");
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      deserializeJson(doc, payload);

      maxWeekHigh = 0;
      minWeekLow = 999999;
      maxWeekVolume = 0;

      for (int i = 0; i < 7; i++) {
        candles[i].open   = doc[i][1].as<float>();
        candles[i].high   = doc[i][2].as<float>();
        candles[i].low    = doc[i][3].as<float>();
        candles[i].close  = doc[i][4].as<float>();
        candles[i].volume = doc[i][5].as<float>();

        if (candles[i].high > maxWeekHigh) maxWeekHigh = candles[i].high;
        if (candles[i].low < minWeekLow) minWeekLow = candles[i].low;
        if (candles[i].volume > maxWeekVolume) maxWeekVolume = candles[i].volume;
      }
    }
    http.end();

    // Obtener tasa conversion EUR/USDT
    http.begin("https://api.binance.com/api/v3/ticker/price?symbol=EURUSDT");
    int eurHttpCode = http.GET();

    if (eurHttpCode == HTTP_CODE_OK) {
      String eurPayload = http.getString();
      JsonDocument eurDoc;
      deserializeJson(eurDoc, eurPayload);
      eurUsdtRate = eurDoc["price"].as<float>();
    }
    http.end();

    renderUI();
  }
}

void setup() {
  Serial.begin(115200);

  iniciarPantalla();

  conectarWiFiMulti();

  if (WiFi.status() == WL_CONNECTED) {
    // 1. Forzando y configurando la sincronización horaria por NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Sincronizando hora...", 20, 20);

    // Espera activa de hasta 5 segundos para obtener la hora real de Internet
    struct tm timeinfo;
    int retry = 0;
    while(!getLocalTime(&timeinfo, 100) && retry < 50) {
      delay(100);
      retry++;
    }

    tft.fillScreen(TFT_BLACK);
    tft.drawString("Cargando Binance...", 20, 20);
    fetchBinanceData();
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Error: Red no encontrada", 20, 20);
  }
}

unsigned long lastUpdate = 0;

void loop() {
  // Refresca cada 5 MINUTOS (300.000 ms)
  // Aumentar o disminuir segun gustos
  if (millis() - lastUpdate > 300000) {
    fetchBinanceData();
    lastUpdate = millis();
  }

  // CONFIGURACION DE TÁCTIL EN PANTALLA
  int touchX, touchY;
  if (leerToque(touchX, touchY)) {
    // Zona para cambiar USD a EUR esquina superior izquierda, donde se muestra la cotización.
    if (touchX >= 0 && touchX <= 260 && touchY >= 0 && touchY <= 50) {
      showInEUR = !showInEUR;
      renderUI();
    } else {
      // Tocar en cualquier otra zona fuerza actualización de la info de Binance
      fetchBinanceData();
      lastUpdate = millis();
    }
    delay(400); // Debounce
  }
}