#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

namespace TopicosProyecto {

extern const char TEMPERATURA[];
extern const char HUMEDAD[];
extern const char LIMITE[];
extern const char ALARMA[];
extern const char ESTADO[];

}  // namespace TopicosProyecto

using ManejadorLimite = void (*)(float nuevoLimite);

class ComunicacionMQTT {
 public:
  ComunicacionMQTT(const char* servidor, uint16_t puerto,
                   unsigned long intervaloReconexionMs = 3000,
                   unsigned long intervaloPublicacionMs = 2000);

  void iniciar(Stream& salidaDiagnostico);
  void actualizar(bool redDisponible);
  void definirManejadorLimite(ManejadorLimite manejador);
  void publicarDatosSiCorresponde(float temperatura, float humedad,
                                  float limite, bool alarmaActiva,
                                  bool sensorValido);

  bool conectado();

 private:
  bool conectar();
  void recibirMensaje(char* topico, byte* payload,
                      unsigned int longitud);
  static void callback(char* topico, byte* payload,
                       unsigned int longitud);

  const char* servidor_;
  uint16_t puerto_;
  unsigned long intervaloReconexionMs_;
  unsigned long intervaloPublicacionMs_;
  unsigned long ultimoIntento_ = 0;
  unsigned long ultimaPublicacion_ = 0;

  WiFiClient clienteWifi_;
  PubSubClient clienteMqtt_;
  Stream* salida_ = nullptr;
  ManejadorLimite manejadorLimite_ = nullptr;

  static ComunicacionMQTT* instanciaActiva_;
};
