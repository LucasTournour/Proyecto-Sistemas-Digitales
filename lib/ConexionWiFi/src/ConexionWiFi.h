#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiMulti.h>

class ConexionWiFi {
 public:
  explicit ConexionWiFi(
      const char* nombrePortal = "ESP32-Configuracion",
      unsigned long intervaloReconexionMs = 5000,
      unsigned long esperaAntesDelPortalMs = 15000);

  void iniciar(Stream& salidaDiagnostico);
  void actualizar();

  bool conectado() const;
  IPAddress ipLocal() const;
  void borrarRedesGuardadas();

 private:
  static constexpr uint8_t MAXIMO_REDES = 8;

  void cargarRedesGuardadas();
  void agregarRedAlSelector(const String& ssid, const String& clave);
  void guardarRedActual();
  void iniciarPortal();
  void informarConexion();

  const char* nombrePortal_;
  unsigned long intervaloReconexionMs_;
  unsigned long esperaAntesDelPortalMs_;
  unsigned long ultimoIntento_ = 0;
  unsigned long desconectadoDesde_ = 0;
  Stream* salida_ = nullptr;
  WiFiMulti selectorRedes_;
  WiFiManager administradorWiFi_;
  Preferences preferencias_;
  uint8_t cantidadRedes_ = 0;
  bool estadoAnterior_ = false;
  bool portalActivo_ = false;
  bool preferenciasAbiertas_ = false;
};
