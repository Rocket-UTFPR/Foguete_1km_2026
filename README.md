# Foguete_1km_2026
Repositório com os códigos e placas da aviônica do foguete de 1km para a LASC de 2026.

# 1. Especificações de requisitos
Segue abaixo especificações dos requisitos

## 1.1 Telemetria

Requisitos Não-Funcionais

|Requisito |Descrição |Prioridade |
|----------|----------|-----------|
|RNF01 |O código deve ser implementado em C++ com o framework Arduino                                            |Alta |
|RNF02 |Deve-se usar o microcontrolador ESP32                                                                    |Alta |
|RNF03 |A implementação deve fazer uso do sistema operacional FreeRTOS                                           |Alta |
|RNF04 |Deve haver uma task para captação de dados e acionamento de paraquedas e uma para gravação e transmissão |Alta |
|RNF05 |Todos os componentes não passivos da placa devem ser SMD                                                 |Média|
|RNF06 |O sistema deve operar no mínimo 2h sem carga                                                             |Média|
|RNF07 |O alcance da comunicação LoRa deve ser de, no mímino,5km em campo aberto                                 |Alta |
|RNF08 |O conversor CC-CC não isolado deve ser buck ou Ćuk, projetado pela própria equipe                        |Baixa|

Requisitos Funcionais

|Requsito |Descrição |Prioridade |
|---------|----------|-----------|
|RF01 | O microcontrolador deve acionar paraquedas após apogeu                                            |Alta  |
|RF02 | O microcontrolador deve transmitir dados de GPS para base via LoRa                                |Alta  |
|RF03 | O microcontrolador deve gravar dados de altitude no cartão SD                                     |Média |
|RF04 | Deve ser possível gravar código OTA, com página web estilizada                                    |Média |
|RF05 | Deve haver um wifi-manager que permita ao usuario enviar credenciais de wifi em tempo de execução |Média |
|RF06 | A placa de circuito impresso deve ter botões de reset e boot                                      |Média |
|RF07 | A bateria deve ser conectada à PCB por um RBF (Remove Before Flight)                              |Alta  |
|RF08 | O microcontrolador deve medir dados de altitude por barômetro e captar localização por GPS        |Alta  |
|RF09 | Um conversor CC-CC não isolado deve interfacear a bateria com o sistema, tensão de saída de 5V    |Alta  |
