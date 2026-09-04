#pragma once

#include <Arduino.h>
#include <DHT.h>

class SensorAmbiente {
 public:
  SensorAmbiente(uint8_t pin, uint8_t tipo,
                 unsigned long intervaloLecturaMs = 2000);

  void iniciar(Stream& salidaDiagnostico);
  bool actualizar();

  float temperatura() const;
  float humedad() const;
  bool lecturaValida() const;

 private:
  DHT dht_;
  unsigned long intervaloLecturaMs_;
  unsigned long ultimaLectura_ = 0;
  Stream* salida_ = nullptr;
  float temperatura_ = 0.0F;
  float humedad_ = 0.0F;
  bool lecturaValida_ = false;
};
