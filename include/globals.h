#ifndef GLOBALS_H
#define GLOBALS_H 

#include "defines.h"

// ==== Variáveis Globais ====
// Controle dos LEDs WS2812
static int sm = 0;
static PIO pio = pio0;
static uint32_t grb[MAX_LEDS];

// Controle do Display
ssd1306_t ssd;

// Estados do Motor
typedef enum {
    MOTOR_DESLIGADO,
    MOTOR_LIGADO_MOVIMENTO,
    MOTOR_LIGADO_PARADO,
    MOTOR_LIGADO_OCIOSO
} estado_motor_t;

// Controle do Sistema
volatile estado_motor_t estado_motor = MOTOR_DESLIGADO;
volatile uint32_t last_interrupt_time = 0;
absolute_time_t tempo_inicio_parado;
uint32_t gasto_tempo = 0;
uint32_t viagem = 0;
uint32_t tempo_ocioso = 0;

#endif