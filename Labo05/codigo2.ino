#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


#define WLAN_SSID   "ARTEFACTOS"
#define WLAN_PASS   "87654321"


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""


#define TRIG_PIN  18
#define ECHO_PIN  19
#define PIN_R     25
#define PIN_G     26
#define PIN_B     27


#define RGB_ANODO_COMUN  false

#define PWM_FREQ 5000
#define PWM_RES  8

#define INTERVALO_PUBLICAR_MS 5000

#define DIST_CERCA_CM 20
#define DIST_MEDIA_CM 50


WiFiClient client;

Adafruit_MQTT_Client mqtt(
  &client,
  AIO_SERVER,
  AIO_SERVERPORT,
  AIO_USERNAME,
  AIO_KEY
);

Adafruit_MQTT_Publish feedDistancia =
  Adafruit_MQTT_Publish(
    &mqtt,
    AIO_USERNAME "/feeds/distancia"
  );

Adafruit_MQTT_Subscribe feedBoton =
  Adafruit_MQTT_Subscribe(
    &mqtt,
    AIO_USERNAME "/feeds/boton"
  );

float ultimaDistancia = -1;
bool ledEncendido = true;
unsigned long ultimoPublicar = 0;
unsigned long ultimoPing = 0;


void conectarWiFi();
void conectarMQTT();

float leerDistanciaCm();
float distanciaPromedio(int muestras);

void escribirRGB(uint8_t r, uint8_t g, uint8_t b);
void actualizarLED();


void setup() {

  Serial.begin(115200);

  delay(10);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  
  ledcAttach(PIN_R, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_G, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_B, PWM_FREQ, PWM_RES);

  escribirRGB(255, 255, 255);
  conectarWiFi();
  mqtt.subscribe(&feedBoton);
}


void loop() {

  conectarMQTT();

  Adafruit_MQTT_Subscribe *subscription;

  while ((subscription = mqtt.readSubscription(100))) {

    if (subscription == &feedBoton) {

      const char *mensaje = (char *)feedBoton.lastread;

      Serial.print("Toggle recibido: ");
      Serial.println(mensaje);

      if (strcmp(mensaje, "ON") == 0) {

        ledEncendido = true;

      } else if (strcmp(mensaje, "OFF") == 0) {

        ledEncendido = false;
      }

      actualizarLED();
    }
  }

 
  if (millis() - ultimoPublicar >= INTERVALO_PUBLICAR_MS) {

    ultimoPublicar = millis();

    float d = distanciaPromedio(5);

    if (d > 0) {

      ultimaDistancia = d;

      Serial.print("Distancia: ");
      Serial.print(d, 1);
      Serial.println(" cm");

      if (!feedDistancia.publish(d)) {

        Serial.println("Error al publicar la distancia");

      } else {

        Serial.println("Distancia publicada en Adafruit IO");
      }

    } else {

      ultimaDistancia = -1;

      Serial.println("Lectura fuera de rango o sin eco");
    }

    actualizarLED();
  }



  if (millis() - ultimoPing >= 30000) {

    ultimoPing = millis();

    if (!mqtt.ping()) {
      mqtt.disconnect();
    }
  }
}


float leerDistanciaCm() {

  // Limpiar TRIG
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Pulso de 10 microsegundos
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(
    ECHO_PIN,
    HIGH,
    30000
  );

  if (duracion == 0) {
    return -1;
  }


  float distancia = (duracion * 0.0343) / 2.0;

  // Rango que queremos utilizar
  if (distancia < 2 || distancia > 100) {
    return -1;
  }

  return distancia;
}


float distanciaPromedio(int muestras) {

  float suma = 0;

  int validas = 0;

  for (int i = 0; i < muestras; i++) {

    float d = leerDistanciaCm();

    if (d > 0) {

      suma += d;

      validas++;
    }

    delay(40);
  }

  if (validas > 0) {

    return suma / validas;

  } else {

    return -1;
  }
}


void escribirRGB(
  uint8_t r,
  uint8_t g,
  uint8_t b
) {

  if (RGB_ANODO_COMUN) {

    r = 255 - r;

    g = 255 - g;

    b = 255 - b;
  }

  ledcWrite(PIN_R, r);

  ledcWrite(PIN_G, g);

  ledcWrite(PIN_B, b);
}


#define DIST_CERCA_CM 10
#define DIST_MEDIA_CM 20
#define DIST_MAX_CM   30

void actualizarLED() {

  // El Toggle apagado
  if (!ledEncendido) {

    escribirRGB(0, 0, 0);

    return;
  }

  // No hay una distancia 
  if (ultimaDistancia <= 0 || ultimaDistancia > DIST_MAX_CM) {

    escribirRGB(255, 255, 255);

    return;
  }

  if (ultimaDistancia <= DIST_CERCA_CM) {

    // 0 - 10 cm -> ROJO
    escribirRGB(255, 0, 0);

  } else if (ultimaDistancia <= DIST_MEDIA_CM) {

    // 10 - 20 cm -> AMARILLO
    escribirRGB(255, 255, 0);

  } else {

    // 20 - 30 cm -> VERDE
    escribirRGB(0, 255, 0);
  }
}


void conectarWiFi() {

  Serial.print("Conectando a ");

  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.print("WiFi conectado. IP: ");

  Serial.println(WiFi.localIP());
}


void conectarMQTT() {

  if (mqtt.connected()) {
    return;
  }

  Serial.print("Conectando a Adafruit IO... ");

  int8_t ret;

  uint8_t intentos = 3;

  while ((ret = mqtt.connect()) != 0) {

    Serial.println(mqtt.connectErrorString(ret));

    Serial.println("Reintentando en 5 segundos...");

    mqtt.disconnect();

    delay(5000);

    intentos--;

    if (intentos == 0) {

      Serial.println(
        "No se pudo conectar. Reiniciando la ESP32..."
      );

      ESP.restart();
    }
  }

  Serial.println("¡Conectado a Adafruit IO!");
}