# 📈 ESP32-CYD-Bitcoin-Ticker (ST7796)
Este código es un sencillo proyecto pensado para no ser el tipico panel de información metereológica, quizás esto
constituya un aliciente para realizar un proyecto económico, fácil (solo necesitamos la ESP32-CYD) y algo diferente. Espero que te animes a introducirte en el apasionante mundo de los microcontroladores.

Panel de monitoreo de Bitcoin en tiempo real, cotización en USD y EUR, basado en el microcontrolador ESP32 y la pantalla táctil CYD (Cheap Yellow Display) de 3.5 pulgadas.

El código ha sido realizado, compilado y subido desde Visual Studio Code.

## 🚀 Características
- **Datos en tiempo real:** Gráfico de velas (OHLCV) de 7 días y volumen de **Bitcoin (BTC/USDT)** directo desde la API pública de Binance.
- **Tiempo de refresco:** El código comtempla la actualización de los datos cada 5 minutos.
- **Precios dinámicos:** Actualización automática de máximos y mínimos diarios de la vela activa.
- **Conversion de Divisa:** Conmutación rápida entre tipos de cambio USD y EUR tocando el valor de cotización.
- **Reloj NTP:** Sincronización automática de hora y fecha.
- **Multired WiFi:** Reconocimiento y conexión automática a varias redes guardadas.
- **Identificación de Red:** Identifica el nombre de la red wifi a la que estamos conectados.

## 🛠️ Hardware
- **Placa:** ESP32 CYD 3.5" (Pantalla ST7796, resolución $480 \times 320$ px).


## 🎥 Video del proyecto
[![Ver vídeo demostrativo](https://img.youtube.com/vi/v3RNFjvWsig/maxresdefault.jpg)](https://www.youtube.com/watch?v=v3RNFjvWsig)


## 📂 Estructura del Proyecto
```text
├── include/
│   └── config.h          # Configuración para tus redes wifi
├── src/
│   └── main.cpp          # Código principal del dashboard
├── .gitignore            # Exclusión de archivos sensibles e innecesarios
├── platformio.ini        # Configuración de PlatformIO y librerías
└── README.md             # Documentación del proyecto


<details>
<summary>
  <img src="https://img.shields.io/badge/Contacto-black?style=for-the-badge&logo=protonmail" alt="Email">
</summary>

<br>

📧 **Email:** `jsocialflow [arroba] protonmail [punto] com`

</details>