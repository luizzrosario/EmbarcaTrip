# EmbarcaTrip 🚗📈

**Telemetria de veículos** com registro de viagens, otimizando o uso do motor e proporcionando economia de combustível através de um dashboard de informações em tempo real.

## 📋 Objetivo Geral

Desenvolver um sistema de simulação de telemetria de veículos, monitorando o estado do motor, alertando o motorista em caso de ociosidade, e registrando dados da viagem para posterior análise.

## 🎯 Funcionalidades

- Controle de movimento via **Joystick**, simulando o deslocamento de um veículo em uma matriz 8x8.
- Monitoramento de **estado do motor** com feedback visual e sonoro:
  - **Motor Desligado** (Branco)
  - **Motor Ligado Parado** (Amarelo)
  - **Motor Ligado em Movimento** (Verde)
  - **Motor Ocioso** (Vermelho)
- Contabilização do tempo gasto em cada estado durante a viagem.
- Alerta sonoro e visual caso o motor fique ocioso por mais de **15 segundos**.
- Registro da viagem com **ID**, **consumo estimado** e **tempo ocioso** enviado via **UART**.
- Interface gráfica em **Display OLED** e **Matriz de LEDs**.

## 🛠️ Periféricos Utilizados (BitDogLab)

- **Joystick**: Movimento do quadrado (simulação de veículo).
- **Botão A**: Ligar/Desligar motor (com tratamento de debounce).
- **Botão B**: Enviar o log da viagem via UART (com tratamento de debounce).
- **Display OLED**: Dashboard informativo (posição do joystick e estados do motor).
- **Matriz de LEDs**: Indicação visual do estado atual do motor.
- **LED RGB**: Sinalização de motor ligado em diferentes estados.
- **Buzzer**: Alerta sonoro de motor ocioso.

## 🚀 Como Funciona

1. Utilize o **Joystick** para simular o movimento do veículo no display.
2. Pressione **Botão A** para ligar/desligar o motor.
3. O sistema automaticamente identifica o estado do motor conforme o movimento do joystick.
4. Se o motor ficar parado por mais de **15 segundos**, será emitido um alerta.
5. Ao final da viagem, pressione **Botão B** para enviar o **log** via UART, contendo:
   - ID da viagem
   - Consumo de combustível (estimado)
   - Tempo de motor ocioso

## 🔗 Links 

- [Repositório do Projeto no GitHub](https://github.com/luizzrosario/EmbarcaTrip)
- [Demonstração em Vídeo](https://www.youtube.com/watch?v=i7c6Dc3rWO8)

## 👨‍💻 Autores

- **Luiz Rosário** - Desenvolvimento do Projeto
