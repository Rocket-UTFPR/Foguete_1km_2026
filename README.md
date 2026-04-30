# Foguete_1km_2026
Repositório com os códigos e placas da aviônica do foguete de 1km para a LASC de 2026.

# 1. Especificações de requisitos
Segue abaixo especificações dos requisitos

## 1.1 Telemetria

          Requisitos Não-Funcionais
|Requisito |Descrição |Prioridade |
|----------|----------|-----------|
|RF01 |O código deve ser implementado em C++ com o framework Arduino                                            |Alta |
|RF02 |Deve-se usar o microcontrolador ESP32                                                                    |Alta |
|RF03 |A implementação deve fazer uso do sistema operacional FreeRTOS                                           |Alta |
|RF04 |Deve haver uma task para captação de dados e acionamento de paraquedas e uma para gravação e transmissão |Alta |
|RF05 |Todos os componentes não passivos da placa devem ser SMD                                                 |Média|
