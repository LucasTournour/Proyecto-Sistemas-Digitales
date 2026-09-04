#include "ComunicacionMQTT.h"

#include <cstdlib>

namespace TopicosProyecto {

const char TEMPERATURA[] = "proyecto/temperatura";
const char HUMEDAD[] = "proyecto/humedad";
const char LIMITE[] = "proyecto/limite";
const char ALARMA[] = "proyecto/alarma";
const char ESTADO[] = "proyecto/estado";

}  // namespace TopicosProyecto

ComunicacionMQTT* ComunicacionMQTT::instanciaActiva_ = nullptr;

ComunicacionMQTT::ComunicacionMQTT(
    const char* servidor, uint16_t puerto,
    unsigned long intervaloReconexionMs,
    unsigned long intervaloPublicacionMs)
    : servidor_(servidor),
      puerto_(puerto),
      intervaloReconexionMs_(intervaloReconexionMs),
      intervaloPublicacionMs_(intervaloPublicacionMs),
      clienteMqtt_(clienteWifi_) {}

void ComunicacionMQTT::iniciar(Stream& salidaDiagnostico) {
  salida_ = &salidaDiagnostico;
  instanciaActiva_ = this;
  clienteMqtt_.setServer(servidor_, puerto_);
  clienteMqtt_.setCallback(callback);
  clienteMqtt_.setKeepAlive(30);
  clienteMqtt_.setSocketTimeout(5);
}

void ComunicacionMQTT::actualizar(bool redDisponible) {
  if (!redDisponible) {
    return;
  }

  if (clienteMqtt_.connected()) {
    clienteMqtt_.loop();
    return;
  }

  const unsigned long ahora = millis();
  if (ultimoIntento_ != 0 &&
      ahora - ultimoIntento_ < intervaloReconexionMs_) {
    return;
  }

  ultimoIntento_ = ahora;
  conectar();
}

void ComunicacionMQTT::definirManejadorLimite(
    ManejadorLimite manejador) {
  manejadorLimite_ = manejador;
}

void ComunicacionMQTT::publicarDatosSiCorresponde(
    float temperatura, float humedad, float limite,
    bool alarmaActiva, bool sensorValido) {
  const unsigned long ahora = millis();
  if (ahora - ultimaPublicacion_ < intervaloPublicacionMs_) {
    return;
  }
  ultimaPublicacion_ = ahora;

  if (!clienteMqtt_.connected()) {
    return;
  }

  if (!sensorValido) {
    clienteMqtt_.publish(TopicosProyecto::ESTADO, "error_sensor", true);
    return;
  }

  char textoTemperatura[16];
  char textoHumedad[16];
  char textoLimite[16];
  snprintf(textoTemperatura, sizeof(textoTemperatura), "%.1f",
           temperatura);
  snprintf(textoHumedad, sizeof(textoHumedad), "%.1f", humedad);
  snprintf(textoLimite, sizeof(textoLimite), "%.1f", limite);

  clienteMqtt_.publish(TopicosProyecto::TEMPERATURA,
                       textoTemperatura, true);
  clienteMqtt_.publish(TopicosProyecto::HUMEDAD, textoHumedad, true);
  clienteMqtt_.publish(TopicosProyecto::LIMITE, textoLimite, true);
  clienteMqtt_.publish(TopicosProyecto::ALARMA,
                       alarmaActiva ? "1" : "0", true);
  clienteMqtt_.publish(TopicosProyecto::ESTADO, "conectado", true);

  if (salida_ != nullptr) {
    salida_->println("Datos publicados en MQTT");
    salida_->print("Estado de alarma: ");
    salida_->println(alarmaActiva ? "ACTIVA" : "NORMAL");
  }
}

bool ComunicacionMQTT::conectado() {
  return clienteMqtt_.connected();
}

bool ComunicacionMQTT::conectar() {
  if (salida_ != nullptr) {
    salida_->print("Conectando con Mosquitto en ");
    salida_->print(servidor_);
    salida_->print(":");
    salida_->print(puerto_);
    salida_->print("... ");
  }

  String identificador = "ESP32-DHT22-";
  identificador += String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);

  const bool conectadoAhora = clienteMqtt_.connect(
      identificador.c_str(), TopicosProyecto::ESTADO, 0, true,
      "desconectado");

  if (!conectadoAhora) {
    if (salida_ != nullptr) {
      salida_->print("fallo. Codigo de error: ");
      salida_->println(clienteMqtt_.state());
    }
    return false;
  }

  clienteMqtt_.subscribe(TopicosProyecto::LIMITE);
  clienteMqtt_.publish(TopicosProyecto::ESTADO, "conectado", true);

  if (salida_ != nullptr) {
    salida_->println("conectado");
  }
  return true;
}

void ComunicacionMQTT::recibirMensaje(char* topico, byte* payload,
                                      unsigned int longitud) {
  String mensaje;
  mensaje.reserve(longitud);
  for (unsigned int i = 0; i < longitud; ++i) {
    mensaje += static_cast<char>(payload[i]);
  }
  mensaje.trim();

  if (salida_ != nullptr) {
    salida_->println();
    salida_->print("Mensaje MQTT recibido en ");
    salida_->print(topico);
    salida_->print(": ");
    salida_->println(mensaje);
  }

  if (String(topico) != TopicosProyecto::LIMITE) {
    return;
  }

  char* fin = nullptr;
  const float nuevoLimite = strtof(mensaje.c_str(), &fin);
  if (fin == mensaje.c_str() || *fin != '\0' ||
      nuevoLimite < 0.0F || nuevoLimite > 100.0F) {
    if (salida_ != nullptr) {
      salida_->println("Error: limite MQTT invalido");
    }
    return;
  }

  if (manejadorLimite_ != nullptr) {
    manejadorLimite_(nuevoLimite);
  }
}

void ComunicacionMQTT::callback(char* topico, byte* payload,
                                unsigned int longitud) {
  if (instanciaActiva_ != nullptr) {
    instanciaActiva_->recibirMensaje(topico, payload, longitud);
  }
}
