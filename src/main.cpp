#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
// ------------------------------------------------------------
// CONFIGURACIÓN DE WIFI
// ------------------------------------------------------------

// Reemplazar por el nombre y contraseña de tu red WiFi
const char *ssid = "Personal-985-2.4GHz";
const char *password = "F7C0F5F985";

// ------------------------------------------------------------
// PINES Y SENSOR
// ------------------------------------------------------------

#define DHTPIN 4
#define DHTTYPE DHT22

const uint8_t PIN_LED = 2;

DHT dht(DHTPIN, DHTTYPE);

// ------------------------------------------------------------
// VARIABLES
// ------------------------------------------------------------

float temperatura = 0.0;
float humedad = 0.0;
float temperaturaMaxima = 30.0;

bool alarmaActiva = false;
bool sensorValido = false;

unsigned long tiempoUltimaLectura = 0;
const unsigned long INTERVALO_LECTURA = 2000;

WebServer servidor(80);

// ------------------------------------------------------------
// PÁGINA WEB
// ------------------------------------------------------------

const char paginaWeb[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">

<head>
    <meta charset="UTF-8">
    <meta
        name="viewport"
        content="width=device-width, initial-scale=1.0"
    >

    <title>Control de temperatura</title>

    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #eef2f7;
            margin: 0;
            padding: 0;
            text-align: center;
        }

        .contenedor {
            max-width: 500px;
            margin: 20px auto;
            padding: 20px;
        }

        .tarjeta {
            background-color: white;
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
        }

        h1 {
            color: #263238;
        }

        h2 {
            color: #455a64;
        }

        .temperatura {
            font-size: 55px;
            font-weight: bold;
            color: #1565c0;
            margin: 20px 0;
        }

        .humedad {
            font-size: 28px;
            color: #455a64;
            margin: 15px 0;
        }

        .normal {
            color: #2e7d32;
            font-size: 28px;
            font-weight: bold;
        }

        .alarma {
            color: #c62828;
            font-size: 28px;
            font-weight: bold;
        }

        .error {
            color: #e65100;
            font-size: 24px;
            font-weight: bold;
        }

        input {
            width: 120px;
            padding: 10px;
            font-size: 18px;
            text-align: center;
            border: 1px solid #999;
            border-radius: 6px;
        }

        button {
            padding: 11px 20px;
            margin-left: 8px;
            font-size: 17px;
            border: none;
            border-radius: 6px;
            background-color: #1565c0;
            color: white;
            cursor: pointer;
        }

        button:hover {
            background-color: #0d47a1;
        }

        .dato {
            font-size: 18px;
            margin: 10px;
        }
    </style>
</head>

<body>

    <div class="contenedor">

        <h1>Control de temperatura</h1>

        <div class="tarjeta">

            <h2>Temperatura actual</h2>

            <div class="temperatura">
                <span id="temperatura">--.-</span> °C
            </div>

            <div class="humedad">
                Humedad:
                <span id="humedad">--.-</span> %
            </div>

        </div>

        <div class="tarjeta">

            <h2>Configuración</h2>

            <p>Temperatura máxima permitida</p>

            <input
                type="number"
                id="limite"
                min="-20"
                max="80"
                step="0.1"
                value="30"
            >

            <button onclick="guardarLimite()">
                Guardar
            </button>

            <p class="dato">
                Límite actual:
                <span id="limiteActual">30.0</span> °C
            </p>

        </div>

        <div class="tarjeta">

            <h2>Estado del sistema</h2>

            <div id="estado" class="normal">
                ESPERANDO DATOS
            </div>

        </div>

    </div>

    <script>

        function actualizarDatos()
        {
            fetch("/datos")
                .then(respuesta => respuesta.json())
                .then(datos =>
                {
                    document.getElementById("limiteActual").innerText =
                        datos.limite.toFixed(1);

                    const estado =
                        document.getElementById("estado");

                    if (!datos.sensorValido)
                    {
                        document.getElementById("temperatura").innerText =
                            "--.-";

                        document.getElementById("humedad").innerText =
                            "--.-";

                        estado.innerText = "ERROR DE SENSOR";
                        estado.className = "error";

                        return;
                    }

                    document.getElementById("temperatura").innerText =
                        datos.temperatura.toFixed(1);

                    document.getElementById("humedad").innerText =
                        datos.humedad.toFixed(1);

                    if (datos.alarma)
                    {
                        estado.innerText = "TEMPERATURA ALTA";
                        estado.className = "alarma";
                    }
                    else
                    {
                        estado.innerText = "NORMAL";
                        estado.className = "normal";
                    }
                })
                .catch(error =>
                {
                    console.log("Error:", error);
                });
        }

        function guardarLimite()
        {
            const limite =
                document.getElementById("limite").value;

            fetch("/configurar?limite=" + limite)
                .then(respuesta => respuesta.text())
                .then(mensaje =>
                {
                    alert(mensaje);
                    actualizarDatos();
                });
        }

        setInterval(actualizarDatos, 2000);

        actualizarDatos();

    </script>

</body>

</html>
)rawliteral";

// ------------------------------------------------------------
// LECTURA DEL DHT22
// ------------------------------------------------------------

void leerSensor()
{
    unsigned long tiempoActual = millis();

    if (tiempoActual - tiempoUltimaLectura < INTERVALO_LECTURA)
    {
        return;
    }

    tiempoUltimaLectura = tiempoActual;

    float nuevaTemperatura = dht.readTemperature();
    float nuevaHumedad = dht.readHumidity();

    if (isnan(nuevaTemperatura) || isnan(nuevaHumedad))
    {
        sensorValido = false;

        Serial.println("Error al leer el DHT22");

        return;
    }

    temperatura = nuevaTemperatura;
    humedad = nuevaHumedad;
    sensorValido = true;

    Serial.print("Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.print(" °C | Humedad: ");
    Serial.print(humedad, 1);
    Serial.println(" %");
}

// ------------------------------------------------------------
// CONTROL DE ALARMA
// ------------------------------------------------------------

void controlarAlarma()
{
    if (!sensorValido)
    {
        alarmaActiva = false;
        digitalWrite(PIN_LED, LOW);

        return;
    }

    if (temperatura >= temperaturaMaxima)
    {
        alarmaActiva = true;
        digitalWrite(PIN_LED, HIGH);
    }
    else
    {
        alarmaActiva = false;
        digitalWrite(PIN_LED, LOW);
    }
}

// ------------------------------------------------------------
// RUTA PRINCIPAL
// ------------------------------------------------------------

void manejarPaginaPrincipal()
{
    servidor.send_P(
        200,
        "text/html",
        paginaWeb
    );
}

// ------------------------------------------------------------
// ENVÍO DE DATOS
// ------------------------------------------------------------

void manejarDatos()
{
    String json = "{";

    json += "\"temperatura\":";
    json += String(temperatura, 1);

    json += ",";

    json += "\"humedad\":";
    json += String(humedad, 1);

    json += ",";

    json += "\"limite\":";
    json += String(temperaturaMaxima, 1);

    json += ",";

    json += "\"alarma\":";
    json += alarmaActiva ? "true" : "false";

    json += ",";

    json += "\"sensorValido\":";
    json += sensorValido ? "true" : "false";

    json += "}";

    servidor.send(
        200,
        "application/json",
        json
    );
}

// ------------------------------------------------------------
// CAMBIO DEL LÍMITE DESDE LA WEB
// ------------------------------------------------------------

void manejarConfiguracion()
{
    if (!servidor.hasArg("limite"))
    {
        servidor.send(
            400,
            "text/plain",
            "Falta el valor del límite"
        );

        return;
    }

    float nuevoLimite =
        servidor.arg("limite").toFloat();

    if (nuevoLimite < -20.0 || nuevoLimite > 80.0)
    {
        servidor.send(
            400,
            "text/plain",
            "El límite debe estar entre -20 y 80 °C"
        );

        return;
    }

    temperaturaMaxima = nuevoLimite;

    Serial.print("Nuevo límite: ");
    Serial.print(temperaturaMaxima, 1);
    Serial.println(" °C");

    servidor.send(
        200,
        "text/plain",
        "Límite actualizado correctamente"
    );
}

// ------------------------------------------------------------
// RUTA NO ENCONTRADA
// ------------------------------------------------------------

void manejarRutaNoEncontrada()
{
    servidor.send(
        404,
        "text/plain",
        "Página no encontrada"
    );
}

// ------------------------------------------------------------
// CONEXIÓN WIFI
// ------------------------------------------------------------

void conectarWiFi()
{
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint8_t intentos = 0;

    while (
        WiFi.status() != WL_CONNECTED &&
        intentos < 30
    )
    {
        delay(500);
        Serial.print(".");
        intentos++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi conectado correctamente");

        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("No se pudo conectar al WiFi");
        Serial.println("Revisar nombre y contraseña");
    }
}

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    delay(1000);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    dht.begin();

    conectarWiFi();

    servidor.on(
        "/",
        HTTP_GET,
        manejarPaginaPrincipal
    );

    servidor.on(
        "/datos",
        HTTP_GET,
        manejarDatos
    );

    servidor.on(
        "/configurar",
        HTTP_GET,
        manejarConfiguracion
    );

    servidor.onNotFound(
        manejarRutaNoEncontrada
    );

    servidor.begin();

    Serial.println("Servidor web iniciado");
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------

void loop()
{
    leerSensor();
    controlarAlarma();
    servidor.handleClient();

    delay(10);
}