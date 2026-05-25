#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <LoRa.h>
#include "SD_MMC.h"
#include "FS.h"

#define LORA_SS 5 
#define LORA_RST 16
#define LORA_DIO0 26

#define ERROR_LOG(msg) Serial.println(msg); return

Adafruit_BMP280 bmp = Adafruit_BMP280();
SPIClass lora_spi(VSPI);

uint32_t pacotesPerdidos = 0;
float altIni;
TaskHandle_t th_captacaoDados = NULL, //Prefixo th_ para handles de tasks
             th_transmissaoDados = NULL;
QueueHandle_t qh_dadosAltitude = NULL; //Prefixo qh_ para handles de queues
//Todo: queue do GPS depois de adicionar o módulo

void t_captacaoDados(void);
void t_transmissaoDados(void);

void setup() {
  File arquivo = File();

  Serial.begin(115200);

  if(!bmp.begin(0x76)){ ERROR_LOG("Erro: BMP não iniciado"); }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, // modo de operação
                Adafruit_BMP280::SAMPLING_X2, // temperatura
                Adafruit_BMP280::SAMPLING_X16, // pressão
                Adafruit_BMP280::FILTER_X16, //filtro de correção de dados
                Adafruit_BMP280::STANDBY_MS_1);
  altIni = bmp.readAltitude(1013.25);
  
  if(!SD_MMC.begin()){ ERROR_LOG("Erro: cartão SD não iniciado"); }
  arquivo = SD_MMC.open("/flight.txt", FILE_WRITE);

  LoRa.setSPI(lora_spi);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if(!LoRa.begin(915e6)){ ERROR_LOG("Erro: LoRa não iniciado"); }
  LoRa.setSyncWord(0xFE);
  LoRa.setSpreadingFactor(11);
  LoRa.setSignalBandwidth(62.5e3);

  qh_dadosAltitude = xQueueCreate(1024, sizeof(double));

  if(xTaskCreatePinnedToCore(
    t_captacaoDados,
    "Captacao de dados",
    5000,
    NULL,
    1,
    &th_captacaoDados,
    0
  ) == pdFALSE){ 
    ERROR_LOG("Erro: task \'Captacao de dados\' não iniciada"); 
  }

  if(xTaskCreatePinnedToCore(
    t_transmissaoDados,
    "Transmissao de dados",
    6000,
    NULL,
    1,
    &th_transmissaoDados,
    1
  ) == pdFALSE){ 
    ERROR_LOG("Erro: task \'Trasmissao de dados\' não iniciada");
  }
}

void loop() {
  
}

void t_captacaoDados(void *pvParameters){ //Prefixo t_ para tasks
  float alt = 0;
  BaseType_t xDadosFilaEnviados = pdFALSE;

  while(1){
    alt = bmp.readAltitude(1013.25) - altIni;
    
    xDadosFilaEnviados = xQueueSend(qh_dadosAltitude, &alt, portMAX_DELAY);

    if(xDadosFilaEnviados == pdFALSE) pacotesPerdidos++;
  }
}

void t_transmissaoDados(void *pvParameters){
  uint8_t fLoraDisponivel = 0; //Prefixo f para flags
  float alt = 0;
  BaseType_t xDadosFilaRecebidos = pdFALSE;
  String fraseEnvio = "";
  File arquivo = File();

  while(1){
    xDadosFilaRecebidos = xQueueReceive(qh_dadosAltitude, &alt, portMAX_DELAY);
    Serial.println(alt);
    
    if(xDadosFilaRecebidos == pdTRUE){
      fraseEnvio = String(alt) + ";" + String(pacotesPerdidos);
      arquivo = SD_MMC.open("/flight.txt", FILE_APPEND);

      if(arquivo){
        arquivo.print("sd corrompido: ");
        arquivo.println(fraseEnvio);
        arquivo.close();
      }else{
        Serial.println("Erro: arquivo não aberto");
      }

      fLoraDisponivel = LoRa.beginPacket();
      
      if(fLoraDisponivel == 1){
        LoRa.println(fraseEnvio);
        LoRa.endPacket();
      } else{
        Serial.println("LoRa indisponível");
      }
    }
  }
}