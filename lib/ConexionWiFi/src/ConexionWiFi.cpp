#include "ConexionWiFi.h"

namespace {

constexpr char ESPACIO_PREFERENCIAS[] = "Personal-985-2.4GHz";
constexpr char CLAVE_CANTIDAD[] = "F7C0F5F985";

String claveSsid(uint8_t indice) {
  return "ssid" + String(indice);
}

String clavePassword(uint8_t indice) {
  return "pass" + String(indice);
}

}  // namespace

ConexionWiFi::ConexionWiFi(const char* nombrePortal,
                           unsigned long intervaloReconexionMs,
                           unsigned long esperaAntesDelPortalMs)
    : nombrePortal_(nombrePortal),
      intervaloReconexionMs_(intervaloReconexionMs),
      esperaAntesDelPortalMs_(esperaAntesDelPortalMs) {}

void ConexionWiFi::iniciar(Stream& salidaDiagnostico) {
  salida_ = &salidaDiagnostico;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  preferenciasAbiertas_ = preferencias_.begin(ESPACIO_PREFERENCIAS, false);
  cargarRedesGuardadas();

  administradorWiFi_.setConfigPortalBlocking(false);
  administradorWiFi_.setConnectTimeout(10);

  if (cantidadRedes_ > 0) {
    salida_->print("Buscando ");
    salida_->print(cantidadRedes_);
    salida_->println(" red(es) WiFi guardada(s)...");
    selectorRedes_.run(8000);
  }

  if (conectado()) {
    estadoAnterior_ = true;
    guardarRedActual();
    informarConexion();
    return;
  }

  desconectadoDesde_ = millis();
  salida_->println();
  salida_->println("No se encontro una red conocida");
  salida_->print("Conectate a la red: ");
  salida_->println(nombrePortal_);
  salida_->println("Luego abri 192.168.4.1 para elegir el WiFi del lugar");

  // autoConnect prueba primero la ultima red conservada por el ESP32. Si no
  // funciona, deja abierto el portal sin bloquear el resto del programa.
  portalActivo_ = true;
  if (administradorWiFi_.autoConnect(nombrePortal_)) {
    portalActivo_ = false;
    estadoAnterior_ = true;
    guardarRedActual();
    informarConexion();
  }
}

void ConexionWiFi::actualizar() {
  administradorWiFi_.process();
  const bool conectadoAhora = conectado();

  if (conectadoAhora && !estadoAnterior_) {
    portalActivo_ = false;
    guardarRedActual();
    informarConexion();
  } else if (!conectadoAhora && estadoAnterior_) {
    if (salida_ != nullptr) {
      salida_->println("WiFi desconectado");
    }
    desconectadoDesde_ = millis();
  }

  estadoAnterior_ = conectadoAhora;
  if (conectadoAhora || portalActivo_) {
    return;
  }

  const unsigned long ahora = millis();
  if (ahora - ultimoIntento_ >= intervaloReconexionMs_) {
    ultimoIntento_ = ahora;
    selectorRedes_.run(1000);
  }

  if (!conectado() &&
      ahora - desconectadoDesde_ >= esperaAntesDelPortalMs_) {
    iniciarPortal();
  }
}

bool ConexionWiFi::conectado() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress ConexionWiFi::ipLocal() const {
  return WiFi.localIP();
}

void ConexionWiFi::borrarRedesGuardadas() {
  administradorWiFi_.resetSettings();
  if (preferenciasAbiertas_) {
    preferencias_.clear();
  }
  cantidadRedes_ = 0;

  if (salida_ != nullptr) {
    salida_->println("Redes WiFi guardadas eliminadas");
  }
}

void ConexionWiFi::cargarRedesGuardadas() {
  if (!preferenciasAbiertas_) {
    if (salida_ != nullptr) {
      salida_->println("No se pudo abrir la memoria de redes WiFi");
    }
    return;
  }

  cantidadRedes_ = preferencias_.getUChar(CLAVE_CANTIDAD, 0);
  if (cantidadRedes_ > MAXIMO_REDES) {
    cantidadRedes_ = MAXIMO_REDES;
  }

  for (uint8_t indice = 0; indice < cantidadRedes_; ++indice) {
    const String ssid = preferencias_.getString(claveSsid(indice).c_str(), "");
    const String clave =
        preferencias_.getString(clavePassword(indice).c_str(), "");
    if (!ssid.isEmpty()) {
      agregarRedAlSelector(ssid, clave);
    }
  }
}

void ConexionWiFi::agregarRedAlSelector(const String& ssid,
                                         const String& clave) {
  selectorRedes_.addAP(ssid.c_str(), clave.c_str());
}

void ConexionWiFi::guardarRedActual() {
  if (!preferenciasAbiertas_ || !conectado()) {
    return;
  }

  const String ssid = WiFi.SSID();
  const String clave = WiFi.psk();
  if (ssid.isEmpty()) {
    return;
  }

  for (uint8_t indice = 0; indice < cantidadRedes_; ++indice) {
    const String ssidGuardado =
        preferencias_.getString(claveSsid(indice).c_str(), "");
    if (ssidGuardado == ssid) {
      preferencias_.putString(clavePassword(indice).c_str(), clave);
      return;
    }
  }

  uint8_t indice = cantidadRedes_;
  if (cantidadRedes_ < MAXIMO_REDES) {
    ++cantidadRedes_;
  } else {
    // Si la lista esta llena, reemplaza la red mas antigua.
    for (uint8_t actual = 1; actual < MAXIMO_REDES; ++actual) {
      const String ssidSiguiente =
          preferencias_.getString(claveSsid(actual).c_str(), "");
      const String claveSiguiente =
          preferencias_.getString(clavePassword(actual).c_str(), "");
      preferencias_.putString(claveSsid(actual - 1).c_str(), ssidSiguiente);
      preferencias_.putString(clavePassword(actual - 1).c_str(),
                              claveSiguiente);
    }
    indice = MAXIMO_REDES - 1;
  }

  preferencias_.putString(claveSsid(indice).c_str(), ssid);
  preferencias_.putString(clavePassword(indice).c_str(), clave);
  preferencias_.putUChar(CLAVE_CANTIDAD, cantidadRedes_);
  agregarRedAlSelector(ssid, clave);

  if (salida_ != nullptr) {
    salida_->print("Red WiFi guardada: ");
    salida_->println(ssid);
  }
}

void ConexionWiFi::iniciarPortal() {
  if (portalActivo_) {
    return;
  }

  if (salida_ != nullptr) {
    salida_->println();
    salida_->println("No se encontro una red conocida");
    salida_->print("Conectate a la red: ");
    salida_->println(nombrePortal_);
    salida_->println("Luego abri 192.168.4.1 para elegir el WiFi del lugar");
  }

  portalActivo_ = true;
  administradorWiFi_.startConfigPortal(nombrePortal_);
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
