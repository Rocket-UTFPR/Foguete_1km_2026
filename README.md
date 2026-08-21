# Foguete_1km_2026
Repositório com os códigos e placas da aviônica do foguete de 1km para a LASC de 2026.

# 1. Especificações de requisitos
Segue abaixo especificações dos requisitos

## 1.1 Requisitos de Hardware

Requisitos Não-Funcionais

|Requisito |Descrição |Prioridade |
|----------|----------|-----------|
|RNFH01 |Deve-se usar o microcontrolador ESP32                                                                    |Alta |
|RNFH02 |Todos os componentes não passivos da placa devem ser SMD                                                 |Média|
|RNFH03 |O sistema deve operar no mínimo 2h sem carga                                                             |Média|
|RNFH04 |O alcance da comunicação LoRa deve ser de, no mímino,5km em campo aberto                                 |Alta |
|RNFH05 |O conversor CC-CC não isolado deve ser buck ou Ćuk, projetado pela própria equipe                        |Baixa|
|RNFH06 |A placa deve contar com plano de terra                                                                   |Alta |
|RNFH07 |Deve haver capacitores cerâmicos de desacoplamento de 100nF nos pinos de alimentação de todos os chips   |Alta |
|RNFH08 |Deve haver filtro de debounce nos botões com freqüência de corte de 30kHz                                |Média|
|RNFH09 |Deve haver filtro passa-baixa RC com freqüência de corte entre 20kHz e 100kHz nos barramentos UART       |Baixa|


Requisitos Funcionais

|Requsito |Descrição |Prioridade |
|---------|----------|-----------|
|RFH01 | O microcontrolador deve transmitir dados de GPS para base via LoRa                                |Alta  |
|RFH02 | O microcontrolador deve gravar dados de altitude no cartão SD                                     |Média |
|RFH03 | A placa de circuito impresso deve ter botões de reset e boot                                      |Média |
|RFH04 | A bateria deve ser conectada à PCB por um RBF (Remove Before Flight)                              |Alta  |
|RFH05 | O microcontrolador deve medir dados de altitude por barômetro e captar localização por GPS        |Alta  |
|RFH06 | Um conversor CC-CC não isolado deve interfacear a bateria com o sistema, tensão de saída de 5V    |Alta  |

# 1.2 Requisitos de Software

Requisitos Não-Funcionais

|Requisito |Descrição |Prioridade |
|----------|----------|-----------|
|RNFS01 |O código deve ser implementado em C++ com o framework Arduino                                            |Alta |
|RNFS02 |A implementação deve fazer uso do sistema operacional FreeRTOS                                           |Alta |
|RNFS03 |Deve haver uma task para captação de dados e acionamento de paraquedas e uma para gravação e transmissão |Alta |

Requisitos Funcionais

|Requisito |Descrição |Prioridade |
|----------|----------|-----------|
|RFS01 | O microcontrolador deve acionar paraquedas após apogeu                                                  |Alta  |
|RFS02 | Deve ser possível gravar código OTA, com página web estilizada                                          |Média |
|RFS03 | Deve haver um wifi-manager que permita ao usuario enviar credenciais de wifi em tempo de execução       |Média |

# 2. Medidas da Placa
- Diâmetro (máximo): 98mm
- Comprimento (máximo): 230mm

# 3. Bugs conhecidos

- No esquemático, a trilha VBUS do USB está conectada à trilha 3V3. A alimentação padrão do USB é de 5V, esse curto-circuito queimaria os componentes da placa;
* Correção: trilhas de VBUS raspadas na versão de voo, jumpeadas para entrada do buck

- No .brd, o sinal diferencial do USB não está roteado corretamente.
* Correção: trilhas jumpeadas com par trançado
