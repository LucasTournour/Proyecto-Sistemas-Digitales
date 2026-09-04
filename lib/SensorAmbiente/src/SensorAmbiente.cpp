#include "SensorAmbiente.h"

SensorAmbiente::SensorAmbiente(uint8_t pin, uint8_t tipo,
                               unsigned long intervaloLecturaMs)
    : dht_(pin, tipo), intervaloLecturaMs_(intervaloLecturaMs) {}

void SensorAmbiente::iniciar(Stream& salidaDiagnostico) {
  salida_ = &salidaDiagnostico;
  dht_.begin();
}

bool SensorAmbiente::actualizar() {
  const unsigned long ahora = millis();
  if (ahora - ultimaLectura_ < intervaloLecturaMs_) {
    return false;
  }
  ultimaLectura_ = ahora;

  const float nuevaTemperatura = dht_.readTemperature();
  const float nuevaHumedad = dht_.readHumidity();

  if (isnan(nuevaTemperatura) || isnan(nuevaHumedad)) {
    lecturaValida_ = false;
    if (salida_ != nullptr) {
      salida_->println();
      salida_->println("Error al leer el DHT22");
    }
    return true;
  }

  temperatura_ = nuevaTemperatura;
  humedad_ = nuevaHumedad;
  lecturaValida_ = true;

  if (salida_ != nullptr) {
    salida_->println();
    salida_->print("Temperatura: ");
    salida_->print(temperatura_, 1);
    salida_->print(" C | Humedad: ");
    salida_->print(humedad_, 1);
    salida_->println(" %");
  }
  return true;
}

float SensorAmbiente::temperatura() const {
  return temperatura_;
}

float SensorAmbiente::humedad() const {
  return humedad_;
}

bool SensorAmbiente::lecturaValida() const {
  return lecturaValida_;
}
