#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ------------------------------------------------------------
// CONFIGURACIÓN DE WIFI
// ------------------------------------------------------------

// Reemplazar por el nombre y contraseña de tu red WiFi
const char *ssid = "Personal-985-2.4GHz";
const char *password = "F7C0F5F985";

// ------------------------------------------------------------
// PINES
// ------------------------------------------------------------

// Potenciómetro conectado al GPIO34
const uint8_t PIN_POTENCIOMETRO = 34;

// LED conectado al GPIO2
const uint8_t PIN_LED = 2;

// ------------------------------------------------------------
// VARIABLES DEL SISTEMA
// ------------------------------------------------------------

int valorADC = 0;

float temperatura = 0.0;
float temperaturaMaxima = 30.0;

// Estado de la alarma
bool alarmaActiva = false;

// Servidor web en el puerto 80
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
            margin: 40px auto;
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

            <h2>Temperatura simulada</h2>

            <div class="temperatura">
                <span id="temperatura">0.0</span> °C
            </div>

            <div class="dato">
                Valor ADC:
                <span id="adc">0</span>
            </div>

        </div>

        <div class="tarjeta">

            <h2>Configuración</h2>

            <p>
                Temperatura máxima permitida
            </p>

            <input
                type="number"
                id="limite"
                min="0"
                max="100"
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
                NORMAL
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
                document.getElementById("temperatura").innerText =
                    datos.temperatura.toFixed(1);

                document.getElementById("adc").innerText =
                    datos.adc;

                document.getElementById("limiteActual").innerText =
                    datos.limite.toFixed(1);

                const estado = document.getElementById("estado");

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

        setInterval(actualizarDatos, 1000);

        actualizarDatos();

    </script>

</body>

</html>
)rawliteral";

// ------------------------------------------------------------
// LECTURA DEL POTENCIÓMETRO
// ------------------------------------------------------------

void leerTemperatura()
{
    valorADC = analogRead(PIN_POTENCIOMETRO);

    /*
     * El ADC entrega valores entre 0 y 4095.
     *
     * Se simula una temperatura entre:
     *
     * 0    ADC = 0 °C
     * 4095 ADC = 100 °C
     */

    temperatura = valorADC * 100.0 / 4095.0;
}

// ------------------------------------------------------------
// CONTROL DE LA SALIDA
// ------------------------------------------------------------

void controlarAlarma()
{
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
// RUTA PARA ENVIAR DATOS A LA PÁGINA
// ------------------------------------------------------------

void manejarDatos()
{
    String json = "{";

    json += "\"temperatura\":";
    json += String(temperatura, 1);

    json += ",";

    json += "\"adc\":";
    json += String(valorADC);

    json += ",";

    json += "\"limite\":";
    json += String(temperaturaMaxima, 1);

    json += ",";

    json += "\"alarma\":";
    json += alarmaActiva ? "true" : "false";

    json += "}";

    servidor.send(
        200,
        "application/json",
        json
    );
}

// ------------------------------------------------------------
// RUTA PARA RECIBIR EL NUEVO LÍMITE
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

    if (nuevoLimite < 0.0 || nuevoLimite > 100.0)
    {
        servidor.send(
            400,
            "text/plain",
            "El límite debe estar entre 0 y 100 °C"
        );

        return;
    }

    temperaturaMaxima = nuevoLimite;

    Serial.print("Nuevo límite: ");
    Serial.print(temperaturaMaxima);
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

    WiFi.begin(
        ssid,
        password
    );

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
// CONFIGURACIÓN INICIAL
// ------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    delay(1000);

    pinMode(
        PIN_POTENCIOMETRO,
        INPUT
    );

    pinMode(
        PIN_LED,
        OUTPUT
    );

    digitalWrite(
        PIN_LED,
        LOW
    );

    /*
     * Se configura el ADC del ESP32 con resolución de 12 bits.
     *
     * Valores posibles:
     * 0 a 4095
     */

    analogReadResolution(12);

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
// BUCLE PRINCIPAL
// ------------------------------------------------------------

void loop()
{
    leerTemperatura();

    controlarAlarma();

    servidor.handleClient();

    delay(10);
}