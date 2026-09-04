#include "AlarmaTemperatura.h"

AlarmaTemperatura::AlarmaTemperatura(uint8_t pinLed,
                                     float limiteInicial,
                                     float limiteMinimo,
                                     float limiteMaximo)
    : pinLed_(pinLed),
      limite_(limiteInicial),
      limiteMinimo_(limiteMinimo),
      limiteMaximo_(limiteMaximo) {}

void AlarmaTemperatura::iniciar(Stream& salidaDiagnostico) {
  salida_ = &salidaDiagnostico;
  pinMode(pinLed_, OUTPUT);
  digitalWrite(pinLed_, LOW);
}

void AlarmaTemperatura::actualizar(bool sensorValido,
                                   float temperatura) {
  activa_ = sensorValido && temperatura >= limite_;
  digitalWrite(pinLed_, activa_ ? HIGH : LOW);
}

bool AlarmaTemperatura::establecerLimite(float nuevoLimite) {
  if (nuevoLimite < limiteMinimo_ || nuevoLimite > limiteMaximo_) {
    if (salida_ != nullptr) {
      salida_->println("Error: limite fuera de rango");
    }
    return false;
  }

  limite_ = nuevoLimite;
  if (salida_ != nullptr) {
    salida_->print("Nuevo limite de temperatura: ");
    salida_->print(limite_, 1);
    salida_->println(" C");
  }
  return true;
}

float AlarmaTemperatura::limite() const {
  return limite_;
}

bool AlarmaTemperatura::activa() const {
  return activa_;
}
