#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "ARTEFACTOS";
const char* WIFI_PASSWORD = "s^3sQ#9teskVgnb";

// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================

const char* MQTT_SERVER = "10.110.0.28";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

#define DHT_PIN 4        // DATA del DHT11
#define DHT_TYPE DHT11
#define LDR_PIN 32       // Punto medio del divisor LDR + 10k

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}

// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

  // 1 y 2. Obtener temperatura y humedad del DHT11.
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Si la lectura falla, no publicar datos inválidos.
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error leyendo el DHT11");
    return;
  }

  // 3. Leer la LDR (ADC de 12 bits: 0 a 4095)
  //    y convertirla a porcentaje (0 a 100 %).
  int ldrAdc = analogRead(LDR_PIN);
  int luz = map(ldrAdc, 0, 4095, 0, 100);
  luz = constrain(luz, 0, 100);

  // 4. MQTT envía texto, así que se convierten los números a cadenas.
  char tempStr[10];
  char humStr[10];
  char luzStr[10];

  dtostrf(temperatura, 1, 2, tempStr);
  dtostrf(humedad, 1, 2, humStr);
  snprintf(luzStr, sizeof(luzStr), "%d", luz);

  // 5. Publicar cada dato en su topic.
  //    true = mensaje retained: el broker guarda el último valor
  //    y lo entrega al dashboard apenas se conecta o recarga.
  mqttClient.publish(TOPIC_TEMPERATURA, tempStr, true);
  mqttClient.publish(TOPIC_HUMEDAD, humStr, true);
  mqttClient.publish(TOPIC_LUZ, luzStr, true);

  // 6. Mostrar las mediciones en el Monitor Serial.
  Serial.print("Temperatura: ");
  Serial.print(tempStr);
  Serial.println(" °C");

  Serial.print("Humedad: ");
  Serial.print(humStr);
  Serial.println(" %");

  Serial.print("LDR ADC: ");
  Serial.print(ldrAdc);
  Serial.print(" | Luz: ");
  Serial.print(luz);
  Serial.println(" %");

  Serial.println("------------------------------");
}

void setup() {

  Serial.begin(115200);

  // Inicializar sensores y pines.
  dht.begin();
  pinMode(LDR_PIN, INPUT);
  analogReadResolution(12);   // Lecturas de 0 a 4095

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}
