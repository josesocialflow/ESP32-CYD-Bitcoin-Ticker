#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Estructura para almacenar pares SSID / Contraseña
struct WifiNetwork {
  const char* ssid;
  const char* password;
};

// --- LISTA DE REDES CONOCIDAS ---
// Añade todas las redes que utilices (Casa, Oficina, Móvil, etc.)
const WifiNetwork KNOWN_NETWORKS[] = {
  {"Wifi_name_#1",    "password#1"},
  {"Wifi_name_#2",    "password#2"},
  {"Wifi_name_#3",    "password#3"},
  {"Wifi_name_#4",    "password#4"}
};

// Número total de redes configuradas en el array
const int NUM_NETWORKS = sizeof(KNOWN_NETWORKS) / sizeof(KNOWN_NETWORKS[0]);

#endif