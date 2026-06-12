/*******************************************************
Código da placa principal do foguete de 1km para a LASC

Código para ESP32 Dev Module

Início: 2026/01
Término: 

Requisitos implementados:

Aquisição de dados com BMP280
Transmissão de dados com LoRa XL1278
Gravação de dados com socket SDMMC

Core 0: aquisição de sensores
Core 1: transmissão via LoRa
        gravação SD
Escalonamento por afinidade implementado com FreeRTOS
Comunicação por fila FreeRTOS

To-do:

Atualização de código OTA
Captação e envio de dados GPS
Core 0: Acionamento de paraquedas

Atualização de código OTA será necessário, porque alguns
pinos do socket do SD são usados como bootstrapping

Convenções de nomenclatura:

t_  : funções de tasks
qh_ : handles de filas RTOS
th_ : handles de tasks RTOS
f   : flags booleanas
x   : BaseType_t RTOS 

Convenções para nomes de arquivos:

/flight.txt : dados de voo/telemetria
/wifi.txt   : credenciais wifi (NUNCA deixar hardcoded)
/static.txt : dados de teste estático
********************************************************/

/*** Cabeçalho ***/
#include <HardwareSerial.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>
#include "SD_MMC.h"
#include "FS.h"

#define LORA_SS 5 
#define LORA_RST 16
#define LORA_DIO0 26
#define GPS_RX 17 // RX do ESP, TX do GPS
#define GPS_TX 4 // TX do ESP, RX da GPS

#define ERROR_LOG(msg) \
   do{ \
    Serial.println(msg); \
   } while(0) //Do-while para evitar bugs

Adafruit_BMP280 bmp = Adafruit_BMP280();
SPIClass lora_spi(VSPI);
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

uint32_t pacotesPerdidos = 0;
float altIni = 0;

struct dadosTelemetria{
  float altitude;
  double latitude,
          longitude;
  bool newGpsData;
};

TaskHandle_t th_captacaoDados = NULL,
             th_transmissaoDados = NULL;
QueueHandle_t qh_dadosAltitude = NULL;
//Todo: queue do GPS depois de adicionar o módulo

void t_captacaoDados(void);
void t_transmissaoDados(void);

void setup() {
  File arquivo = File();

  Serial.begin(115200);

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  if(!bmp.begin(0x76)) ERROR_LOG("Erro: BMP não iniciado");
  else{
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, // modo de operação
                  Adafruit_BMP280::SAMPLING_X2, // temperatura
                  Adafruit_BMP280::SAMPLING_X16, // pressão
                  Adafruit_BMP280::FILTER_X16, //filtro de correção de dados
                  Adafruit_BMP280::STANDBY_MS_1);
    altIni = bmp.readAltitude(1013.25);
  }
  if(!SD_MMC.begin()){ ERROR_LOG("Erro: cartão SD não iniciado"); }
  arquivo = SD_MMC.open("/flight.txt", FILE_WRITE);

  if(!arquivo){ ERROR_LOG("Erro: arquivo não aberto");}

  /*************************** Tabela de alcance LoRa (empírica) ***************************
  SF | BW (kHZ) | Velocidade aproximada (kbps) | Alcance campo aberto | Alcance urbano
  6  | 500      |              ~25             |       500m - 1km     | 100m - 300m
  7  | 125      |              ~9              |       1km - 2 km     | 300m - 800m
  8  | 125      |              ~4.5            |       2km - 4km      | 400m - 1.2km
  9  | 125      |              ~2              |       3km - 6km      | 500m - 2km
  10 | 125      |              ~1              |       5km - 8km      | 700m - 3km
  11 | 62.5     |              ~0.5            |       7km - 12km     | 1km - 4km
  12 | 31.25    |              ~0.25           |      10km - 15+ km   | 2km - 5km
  *************************** Usar para configurar parâmetros LoRa *************************/

  LoRa.setSPI(lora_spi);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if(!LoRa.begin(915e6)){ ERROR_LOG("Erro: LoRa não iniciado"); }
  LoRa.setSyncWord(0xFE);
  LoRa.setSpreadingFactor(11);
  LoRa.setSignalBandwidth(62.5e3);

  qh_dadosAltitude = xQueueCreate(1024, sizeof(dadosTelemetria));

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

/*** Declaração de tasks ***/

void t_captacaoDados(void *pvParameters){
  dadosTelemetria dados = {};
  BaseType_t xDadosFilaEnviados = pdFALSE;

  while(1){
    dados.altitude = bmp.readAltitude(1013.25) - altIni;
    
    dados.newGpsData = false;

    while(gpsSerial.available()){
      Serial.println("porta recebeu");
      if(gps.encode(gpsSerial.read())){
        Serial.println("String válida lida");
        if(gps.location.isValid()){
          Serial.println("location valid");
          dados.latitude = gps.location.lat();
          dados.longitude = gps.location.lng();
          dados.newGpsData = true;
        }
      }
    }

    xDadosFilaEnviados = xQueueSend(qh_dadosAltitude, &dados, portMAX_DELAY);

    if(xDadosFilaEnviados == pdFALSE) pacotesPerdidos++;

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void t_transmissaoDados(void *pvParameters){
  uint8_t fLoraDisponivel = 0;
  dadosTelemetria dados;
  BaseType_t xDadosFilaRecebidos = pdFALSE;
  String payloadTelemetria = "";
  File arquivo = File();

  while(1){
    xDadosFilaRecebidos = xQueueReceive(qh_dadosAltitude, &dados, portMAX_DELAY);
    Serial.println(dados.altitude);
    Serial.println(dados.longitude);
    Serial.println(dados.latitude);

    if(xDadosFilaRecebidos == pdTRUE){
      payloadTelemetria = String(dados.altitude) + ";" 
                          + String(dados.latitude) + ";"
                          + String(dados.longitude) + ";"
                          + String(pacotesPerdidos);
    arquivo = SD_MMC.open("/flight.txt", FILE_APPEND);

      if(arquivo){
        arquivo.println(payloadTelemetria);
        arquivo.close();
      }else{
        Serial.println("Erro: arquivo não aberto");
      }

      fLoraDisponivel = LoRa.beginPacket();
      
      if(fLoraDisponivel == 1){
        LoRa.println(payloadTelemetria);
        LoRa.endPacket();
      } else{
        Serial.println("LoRa indisponível");
      }
    }
  }
}