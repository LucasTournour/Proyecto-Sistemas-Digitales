#pragma once

#include <Arduino.h>

class AlarmaTemperatura {
 public:
  AlarmaTemperatura(uint8_t pinLed, float limiteInicial = 30.0F,
                    float limiteMinimo = 0.0F,
                    float limiteMaximo = 100.0F);

  void iniciar(Stream& salidaDiagnostico);
  void actualizar(bool sensorValido, float temperatura);
  bool establecerLimite(float nuevoLimite);

  float limite() const;
  bool activa() const;

 private:
  uint8_t pinLed_;
  float limite_;
  float limiteMinimo_;
  float limiteMaximo_;
  bool activa_ = false;
  Stream* salida_ = nullptr;
};
