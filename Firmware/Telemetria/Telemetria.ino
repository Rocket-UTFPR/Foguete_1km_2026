TaskHandle_t th_captacaoDados = NULL, //Prefixo th_ para handles de tasks
             th_transmissaoDados = NULL;
QueueHandle_t qh_dadosAltitude = NULL; //Prefixo qh_ para handles de queues
//Todo: queue do GPS depois de adicionar o módulo

void t_captacaoDados(void);
void t_transmissaoDados(void);

void setup() {
  Serial.begin(115200);

  qh_dadosAltitude = xQueueCreate(1024, sizeof(double));

  xTaskCreatePinnedToCore(
    t_captacaoDados,
    "Captacao de dados",
    1000,
    NULL,
    1,
    &th_captacaoDados,
    0
  );

  xTaskCreatePinnedToCore(
    t_transmissaoDados,
    "Transmissao de dados",
    6000,
    NULL,
    1,
    &th_transmissaoDados,
    1
  );
}

void loop() {

}

void t_captacaoDados(void){ //Prefixo t_ para tasks

}

void t_transmissaoDados(void){

}