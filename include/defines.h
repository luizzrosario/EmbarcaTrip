#ifndef DEFINES_H
#define DEFINES_H

// ==== Definições de Hardware ====
// Sensores de Joystick
#define SENSOR_X        27
#define SENSOR_Y        26
#define BUTTON_JOYSTICK 22

// Buzzers
#define BUZZER_PIN_A    10
#define BUZZER_PIN_B    21

// Botões de Controle
#define BUTTON_A_PIN    5
#define BUTTON_B_PIN    6

// LEDs Indicadores
#define LED_R_PIN       13
#define LED_G_PIN       11
#define LED_B_PIN       12

// Display OLED via I2C
#define I2C_PORT        i2c1
#define I2C_SDA         14
#define I2C_SCL         15
#define ENDERECO_OLED   0x3C

// Dimensões do Display
#define WIDTH           128
#define HEIGHT          64

// LEDs WS2812
#define WS2812_PIN      7
#define MAX_LEDS        25

// ==== Definições de Parâmetros de Funcionamento ====
// Limiares de Movimento do Joystick
#define LIMIAR_MIN      1840
#define LIMIAR_MAX      2200

// Limites de Movimento do Quadrado no Display
#define MOVEMENT_X_MIN  0
#define MOVEMENT_X_MAX  120
#define MOVEMENT_Y_MIN  8
#define MOVEMENT_Y_MAX  44

// Tempos de Controle
#define TEMPO_OCIOSIDADE_MS 10000 // 10 segundos
#define TEMPO_DEBOUNCE_MS   200   // 200 ms



#endif // DEFINES_H