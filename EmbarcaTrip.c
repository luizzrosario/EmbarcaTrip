#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "include/ssd1306.h"
#include "ws2812.pio.h"

// Definição dos pinos dos sensores e botões
#define SENSOR_X 27        // Pino do sensor no eixo X
#define SENSOR_Y 26        // Pino do sensor no eixo Y
#define BUTTON_JOYSTICK 22 // Pino do botão do joystick

// Definição dos pinos do buzzer
#define BUZZER_PIN_A 10 // Pino do buzzer A
#define BUZZER_PIN_B 21 // Pino do buzzer B

// Definição dos pinos dos botões
#define BUTTON_A_PIN 5 // Pino do botão A
#define BUTTON_B_PIN 6 // Pino do botão B

// Definição dos pinos dos LEDs
#define LED_R_PIN 13 // Pino do LED vermelho
#define LED_G_PIN 11 // Pino do LED verde
#define LED_B_PIN 12 // Pino do LED azul

// Definição dos pinos e endereço do display OLED
#define ENDERECO_OLED 0x3C // Endereço I2C do display OLED
#define I2C_PORT i2c1      // Porta I2C utilizada
#define I2C_SDA 14         // Pino SDA do I2C
#define I2C_SCL 15         // Pino SCL do I2C

// Definição das dimensões do display OLED
#define WIDTH 128 // Largura do display
#define HEIGHT 64 // Altura do display

// Limiares para detecção de movimento do joystick
#define LIMIAR_MIN 1840 // Valor mínimo para considerar movimento
#define LIMIAR_MAX 2200 // Valor máximo para considerar movimento

// Definição dos limites de movimento do quadrado no display
#define MOVEMENT_X_MIN 0
#define MOVEMENT_X_MAX 120
#define MOVEMENT_Y_MIN 8
#define MOVEMENT_Y_MAX 44

// Definição de tempos
#define TEMPO_OCIOSIDADE_MS 10000 // Tempo para considerar o motor ocioso (10 segundos)
#define TEMPO_DEBOUNCE_MS 200     // Tempo de debounce para os botões (200 ms)

#define WS2812_PIN 7
#define MAX_LEDS 25

// Variáveis para controle dos LEDs WS2812
static int sm = 0;  // Máquina de estado do PIO
static PIO pio = pio0;  // Bloco PIO
static uint32_t grb[MAX_LEDS];  // Array para armazenar as cores dos LEDs

// Estrutura para o display SSD1306
ssd1306_t ssd;

// Enumeração para os estados do motor
typedef enum
{
    MOTOR_DESLIGADO,        // Motor desligado
    MOTOR_LIGADO_MOVIMENTO, // Motor ligado e em movimento
    MOTOR_LIGADO_PARADO,    // Motor ligado, mas parado
    MOTOR_LIGADO_OCIOSO     // Motor ligado e ocioso (parado por muito tempo)
} estado_motor_t;

// Variáveis globais
volatile estado_motor_t estado_motor = MOTOR_DESLIGADO; // Estado atual do motor
volatile uint32_t last_interrupt_time = 0;              // Último tempo de interrupção
absolute_time_t tempo_inicio_parado;                    // Tempo em que o motor ficou parado
uint32_t gasto_tempo = 0;                               // Tempo de movimento do motor
uint32_t viagem = 0;                                    // Contador de viagens
uint32_t tempo_ocioso = 0;                              // Tempo ocioso do motor

// Função para atualizar os LEDs com base no estado do motor
void atualizar_leds()
{
    gpio_put(LED_R_PIN, estado_motor == MOTOR_LIGADO_PARADO || estado_motor == MOTOR_LIGADO_OCIOSO);
    gpio_put(LED_G_PIN, estado_motor == MOTOR_LIGADO_MOVIMENTO || estado_motor == MOTOR_LIGADO_PARADO);
    gpio_put(LED_B_PIN, 0); // LED azul sempre desligado
}

// Função para tocar uma nota no buzzer
void tocar_nota(uint pino_Buzzer, int frequencia, int duracao)
{
    if (frequencia > 0)
    {
        int slice_num = pwm_gpio_to_slice_num(pino_Buzzer);
        uint32_t freq_sistema = clock_get_hz(clk_sys);
        uint16_t wrap_valor = freq_sistema / frequencia - 1;

        pwm_set_wrap(slice_num, wrap_valor);
        pwm_set_gpio_level(pino_Buzzer, wrap_valor / 2); // Duty cycle de 50%
        pwm_set_enabled(slice_num, true);                // Ativa o PWM

        sleep_ms(duracao);                 // Mantém a nota pelo tempo especificado
        pwm_set_enabled(slice_num, false); // Desativa o PWM
    }
    else
    {
        sleep_ms(duracao); // Pausa (nota silenciosa)
    }
}

// Desenha e move um quadrado no display OLED
void move_square(ssd1306_t *ssd, int x, int y)
{
    static int prev_x = WIDTH / 2 - 4;  // Posição inicial X do quadrado
    static int prev_y = HEIGHT / 2 - 4; // Posição inicial Y do quadrado

    // Apaga o quadrado anterior
    ssd1306_rect(ssd, prev_y, prev_x, 8, 8, false, true);

    // Atualiza a posição do quadrado
    prev_x = x;
    prev_y = y;

    // Desenha o quadrado na nova posição
    ssd1306_rect(ssd, prev_y, prev_x, 8, 8, true, true);
    ssd1306_send_data(ssd);
}

// Função para exibir o estado do motor no display OLED
void exibir_estado_no_display()
{
    ssd1306_fill(&ssd, false); // Limpa o display

    char texto[20];
    // Exibe o ID da viagem
    sprintf(texto, "V:%d", viagem);
    ssd1306_draw_string(&ssd, texto, 95, 0);
    // Exibe o tempo ocioso
    sprintf(texto, "Ocioso:%dm", tempo_ocioso / 1000);
    ssd1306_draw_string(&ssd, texto, 0, 55);
    // Exibe o consumo de combustível
    sprintf(texto, "%dL", gasto_tempo / 1000);
    ssd1306_draw_string(&ssd, texto, 95, 55);

    // Exibe o estado atual do motor
    switch (estado_motor)
    {
    case MOTOR_LIGADO_MOVIMENTO:
        ssd1306_draw_string(&ssd, "Movimento", 0, 0);
        break;
    case MOTOR_LIGADO_PARADO:
        ssd1306_draw_string(&ssd, "Parado", 0, 0);
        break;
    case MOTOR_LIGADO_OCIOSO:
        ssd1306_draw_string(&ssd, "Ocioso", 0, 0);
        ssd1306_draw_string(&ssd, "DESLIGUE", 0, 10);
        break;
    case MOTOR_DESLIGADO:
    default:
        ssd1306_draw_string(&ssd, "Desligado", 0, 0);
        break;
    }

    ssd1306_send_data(&ssd); // Envia os dados para o display
}

// Função para converter porcentagens de cores RGB para o formato GRB
static void rgb_to_grb(uint32_t porcentColors[][3]) {
    for (int i = 0; i < MAX_LEDS; i++) {
        uint8_t r = porcentColors[i][0] ? 255 * (porcentColors[i][0] / 100.0) : 0;
        uint8_t g = porcentColors[i][1] ? 255 * (porcentColors[i][1] / 100.0) : 0;
        uint8_t b = porcentColors[i][2] ? 255 * (porcentColors[i][2] / 100.0) : 0;
        grb[i] = (g << 16) | (r << 8) | b;
    }
}

// Função para inicializar a matriz de LEDs WS2812
void init_ws2812() {
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
    for (int i = 0; i < MAX_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, 0);
    }
}

// Função para exibir um número ou símbolo especial na matriz de LEDs WS2812
void display_number(int number) {
    // Matrizes de 5x5 para representar símbolos
    const uint32_t numbers[4][25] = {
        // 0 - "!" vermelho
        {0, 0, 1, 0, 0,
         0, 0, 0, 0, 0,
         0, 0, 1, 0, 0,
         0, 0, 1, 0, 0,
         0, 0, 1, 0, 0},
        // 1 - "+" verde
        {0, 0, 1, 0, 0,
         0, 0, 1, 0, 0,
         1, 1, 1, 1, 1,
         0, 0, 1, 0, 0,
         0, 0, 1, 0, 0},
        // 2 - "-" amarelo
        {0, 0, 0, 0, 0,
         0, 0, 0, 0, 0,
         1, 1, 1, 1, 1,
         0, 0, 0, 0, 0,
         0, 0, 0, 0, 0},
        // 3 - "-" branco
        {0, 0, 0, 0, 0,
         0, 0, 0, 0, 0,
         0, 0, 1, 0, 0,
         0, 0, 0, 0, 0,
         0, 0, 0, 0, 0}
    };

    // Cores para cada símbolo
    const uint32_t colors[4][3] = {
        {100, 0, 0},   // "!" Vermelho
        {0, 100, 0},   // "+" Verde
        {100, 100, 0}, // "-" Amarelo 
        {100, 100, 100}// Branco
    };

    uint32_t porcentColors[MAX_LEDS][3] = {0};
    for (int i = 0; i < MAX_LEDS; i++) {
        if (numbers[number][i]) {
            porcentColors[i][0] = colors[number][0]; // Vermelho
            porcentColors[i][1] = colors[number][1]; // Verde
            porcentColors[i][2] = colors[number][2]; // Azul
        }
    }

    rgb_to_grb(porcentColors);
    for (int i = 0; i < MAX_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, grb[i] << 8u);
    }
    sleep_us(10);
}

void button_isr(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Verifica o debounce
    if (current_time - last_interrupt_time > TEMPO_DEBOUNCE_MS)
    {
        last_interrupt_time = current_time;

        if (gpio == BUTTON_A_PIN)
        {
            // Alterna entre ligar e desligar o motor
            if (estado_motor == MOTOR_DESLIGADO) { 
                estado_motor = MOTOR_LIGADO_PARADO;
                display_number(2); // Exibe o símbolo "-" amarelo
            } else {
                estado_motor = MOTOR_DESLIGADO;
                display_number(3); // Exibe o símbolo "-" branco
            }
            tempo_inicio_parado = get_absolute_time(); // Reinicia o contador de tempo parado
            atualizar_leds();                          // Atualiza os LEDs
        }
        else if (gpio == BUTTON_B_PIN)
        {
            // Incrementa o contador de viagens e zera o tempo de movimento
            printf("Viagem %d: Consumo = %d L Tempo ocioso = %d\n", viagem, gasto_tempo / 1000, tempo_ocioso); // Envia dados para o PC
            viagem++;
            gasto_tempo = 0;  // Zera o tempo de movimento
            tempo_ocioso = 0; // Zera o tempo ocioso
        }
    }
}

// Função para inicializar o hardware
void init_hardware()
{
    stdio_init_all(); // Inicializa a biblioteca stdio

    // Configuração dos botões
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_RISE, true, button_isr);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_RISE, true, button_isr);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_RISE, true, button_isr);

    // Configuração dos pinos do buzzer
    gpio_init(BUZZER_PIN_A);
    gpio_set_function(BUZZER_PIN_A, GPIO_FUNC_PWM);
    gpio_init(BUZZER_PIN_B);
    gpio_set_function(BUZZER_PIN_B, GPIO_FUNC_PWM);

    // Configuração dos LEDs
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    atualizar_leds(); // Atualiza os LEDs com o estado inicial

    // Configuração do display OLED
    i2c_init(I2C_PORT, 400 * 1000); // Inicializa o I2C com 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_OLED, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display inicialmente
    ssd1306_send_data(&ssd);   // Envia os dados para o display

    // Configuração do ADC para o joystick
    adc_init();
    adc_gpio_init(SENSOR_X);
    adc_gpio_init(SENSOR_Y);

    // Configuração do PIO para os LEDs WS2812
    init_ws2812();
}

// Função principal
int main()
{
    init_hardware(); // Inicializa o hardware
    sleep_ms(10); // Aguarda 100 ms para estabilização
    display_number(3); // Exibe o símbolo "-" branco inicialmente

    while (1)
    {
        // Lê os valores do joystick
        adc_select_input(0);
        uint16_t leitura_y = adc_read();
        adc_select_input(1);
        uint16_t leitura_x = adc_read();

        // Calcula as posições para o movimento do quadrado
        int square_x = MOVEMENT_X_MIN + (leitura_x * (MOVEMENT_X_MAX - MOVEMENT_X_MIN)) / 4095;
        int square_y = MOVEMENT_Y_MAX - (leitura_y * (MOVEMENT_Y_MAX - MOVEMENT_Y_MIN)) / 4095;


        // Move o quadrado no display
        move_square(&ssd, square_x, square_y); 
        if (estado_motor != MOTOR_DESLIGADO)
        {
            // Verifica se o joystick está dentro dos limites (parado)
            bool dentro_limite_x = (leitura_x > LIMIAR_MIN && leitura_x < LIMIAR_MAX);
            bool dentro_limite_y = (leitura_y > LIMIAR_MIN && leitura_y < LIMIAR_MAX);

            if (dentro_limite_x && dentro_limite_y)
            {
                // Motor está parado
                if (estado_motor == MOTOR_LIGADO_MOVIMENTO)
                {
                    estado_motor = MOTOR_LIGADO_PARADO;
                    tempo_inicio_parado = get_absolute_time(); // Inicia o contador de tempo parado
                    display_number(2); // Exibe o símbolo "-" amarelo
                }
                else if (estado_motor == MOTOR_LIGADO_PARADO || estado_motor == MOTOR_LIGADO_OCIOSO)
                {
                    // Verifica se o motor está parado há mais de 10 segundos
                    if (absolute_time_diff_us(tempo_inicio_parado, get_absolute_time()) >= TEMPO_OCIOSIDADE_MS * 1000)
                    {
                        estado_motor = MOTOR_LIGADO_OCIOSO;
                        display_number(0); // Exibe o símbolo "!" vermelho
                        tocar_nota(BUZZER_PIN_A, 400, 100); // Toca um alerta sonoro
                        tocar_nota(BUZZER_PIN_B, 250, 100);
                        tempo_ocioso += 300; // Incrementa o tempo ocioso
                    }
                }
            }
            else
            {
                // Motor está em movimento
                display_number(1); // Exibe o símbolo "+" verde
                estado_motor = MOTOR_LIGADO_MOVIMENTO;
                gasto_tempo += 10; // Incrementa o tempo de movimento
            }
        }

        // Atualiza os LEDs e o display
        atualizar_leds();
        exibir_estado_no_display();

        sleep_ms(10); // Aguarda 10 ms antes de repetir 
    }
}