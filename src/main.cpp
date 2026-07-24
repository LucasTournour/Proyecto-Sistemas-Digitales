#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <PubSubClient.h>
// ------------------------------------------------------------
// CONFIGURACIÓN DE WIFI
// ------------------------------------------------------------

// Reemplazar por el nombre y contraseña de tu red WiFi
const char *WIFI_SSID = "Personal-985-2.4GHz";
const char *WIFI_PASSWORD = "F7C0F5F985";

// ============================================================
// CONFIGURACIÓN MQTT
// ============================================================

// Colocar la dirección IPv4 de la computadora donde funciona Mosquitto.
// Para verla, ejecutar ipconfig en PowerShell.
const char* MQTT_BROKER = "192.168.0.145";

const uint16_t MQTT_PORT = 1883;

// Tópicos MQTT
const char* TOPIC_TEMPERATURA = "proyecto/temperatura";
const char* TOPIC_HUMEDAD     = "proyecto/humedad";
const char* TOPIC_LIMITE      = "proyecto/limite";
const char* TOPIC_ALARMA      = "proyecto/alarma";
const char* TOPIC_ESTADO      = "proyecto/estado";

// ============================================================
// CONFIGURACIÓN DEL SENSOR
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
float temperaturaMaxima = 30.0;

bool alarmaActiva = false;
bool sensorValido = false;

// Tiempo entre lecturas del DHT22
const unsigned long INTERVALO_LECTURA = 2000;
unsigned long tiempoUltimaLectura = 0;

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
    Serial.println(MQTT_BROKER);
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
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.print(MQTT_PORT);
    Serial.print("... ");

    String identificador = "ESP32-DHT22-";
    identificador += String(
        static_cast<uint32_t>(ESP.getEfuseMac()),
        HEX
    );

    /*
     * Última voluntad MQTT:
     * Si el ESP32 pierde conexión inesperadamente,
     * el broker publica "desconectado".
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

        // Suscripción para recibir el límite desde Node-RED
        mqttClient.subscribe(TOPIC_LIMITE);

        // Publicar estado de conexión
        mqttClient.publish(
            TOPIC_ESTADO,
            "conectado",
            true
        );

        // Publicar el límite actual
        char textoLimite[16];

        snprintf(
            textoLimite,
            sizeof(textoLimite),
            "%.1f",
            temperaturaMaxima
        );

        mqttClient.publish(
            TOPIC_LIMITE,
            textoLimite,
            true
        );

        return true;
    }

    Serial.print("falló. Código de error: ");
    Serial.println(mqttClient.state());

    return false;
}

// ============================================================
// RECONEXIÓN MQTT NO BLOQUEANTE
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
// LECTURA DEL DHT22
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

    float temperatura = dht.readTemperature();
float humedad = dht.readHumidity();

if (!isnan(temperatura) && !isnan(humedad))
{
    String tempTexto = String(temperatura, 1);
    String humTexto = String(humedad, 1);

    client.publish("proyecto/temperatura", tempTexto.c_str());
    client.publish("proyecto/humedad", humTexto.c_str());

    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" °C | Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");
}
else
{
    Serial.println("Error al leer el DHT22");
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

    alarmaActiva = temperatura >= temperaturaMaxima;

    digitalWrite(
        LED_ALARMA,
        alarmaActiva ? HIGH : LOW
    );
}

// ============================================================
// PUBLICACIÓN MQTT
// ============================================================

void publicarDatosMQTT()
{
    static unsigned long ultimaPublicacion = 0;

    unsigned long tiempoActual = millis();

    if (
        tiempoActual - ultimaPublicacion
        < INTERVALO_LECTURA
    )
    {
        return;
    }

    ultimaPublicacion = tiempoActual;

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

    mqttClient.publish(
        TOPIC_TEMPERATURA,
        textoTemperatura,
        true
    );

    mqttClient.publish(
        TOPIC_HUMEDAD,
        textoHumedad,
        true
    );

    mqttClient.publish(
        TOPIC_LIMITE,
        textoLimite,
        true
    );

    mqttClient.publish(
        TOPIC_ALARMA,
        alarmaActiva ? "1" : "0",
        true
    );

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
        MQTT_BROKER,
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
// LOOP
// ============================================================

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        conectarWiFi();
    }

    controlarConexionMQTT();

    if (mqttClient.connected())
    {
        mqttClient.loop();
    }

    leerSensor();
    controlarAlarma();
    publicarDatosMQTT();

    delay(10);
}
