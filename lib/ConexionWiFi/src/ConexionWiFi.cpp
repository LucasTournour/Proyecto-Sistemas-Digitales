#include "ConexionWiFi.h"

ConexionWiFi::ConexionWiFi(const char* nombreRed, const char* clave,
                           unsigned long intervaloReconexionMs)
    : nombreRed_(nombreRed),
      clave_(clave),
      intervaloReconexionMs_(intervaloReconexionMs) {}

void ConexionWiFi::iniciar(Stream& salidaDiagnostico) {
  salida_ = &salidaDiagnostico;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  conectar();
}

void ConexionWiFi::actualizar() {
  const bool conectadoAhora = conectado();

  if (conectadoAhora && !estadoAnterior_) {
    informarConexion();
  } else if (!conectadoAhora && estadoAnterior_) {
    if (salida_ != nullptr) {
      salida_->println("WiFi desconectado");
    }
  }

  estadoAnterior_ = conectadoAhora;
  if (conectadoAhora) {
    return;
  }

  const unsigned long ahora = millis();
  if (ahora - ultimoIntento_ >= intervaloReconexionMs_) {
    conectar();
  }
}

bool ConexionWiFi::conectado() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress ConexionWiFi::ipLocal() const {
  return WiFi.localIP();
}

void ConexionWiFi::conectar() {
  if (salida_ != nullptr) {
    salida_->print("Conectando a WiFi: ");
    salida_->println(nombreRed_);
  }

  WiFi.begin(nombreRed_, clave_);
  ultimoIntento_ = millis();
}

void ConexionWiFi::informarConexion() {
  if (salida_ == nullptr) {
    return;
  }

  salida_->println();
  salida_->println("WiFi conectado correctamente");
  salida_->print("Red: ");
  salida_->println(WiFi.SSID());
  salida_->print("IP del ESP32: ");
  salida_->println(WiFi.localIP());
}
