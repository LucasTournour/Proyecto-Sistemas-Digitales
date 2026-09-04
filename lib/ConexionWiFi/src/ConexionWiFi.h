#pragma once

#include <Arduino.h>
#include <WiFi.h>

class ConexionWiFi {
 public:
  ConexionWiFi(const char* nombreRed, const char* clave,
               unsigned long intervaloReconexionMs = 5000);

  void iniciar(Stream& salidaDiagnostico);
  void actualizar();

  bool conectado() const;
  IPAddress ipLocal() const;

 private:
  void conectar();
  void informarConexion();

  const char* nombreRed_;
  const char* clave_;
  unsigned long intervaloReconexionMs_;
  unsigned long ultimoIntento_ = 0;
  Stream* salida_ = nullptr;
  bool estadoAnterior_ = false;
};
