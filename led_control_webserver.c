/**
 * AULA IoT - Embarcatech - Ricardo Prates - 004 - Webserver Raspberry Pi Pico w - wlan
 *
 * Material de suporte
 * 
 * https://www.raspberrypi.com/documentation/pico-sdk/networking.html#group_pico_cyw43_arch_1ga33cca1c95fc0d7512e7fef4a59fd7475 
 */

#include <stdio.h>               // Biblioteca padrão para entrada e saída
#include <string.h>              // Biblioteca manipular strings
#include <stdlib.h>              // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)

#include "pico/stdlib.h"         // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/adc.h"        // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)


#include "inc/ssd1306.h"         // Display

#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"


// Configurando I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

// Configurando Display
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SQUARE_SIZE 8

volatile bool led_r_estado = false;
volatile bool led_g_estado = false;
volatile bool led_b_estado = false;
bool cor = true;
absolute_time_t last_interrupt_time = 0;
float rpm = 0;

uint8_t ssd[ssd1306_buffer_length];

struct render_area frame_area = {
            .start_column = 0,
            .end_column = ssd1306_width - 1,
            .start_page = 0,
            .end_page = ssd1306_n_pages - 1
        };

// Flag
volatile char c = '~';
volatile bool new_data = false;
volatile int current_digit = 0;

volatile int flag = 0;

// Protótipos das funções
void npDisplayDigit(int digit);

// Função auxiliar para processar o comando e atualizar os displays
void process_command(char c, int digit, char *line1, char *line2, uint8_t *ssd, struct render_area *frame_area) {
    // Atualiza o OLED
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, frame_area);
    ssd1306_draw_string(ssd, 5, 0, line1);
    ssd1306_draw_string(ssd, 5, 8, line2);
    render_on_display(ssd, frame_area);
}


// Credenciais WIFI - Tome cuidado se publicar no github!
const char WIFI_SSID[] = "TIM_ULTRAFIBRA_28A0";
const char WIFI_PASSWORD[] = "64t4fu76eb";


// Credenciais WIFI - Tome cuidado se publicar no github!
// const char WIFI_SSID[] = "teste";
// const char WIFI_PASSWORD[] = "6>2Tw824";

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Leitura da temperatura interna
float temp_read(void);

// Tratamento do request do usuário
void user_request(char **request);
void update_menu(uint8_t *ssd, struct render_area *frame_area);


void setup_pwm(uint pin, uint16_t level) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 255);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), level);
    pwm_set_enabled(slice, true);
}


// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
    gpio_led_bitdog();
    
    

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);


        // Configura a área de renderização do display OLED
        ssd1306_init();

        struct render_area frame_area = {
            .start_column = 0,
            .end_column = ssd1306_width - 1,
            .start_page = 0,
            .end_page = ssd1306_n_pages - 1
        };
        calculate_render_area_buffer_length(&frame_area);
    
        // zera o display inteiro
        uint8_t ssd[ssd1306_buffer_length];
        memset(ssd, 0, ssd1306_buffer_length);
        render_on_display(ssd, &frame_area);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    while (true)
    {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */

        // Atualiza o menu do display
        update_menu(ssd, &frame_area);
        cor = !cor;

        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);      // Reduz o uso da CPU


    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------


void update_menu(uint8_t *ssd, struct render_area *frame_area) {
    memset(ssd, 0, ssd1306_buffer_length);

    if(flag == 1){
        ssd1306_draw_string(ssd, 34, 20, "PULSEIRA");
        ssd1306_draw_string(ssd, 40, 36, "LARANJA");
        render_on_display(ssd, frame_area);
    }else if(flag == 2){
        ssd1306_draw_string(ssd, 34, 20, "PULSEIRA");
        ssd1306_draw_string(ssd, 34, 36, "VERMELHA");
        render_on_display(ssd, frame_area);
    }else if(flag == 3){
        ssd1306_draw_string(ssd, 34, 20, "PULSEIRA");
        ssd1306_draw_string(ssd, 46, 36, "VERDE");
        render_on_display(ssd, frame_area);
    }else if(flag == 4){
        ssd1306_draw_string(ssd, 34, 20, "PULSEIRA");
        ssd1306_draw_string(ssd, 49, 36, "AZUL");
        render_on_display(ssd, frame_area);
    }

}

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void){
    // Configuração dos LEDs como saída
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);
    
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);
    
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário - digite aqui
void user_request(char **request){
    memset(ssd, 0, ssd1306_buffer_length);
    if (strstr(*request, "GET /muitoUrgente") != NULL)
    {

        gpio_put(LED_RED_PIN, 1);
        setup_pwm(LED_GREEN_PIN, 40);
        gpio_put(LED_BLUE_PIN, 0);
        flag = 1;
    }
    if (strstr(*request, "GET /Urgente") != NULL)
    {       
            gpio_put(LED_RED_PIN, 1);
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_BLUE_PIN, 0);
            setup_pwm(LED_GREEN_PIN, 0);
            flag = 2;
    }

    if (strstr(*request, "GET /poucoUrgente") != NULL)
    {       
            gpio_put(LED_RED_PIN, 0);
            gpio_put(LED_BLUE_PIN, 0);
            setup_pwm(LED_GREEN_PIN, 100);
            flag = 3;
    }
    if (strstr(*request, "GET /naoUrgente") != NULL)
    {       
            gpio_put(LED_RED_PIN, 0);
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_BLUE_PIN, 1);
            setup_pwm(LED_GREEN_PIN, 0);
            flag = 4;
    }
};

// Leitura da temperatura interna
float temp_read(void){
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
        return temperature;
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    
    // Leitura da temperatura interna
    float temperature = temp_read();

    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html), // Formatar uma string e armazená-la em um buffer de caracteres
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<meta charset='UTF-8'>"
             "<title> Classificar Pacientes </title>\n"
             "<style>\n"
             "body { background-color: #b5e5fb; font-family: Arial, sans-serif; text-align: left; margin-top: 50px; }\n"
             "h1 { font-size: 50px; margin-bottom: 30px; }\n"
             "button { background-color:rgb(255, 255, 255); font-size: 20px; margin: 5px; padding: 10px 25px;}\n"
             "button:hover { background-color: #d1d5db; }\n"
             
             ".temperature { font-size: 48px; margin-top: 30px; color: #333; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Classificar Pacientes</h1>\n"

            
             "<form action=\"./muitoUrgente\"><button>Dor forte ou intensa / Falta de ar</button></form>\n"
             "<form action=\"./Urgente\"><button>Sangramento ativo</button></form>\n"
             "<form action=\"./Urgente\"><button>Febre acima de 39°C</button></form>\n"
             "<form action=\"./poucoUrgente\"><button>Sintomas leves (como dor de cabeça leve)</button></form>\n"
             "<form action=\"./naoUrgente\"><button>Em observação ou para renovação de receita</button></form>\n"
             "</body>\n"
             "</html>\n");

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}

