#include <AlarmaTemperatura.h>
#include <Arduino.h>
#include <ComunicacionMQTT.h>
#include <ConexionWiFi.h>
#include <DHT.h>
#include <SensorAmbiente.h>

namespace {

constexpr char MQTT_SERVER[] = "161.153.217.107";
constexpr uint16_t MQTT_PORT = 1883;

// Cambia estos dos valores cada vez que uses el equipo en otra red.
constexpr char WIFI_SSID[] = "Profesores";
constexpr char WIFI_PASSWORD[] = "Profe2016";

constexpr uint8_t DHT_PIN = 4;
constexpr uint8_t LED_ALARMA = 2;

ConexionWiFi conexionWiFi(WIFI_SSID, WIFI_PASSWORD);
ComunicacionMQTT comunicacionMqtt(MQTT_SERVER, MQTT_PORT);
SensorAmbiente sensor(DHT_PIN, DHT22);
AlarmaTemperatura alarma(LED_ALARMA, 30.0F);

void recibirNuevoLimite(float nuevoLimite) {
  alarma.establecerLimite(nuevoLimite);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  alarma.iniciar(Serial);
  sensor.iniciar(Serial);
  conexionWiFi.iniciar(Serial);

  comunicacionMqtt.definirManejadorLimite(recibirNuevoLimite);
  comunicacionMqtt.iniciar(Serial);

  Serial.println();
  Serial.println("Sistema orientado a objetos iniciado");
}

void loop() {
  conexionWiFi.actualizar();
  comunicacionMqtt.actualizar(conexionWiFi.conectado());

  sensor.actualizar();
  alarma.actualizar(sensor.lecturaValida(), sensor.temperatura());

  comunicacionMqtt.publicarDatosSiCorresponde(
      sensor.temperatura(), sensor.humedad(), alarma.limite(),
      alarma.activa(), sensor.lecturaValida());

  delay(10);
}
