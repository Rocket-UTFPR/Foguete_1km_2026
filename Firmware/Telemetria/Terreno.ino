#include <SPI.h>
#include <LoRa.h>

// Definição de pinos padrão para o Heltec LoRa 32 (V2)
// Se usar o V3, a biblioteca Heltec.h facilita, mas esta configuração SPI funciona:
#define SS      18
#define RST     14
#define DIO0    26
#define BAND    915E6 // Frequência para o Brasil

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Configura os pinos SPI do LoRa interno do ESP32
  SPI.begin(5, 19, 27, 18);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Falha ao iniciar o Heltec LoRa!");
    while (1);
  }
  
  Serial.println("Heltec LoRa 32 Pronto! Aguardando dados...");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    String mensagem = "";
    bool capturando = false;

    while (LoRa.available()) {
      char c = (char)LoRa.read();
      
      if (c == '<') {
        capturando = true;
        mensagem = ""; 
      } else if (c == '>') {
        capturando = false;
        
        // Saída limpa no monitor serial do seu ESP32
        Serial.print("Recebido do LoRa32: ");
        Serial.println(mensagem);
        Serial.print("Sinal (RSSI): ");
        Serial.println(LoRa.packetRssi());
      } else if (capturando) {
        mensagem += c;
      }
    }
  }
}
