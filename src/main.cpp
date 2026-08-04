#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <PubSubClient.h>
// ------------------------------------------------------------
// CONFIGURACIÓN DE WIFI
// ------------------------------------------------------------

// Reemplazar por el nombre y contraseña de tu red WiFi
const char *WIFI_SSID = "TP-Link_8812";
const char *WIFI_PASSWORD = "41880586";

// ============================================================
// CONFIGURACIÓN MQTT
// ============================================================

// Dirección IPv4 pública del servidor Oracle donde funciona Mosquitto
const char* mqtt_server = "161.153.217.107";
const uint16_t MQTT_PORT = 1883;

// Tópicos MQTT
const char* TOPIC_TEMPERATURA = "proyecto/temperatura";
const char* TOPIC_HUMEDAD     = "proyecto/humedad";
const char* TOPIC_LIMITE      = "proyecto/limite";
const char* TOPIC_ALARMA      = "proyecto/alarma";
const char* TOPIC_ESTADO      = "proyecto/estado";

// ============================================================
// CONFIGURACIÓN DEL SENSOR DHT22
// ============================================================

#define DHT_PIN  4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// CONFIGURACIÓN DE SALIDAS
// ============================================================

const uint8_t LED_ALARMA = 2;

// ============================================================
// OBJETOS WIFI Y MQTT
// ============================================================

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ============================================================
// VARIABLES DEL SISTEMA
// ============================================================

float temperatura = 0.0;
float humedad = 0.0;

// Límite inicial de temperatura
float temperaturaMaxima = 30.0;

bool alarmaActiva = false;
bool sensorValido = false;

// Tiempo entre lecturas del DHT22
const unsigned long INTERVALO_LECTURA = 2000;
unsigned long tiempoUltimaLectura = 0;

// Tiempo entre publicaciones MQTT
const unsigned long INTERVALO_PUBLICACION = 2000;
unsigned long tiempoUltimaPublicacion = 0;

// Tiempo entre intentos de reconexión MQTT
const unsigned long INTERVALO_RECONEXION = 3000;
unsigned long tiempoUltimoIntentoMQTT = 0;

// ============================================================
// CONEXIÓN WIFI
// ============================================================

void conectarWiFi()
{
    Serial.println();
    Serial.print("Conectando al WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi conectado correctamente");

    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());

    Serial.print("IP del broker MQTT: ");
    Serial.println(mqtt_server);
}

// ============================================================
// RECEPCIÓN DE MENSAJES MQTT
// ============================================================

void recibirMensajeMQTT(
    char* topic,
    byte* payload,
    unsigned int length
)
{
    String mensaje = "";

    for (unsigned int i = 0; i < length; i++)
    {
        mensaje += static_cast<char>(payload[i]);
    }

    mensaje.trim();

    Serial.println();
    Serial.print("Mensaje MQTT recibido en ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(mensaje);

    // Recibir el límite enviado desde Node-RED
    if (String(topic) == TOPIC_LIMITE)
    {
        float nuevoLimite = mensaje.toFloat();

        if (nuevoLimite >= -20.0 && nuevoLimite <= 80.0)
        {
            temperaturaMaxima = nuevoLimite;

            Serial.print("Nuevo límite de temperatura: ");
            Serial.print(temperaturaMaxima, 1);
            Serial.println(" °C");
        }
        else
        {
            Serial.println("Error: límite fuera de rango");
        }
    }
}

// ============================================================
// CONEXIÓN MQTT
// ============================================================

bool conectarMQTT()
{
    Serial.print("Conectando con Mosquitto en ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(MQTT_PORT);
    Serial.print("... ");

    // Identificador único para este ESP32
    String identificador = "ESP32-DHT22-";

    identificador += String(
        static_cast<uint32_t>(ESP.getEfuseMac()),
        HEX
    );

    /*
     * Última voluntad MQTT:
     * si el ESP32 pierde conexión inesperadamente,
     * Mosquitto publica "desconectado".
     */
    bool conectado = mqttClient.connect(
        identificador.c_str(),
        TOPIC_ESTADO,
        0,
        true,
        "desconectado"
    );

    if (conectado)
    {
        Serial.println("conectado");

        // Escuchar cambios del límite desde Node-RED
        mqttClient.subscribe(TOPIC_LIMITE);

        // Publicar estado actual
        mqttClient.publish(
            TOPIC_ESTADO,
            "conectado",
            true
        );

        return true;
    }

    Serial.print("falló. Código de error: ");
    Serial.println(mqttClient.state());

    return false;
}

// ============================================================
// CONTROL DE CONEXIÓN MQTT
// ============================================================

void controlarConexionMQTT()
{
    if (mqttClient.connected())
    {
        return;
    }

    unsigned long tiempoActual = millis();

    if (
        tiempoActual - tiempoUltimoIntentoMQTT
        >= INTERVALO_RECONEXION
    )
    {
        tiempoUltimoIntentoMQTT = tiempoActual;
        conectarMQTT();
    }
}

// ============================================================
// LECTURA DEL SENSOR DHT22
// ============================================================

void leerSensor()
{
    unsigned long tiempoActual = millis();

    if (
        tiempoActual - tiempoUltimaLectura
        < INTERVALO_LECTURA
    )
    {
        return;
    }

    tiempoUltimaLectura = tiempoActual;

    // Las lecturas se guardan en las variables globales
    temperatura = dht.readTemperature();
    humedad = dht.readHumidity();

    if (!isnan(temperatura) && !isnan(humedad))
    {
        sensorValido = true;

        Serial.println();
        Serial.print("Temperatura: ");
        Serial.print(temperatura, 1);
        Serial.print(" °C | Humedad: ");
        Serial.print(humedad, 1);
        Serial.println(" %");
    }
    else
    {
        sensorValido = false;

        Serial.println();
        Serial.println("Error al leer el DHT22");
    }
}

// ============================================================
// CONTROL DE LA ALARMA
// ============================================================

void controlarAlarma()
{
    if (!sensorValido)
    {
        alarmaActiva = false;
        digitalWrite(LED_ALARMA, LOW);

        return;
    }

    // La alarma se activa si la temperatura alcanza o supera el límite
    alarmaActiva = temperatura >= temperaturaMaxima;

    if (alarmaActiva)
    {
        digitalWrite(LED_ALARMA, HIGH);
    }
    else
    {
        digitalWrite(LED_ALARMA, LOW);
    }
}

// ============================================================
// PUBLICACIÓN DE DATOS POR MQTT
// ============================================================

void publicarDatosMQTT()
{
    unsigned long tiempoActual = millis();

    if (
        tiempoActual - tiempoUltimaPublicacion
        < INTERVALO_PUBLICACION
    )
    {
        return;
    }

    tiempoUltimaPublicacion = tiempoActual;

    if (!mqttClient.connected())
    {
        return;
    }

    if (!sensorValido)
    {
        mqttClient.publish(
            TOPIC_ESTADO,
            "error_sensor",
            true
        );

        return;
    }

    char textoTemperatura[16];
    char textoHumedad[16];
    char textoLimite[16];

    snprintf(
        textoTemperatura,
        sizeof(textoTemperatura),
        "%.1f",
        temperatura
    );

    snprintf(
        textoHumedad,
        sizeof(textoHumedad),
        "%.1f",
        humedad
    );

    snprintf(
        textoLimite,
        sizeof(textoLimite),
        "%.1f",
        temperaturaMaxima
    );

    // Publicar temperatura
    mqttClient.publish(
        TOPIC_TEMPERATURA,
        textoTemperatura,
        true
    );

    // Publicar humedad
    mqttClient.publish(
        TOPIC_HUMEDAD,
        textoHumedad,
        true
    );

    // Publicar límite configurado
    mqttClient.publish(
        TOPIC_LIMITE,
        textoLimite,
        true
    );

    // Publicar estado de alarma
    mqttClient.publish(
        TOPIC_ALARMA,
        alarmaActiva ? "1" : "0",
        true
    );

    // Publicar estado del ESP32
    mqttClient.publish(
        TOPIC_ESTADO,
        "conectado",
        true
    );

    Serial.println("Datos publicados en MQTT");

    Serial.print("Estado de alarma: ");

    if (alarmaActiva)
    {
        Serial.println("ACTIVA");
    }
    else
    {
        Serial.println("NORMAL");
    }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    pinMode(LED_ALARMA, OUTPUT);
    digitalWrite(LED_ALARMA, LOW);

    dht.begin();

    conectarWiFi();

    mqttClient.setServer(
        mqtt_server,
        MQTT_PORT
    );

    mqttClient.setCallback(
        recibirMensajeMQTT
    );

    mqttClient.setKeepAlive(30);
    mqttClient.setSocketTimeout(5);

    conectarMQTT();

    Serial.println();
    Serial.println("Sistema iniciado");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop()
{
    // Reconectar WiFi si se pierde la conexión
    if (WiFi.status() != WL_CONNECTED)
    {
        conectarWiFi();
    }

    // Reconectar MQTT si se pierde la conexión
    controlarConexionMQTT();

    // Atender mensajes recibidos por MQTT
    if (mqttClient.connected())
    {
        mqttClient.loop();
    }

    leerSensor();
    controlarAlarma();
    publicarDatosMQTT();

    delay(10);
}