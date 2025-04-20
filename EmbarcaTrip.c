#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "include/ssd1306.h"
#include "ws2812.pio.h"
#include "globals.h"

// Função para atualizar os LEDs com base no estado do motor
void atualizar_leds()
{
    gpio_put(LED_R_PIN, estado_motor == MOTOR_LIGADO_PARADO || estado_motor == MOTOR_LIGADO_OCIOSO);
    gpio_put(LED_G_PIN, estado_motor == MOTOR_LIGADO_MOVIMENTO || estado_motor == MOTOR_LIGADO_PARADO);
    gpio_put(LED_B_PIN, 0); // LED azul sempre desligado
}

// Função auxiliar para configurar e iniciar o PWM no buzzer
static void iniciar_pwm_buzzer(uint pino_buzzer, int frequencia)
{
    int slice_num = pwm_gpio_to_slice_num(pino_buzzer);
    uint32_t freq_sistema = clock_get_hz(clk_sys);
    uint16_t wrap_valor = freq_sistema / frequencia - 1;

    pwm_set_wrap(slice_num, wrap_valor);
    pwm_set_gpio_level(pino_buzzer, wrap_valor / 2); // Duty cycle de 50%
    pwm_set_enabled(slice_num, true);
}

// Função auxiliar para parar o PWM do buzzer
static void parar_pwm_buzzer(uint pino_buzzer)
{
    int slice_num = pwm_gpio_to_slice_num(pino_buzzer);
    pwm_set_enabled(slice_num, false);
}

// Função principal para tocar uma nota
void tocar_nota(uint pino_buzzer, int frequencia, int duracao)
{
    if (frequencia > 0)
    {
        iniciar_pwm_buzzer(pino_buzzer, frequencia);
        sleep_ms(duracao); // Mantém a nota
        parar_pwm_buzzer(pino_buzzer);
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

// Função para exibir informações gerais no display
static void exibir_informacoes_gerais()
{
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
}

// Função para exibir o estado atual do motor
static void exibir_estado_motor()
{
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
}

// Função principal para atualizar o display OLED
void exibir_estado_no_display()
{
    ssd1306_fill(&ssd, false); // Limpa o display

    exibir_informacoes_gerais();
    exibir_estado_motor();

    ssd1306_send_data(&ssd); // Envia os dados para o display
}

// Função para configurar a cor de um pixel específico
static void configurar_pixel(uint32_t porcentagem_cores[][3], int index, const uint32_t cor[3])
{
    porcentagem_cores[index][0] = cor[0]; // Vermelho
    porcentagem_cores[index][1] = cor[1]; // Verde
    porcentagem_cores[index][2] = cor[2]; // Azul
}

// Função para montar o buffer de cores com base no símbolo escolhido
static void montar_buffer_simbolo(int numero, uint32_t porcentagem_cores[][3])
{
    const uint32_t simbolos[4][25] = {
        // 0 - "!" vermelho
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
        // 1 - "+" verde
        {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
        // 2 - "-" amarelo
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        // 3 - "-" branco (apenas no centro)
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    const uint32_t cores[4][3] = {
        {100, 0, 0},    // "!" vermelho
        {0, 100, 0},    // "+" verde
        {100, 100, 0},  // "-" amarelo
        {100, 100, 100} // "-" branco
    };

    for (int i = 0; i < MAX_LEDS; i++)
    {
        if (simbolos[numero][i])
        {
            configurar_pixel(porcentagem_cores, i, cores[numero]);
        }
    }
}

// Função para enviar o buffer para a fita de LEDs
static void enviar_para_leds()
{
    for (int i = 0; i < MAX_LEDS; i++)
    {
        pio_sm_put_blocking(pio, sm, grb[i] << 8u);
    }
    sleep_us(10);
}

// Função para converter porcentagens de cores RGB para o formato GRB
static void rgb_to_grb(uint32_t porcentagem_cores[][3])
{
    for (int i = 0; i < MAX_LEDS; i++)
    {
        uint8_t r = porcentagem_cores[i][0] ? 255 * (porcentagem_cores[i][0] / 100.0) : 0;
        uint8_t g = porcentagem_cores[i][1] ? 255 * (porcentagem_cores[i][1] / 100.0) : 0;
        uint8_t b = porcentagem_cores[i][2] ? 255 * (porcentagem_cores[i][2] / 100.0) : 0;
        grb[i] = (g << 16) | (r << 8) | b;
    }
}

// Função principal para exibir um símbolo na matriz WS2812
void estado_motor_matrix(int number)
{
    uint32_t porcentagem_cores[MAX_LEDS][3] = {0};
    montar_buffer_simbolo(number, porcentagem_cores);
    rgb_to_grb(porcentagem_cores);
    enviar_para_leds();
}

// Função para inicializar a matriz de LEDs WS2812
void init_ws2812()
{
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    for (int i = 0; i < MAX_LEDS; i++)
    {
        pio_sm_put_blocking(pio, sm, 0);
    }
}

// Função para lidar com o botão A
static void tratar_botao_a()
{
    if (estado_motor == MOTOR_DESLIGADO)
    {
        estado_motor = MOTOR_LIGADO_PARADO;
        estado_motor_matrix(2); // Exibe o símbolo "-" amarelo
    }
    else
    {
        estado_motor = MOTOR_DESLIGADO;
        estado_motor_matrix(3); // Exibe o símbolo "-" branco
    }
    tempo_inicio_parado = get_absolute_time(); // Reinicia o contador de tempo parado
    atualizar_leds();                          // Atualiza os LEDs
}

// Função para lidar com o botão B
static void tratar_botao_b()
{
    printf("Viagem %d: Consumo = %d L Tempo ocioso = %d\n", viagem, gasto_tempo / 1000, tempo_ocioso); // Envia dados para o PC
    viagem++;
    gasto_tempo = 0;  // Zera o tempo de movimento
    tempo_ocioso = 0; // Zera o tempo ocioso
}

// Função principal da interrupção
void button_isr(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_interrupt_time > TEMPO_DEBOUNCE_MS)
    {
        last_interrupt_time = current_time;

        if (gpio == BUTTON_A_PIN)
        {
            tratar_botao_a();
        }
        else if (gpio == BUTTON_B_PIN)
        {
            tratar_botao_b();
        }
    }
}

void configurar_botoes()
{
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
}

void configurar_buzzer()
{
    gpio_init(BUZZER_PIN_A);
    gpio_set_function(BUZZER_PIN_A, GPIO_FUNC_PWM);

    gpio_init(BUZZER_PIN_B);
    gpio_set_function(BUZZER_PIN_B, GPIO_FUNC_PWM);
}

void configurar_leds_rgb()
{
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);

    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);

    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    atualizar_leds(); // Atualiza os LEDs com o estado inicial
}

void configurar_display()
{
    i2c_init(I2C_PORT, 400 * 1000); // Inicializa o I2C com 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_OLED, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display inicialmente
    ssd1306_send_data(&ssd);   // Envia os dados para o display
}

void configurar_adc()
{
    adc_init();
    adc_gpio_init(SENSOR_X);
    adc_gpio_init(SENSOR_Y);
}

void init_hardware()
{
    stdio_init_all();      // Inicializa a biblioteca stdio
    configurar_botoes();   // Configura botões
    configurar_buzzer();   // Configura buzzers
    configurar_leds_rgb(); // Configura LEDs RGB
    configurar_display();  // Configura o display OLED
    configurar_adc();      // Configura o ADC para o joystick
    init_ws2812();         // Configura PIO para LEDs WS2812
}

void ler_joystick(uint16_t *leitura_x, uint16_t *leitura_y)
{
    adc_select_input(0);
    *leitura_y = adc_read();
    adc_select_input(1);
    *leitura_x = adc_read();
}

void atualizar_posicao_quadrado(uint16_t leitura_x, uint16_t leitura_y)
{
    int square_x = MOVEMENT_X_MIN + (leitura_x * (MOVEMENT_X_MAX - MOVEMENT_X_MIN)) / 4095;
    int square_y = MOVEMENT_Y_MAX - (leitura_y * (MOVEMENT_Y_MAX - MOVEMENT_Y_MIN)) / 4095;
    move_square(&ssd, square_x, square_y);
}

void gerenciar_estado_motor(uint16_t leitura_x, uint16_t leitura_y)
{
    if (estado_motor == MOTOR_DESLIGADO)
    {
        return;
    }

    bool dentro_limite_x = (leitura_x > LIMIAR_MIN && leitura_x < LIMIAR_MAX);
    bool dentro_limite_y = (leitura_y > LIMIAR_MIN && leitura_y < LIMIAR_MAX);

    if (dentro_limite_x && dentro_limite_y)
    {
        if (estado_motor == MOTOR_LIGADO_MOVIMENTO)
        {
            estado_motor = MOTOR_LIGADO_PARADO;
            tempo_inicio_parado = get_absolute_time();
            estado_motor_matrix(2); // Exibe "-"
        }
        else if (estado_motor == MOTOR_LIGADO_PARADO || estado_motor == MOTOR_LIGADO_OCIOSO)
        {
            if (absolute_time_diff_us(tempo_inicio_parado, get_absolute_time()) >= TEMPO_OCIOSIDADE_MS * 1000)
            {
                estado_motor = MOTOR_LIGADO_OCIOSO;
                estado_motor_matrix(0); // Exibe "!"
                tocar_nota(BUZZER_PIN_A, 400, 100);
                tocar_nota(BUZZER_PIN_B, 250, 100);
                tempo_ocioso += 300;
            }
        }
    }
    else
    {
        estado_motor_matrix(1); // Exibe "+"
        estado_motor = MOTOR_LIGADO_MOVIMENTO;
        gasto_tempo += 100;
    }
}

void loop()
{
    uint16_t leitura_x, leitura_y;

    ler_joystick(&leitura_x, &leitura_y);
    atualizar_posicao_quadrado(leitura_x, leitura_y);
    gerenciar_estado_motor(leitura_x, leitura_y);

    atualizar_leds();
    exibir_estado_no_display();

    sleep_ms(10);
}

// Função principal
int main()
{
    init_hardware();        // Inicializa o hardware
    sleep_ms(10);           // Aguarda 100 ms para estabilização
    estado_motor_matrix(3); // Exibe o símbolo "-" branco inicialmente

    while (1)
    {
        loop(); // Executa o loop principal
    }
}