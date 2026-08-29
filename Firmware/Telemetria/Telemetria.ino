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
#include <SD_MMC.h>

#define LORA_SS 5 
#define LORA_RST 17
#define LORA_DIO0 16
#define GPS_RX 33 // RX do ESP, TX do GPS
#define GPS_TX 32 // TX do ESP, RX da GPS
#define PIN_PARAQUEDAS 27


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
volatile float altAtual = 0,
               altAnterior = 0;

enum class EtapasVoo{
  SOLO,
  VOO,
  QUEDA,
  PARAQUEDAS
};
volatile EtapasVoo etapaAtual = EtapasVoo::SOLO;

struct dadosTelemetria{
  float altitude;
  //double latitude,
  //       longitude;
  //bool newGpsData;
  unsigned long uptime;
};

TaskHandle_t th_captacaoDados = NULL,
             th_transmissaoDados = NULL,
             th_ejecao = NULL;
QueueHandle_t qh_dadosAltitude = NULL;

void t_captacaoDados(void *pvParameters);
void t_transmissaoDados(void *pvParameters);
void t_ejecao(void *pvParameters);

void setup() {
  File arquivo = File();

  Serial.begin(115200);

  pinMode(PIN_PARAQUEDAS, OUTPUT);
  digitalWrite(PIN_PARAQUEDAS, HIGH);

  //gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

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
  arquivo.close();



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

  qh_dadosAltitude = xQueueCreate(2048, sizeof(dadosTelemetria));

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

  if(xTaskCreatePinnedToCore(
    t_ejecao,
    "Ejeção do paraquedas",
    2000,
    NULL,
    2,
    &th_ejecao,
    tskNO_AFFINITY
  ) == pdFALSE){ 
    ERROR_LOG("Erro: task \'Ejeção do paraquedas\' não iniciada"); 
  }
}

void loop() {
  vTaskDelete(NULL);
}

/*** Declaração de tasks ***/

void t_captacaoDados(void *pvParameters){
  struct dadosTelemetria dados = {};
  BaseType_t xDadosFilaEnviados = pdFALSE;
  String payloadTelemetria = ""; 
  File arquivo = SD_MMC.open("/flight.txt", FILE_APPEND);
  

  while(1){
    dados.uptime = millis();
    dados.altitude = bmp.readAltitude(1013.25) - altIni;
    //dados.newGpsData = false;
    altAnterior = altAtual;
    altAtual = dados.altitude;

    payloadTelemetria = String(dados.altitude) + ";" 
                      //+ String(dados.latitude, 6) + ";"
                      //+ String(dados.longitude, 6) + ";"
                      //+ String(pacotesPerdidos) + ";"
                      //+ String(dados.newGpsData) + ";"
                      + String(dados.uptime) +
                      ("\0");
    
    if(arquivo){
      arquivo.println(payloadTelemetria);
      arquivo.flush();
      Serial.println("Gravou no SD..");

      if ((etapaAtual == EtapasVoo::SOLO) && (digitalRead(PIN_PARAQUEDAS) == LOW)) {
        arquivo.close();
      }
    } else {
      Serial.println("Erro: arquivo não aberto no SD");
    }
    
    /*
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
    }*/

    xDadosFilaEnviados = xQueueSend(qh_dadosAltitude, &dados, pdMS_TO_TICKS(5));

    if(xDadosFilaEnviados == pdFALSE) pacotesPerdidos++;

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void t_transmissaoDados(void *pvParameters){
  uint8_t fLoraDisponivel = 0;
  struct dadosTelemetria dados = {};
  BaseType_t xDadosFilaRecebidos = pdFALSE;
  String payloadTelemetria = "";

  while(1){
    xDadosFilaRecebidos = xQueueReceive(qh_dadosAltitude, &dados, portMAX_DELAY);
    Serial.println(altAtual);
    //Serial.println(dados.longitude);
    //Serial.println(dados.latitude);

    if(xDadosFilaRecebidos == pdTRUE){
      payloadTelemetria = String(dados.altitude) + ";" 
                          //+ String(dados.latitude, 6) + ";"
                          //+ String(dados.longitude, 6) + ";"
                          //+ String(pacotesPerdidos) + ";"
                          //+ String(dados.newGpsData) + ";"
                          + String(dados.uptime) +
                          ("\0");

      fLoraDisponivel = LoRa.beginPacket();
      
      if(fLoraDisponivel == 1){
        LoRa.print(payloadTelemetria);
        LoRa.endPacket();
      } else{
        Serial.println("LoRa indisponível");
      }
    }
  }
}

 // Função de Ejeção do Paraquedas
void t_ejecao (void *pvParameters){

  int contQueda = 0;
  int contRuido = 0;

  while(1){
    switch (etapaAtual) {
    case EtapasVoo::SOLO:
      Serial.println("SOLO");
      if (altAtual > 120) etapaAtual = EtapasVoo::VOO;

      break;

    case EtapasVoo::VOO:
      Serial.println("VOO");
      contQueda = 0;
      contRuido = 0;
      if (altAtual < altAnterior) etapaAtual = EtapasVoo::QUEDA;

      break;

    case EtapasVoo:: QUEDA:
      Serial.println("QUEDA");
      if (altAtual < altAnterior){
        contQueda++;
        Serial.println(contQueda);

        //if(contQueda >= 9) break;
      }
      if (altAtual > altAnterior){
        contRuido++;
        Serial.println(contRuido);
      }

      if (contRuido >= 3) etapaAtual = EtapasVoo::VOO;
      if (contQueda >= 9) etapaAtual = EtapasVoo::PARAQUEDAS;
      break;

    case EtapasVoo::PARAQUEDAS:
      digitalWrite(PIN_PARAQUEDAS, LOW);
      Serial.println("PARAQUEDAS ACIONADO");
      if (altAtual < 120) etapaAtual = EtapasVoo::SOLO;
      break;
    
    default:
      ERROR_LOG("Estado não esperado na ejeção do paraquedas");
      etapaAtual = EtapasVoo::SOLO;
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
