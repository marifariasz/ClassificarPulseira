# 🌐 Webserver com Raspberry Pi Pico W - Classificação de Pacientes

Este projeto cria um servidor web simples embarcado no **Raspberry Pi Pico W**, simulando um sistema de **triagem médica** com LEDs para indicar o nível de urgência do atendimento.

## 🚦 Funcionalidades

- Conexão à rede Wi-Fi.
- Servidor HTTP rodando na porta 80.
- Interface web com botões para classificação de pacientes.
- Acionamento de LEDs coloridos (RGB) com e sem PWM.
- Interface responsiva com HTML e CSS embutido.

## 🔵 Níveis de Classificação e LEDs

| Classificação     | Indicação Visual              |
|-------------------|-------------------------------|
| Muito Urgente     | 🟠 LED vermelho + PWM no verde |
| Urgente           | 🔴 LED vermelho                |
| Pouco Urgente     | 🟢 LED verde         |
| Não Urgente       | 🔵 LED azul                    |

## 🛠️ Requisitos

- Raspberry Pi Pico W
- LEDs conectados aos pinos:
  - Vermelho: GPIO13
  - Verde: GPIO11
  - Azul: GPIO12
- Ambiente com:
  - [Pico SDK](https://github.com/raspberrypi/pico-sdk)
  - lwIP
  - CMake + GCC para ARM

## ⚙️ Como Usar

1. Clone o projeto e abra o código.
2. Edite o trecho abaixo com os dados da sua rede Wi-Fi:

```c
const char WIFI_SSID[] = "SEU_SSID";
const char WIFI_PASSWORD[] = "SUA_SENHA";
```
3. Compile o projeto com CMake.
4. Grave o binário na placa Pico W.
5. Abra o terminal para ver o IP local exibido após a conexão Wi-Fi.
6. Acesse o IP no navegador e interaja com os botões da página.
