#include <SPI.h>
#include <LoRa.h>

// ======================================================
// Jose Everth Martinez Benavides 00156021
// Alexander Rafael Rogel Campos 00100922
// ======================================================

const int MI_ID = 3;
const int DESTINO_ID = 4;

String mensajeOriginal = "SENSOR ACTIVO";


#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_NSS   5

#define TOUCH_PIN     27
#define UMBRAL_TOUCH  900  // sin tocar: 1400-1500 | tocando: 260-400

bool tocadoAntes = false; 

char convertirAtbash(char letra) {

  if (letra >= 'A' && letra <= 'Z') {
    return 'Z' - (letra - 'A');
  }

  if (letra >= 'a' && letra <= 'z') {
    return 'z' - (letra - 'a');
  }

  return letra;   
}

String cifrarAtbash(String mensaje) {

  String mensajeCifrado = "";

  for (int i = 0; i < mensaje.length(); i++) {
    mensajeCifrado += convertirAtbash(mensaje.charAt(i));
  }

  return mensajeCifrado;
}


void enviarMensaje() {

  String mensajeCifrado = cifrarAtbash(mensajeOriginal);

  Serial.println();
  Serial.println("TOUCH DETECTADO");

  Serial.print("Mensaje original: ");
  Serial.println(mensajeOriginal);

  Serial.print("Mensaje cifrado: ");
  Serial.println(mensajeCifrado);

  
  LoRa.beginPacket();
  LoRa.print(MI_ID);
  LoRa.print("|");
  LoRa.print(DESTINO_ID);
  LoRa.print("|");
  LoRa.print(mensajeCifrado);
  LoRa.endPacket();

  Serial.print("Paquete enviado: ");
  Serial.print(MI_ID);
  Serial.print("|");
  Serial.print(DESTINO_ID);
  Serial.print("|");
  Serial.println(mensajeCifrado);

  Serial.println("----------------------------");
}



void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("TRANSMISOR LORA");

  Serial.print("Mi ID: ");
  Serial.println(MI_ID);

  Serial.print("Destino: ");
  Serial.println(DESTINO_ID);


  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);


  LoRa.setPins(LORA_NSS, -1, -1);

  if (!LoRa.begin(433E6)) {
    Serial.println("ERROR: No se pudo iniciar LoRa");
    while (true);
  }

  Serial.println("LoRa iniciado correctamente");
  Serial.println("Esperando toque...");
}



void loop() {

  int valorTouch = touchRead(TOUCH_PIN);
  bool tocado = (valorTouch < UMBRAL_TOUCH);


  if (tocado && !tocadoAntes) {
    enviarMensaje();
  }

  tocadoAntes = tocado;   

  delay(100);
}