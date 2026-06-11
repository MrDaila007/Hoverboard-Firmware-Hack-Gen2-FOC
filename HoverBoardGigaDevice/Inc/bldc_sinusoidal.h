/*
 * Sinusoidal commutation tables and angle tracking (host-testable, no MCU deps).
 */
#ifndef BLDC_SINUSOIDAL_H
#define BLDC_SINUSOIDAL_H

#include <stdint.h>

/* 60° sector width in uint16 angle units (65536 = 360°) */
#define BLDC_SIN_SECTOR_ANGLE 10923U

/* 120° and 240° phase offsets for three-phase PWM */
#define BLDC_SIN_PHASE_B_OFFSET 21845U
#define BLDC_SIN_PHASE_G_OFFSET 43691U

extern const uint8_t bldc_hall_to_pos[8];
extern const uint16_t bldc_pos_to_angle[8];

typedef struct
{
    uint16_t rotor_angle;
    uint16_t angle_per_tick;
    uint16_t last_angle;
    uint32_t hall_tick;
    uint32_t hall_period;
    uint8_t last_hall;
} bldc_sin_state_t;

void bldc_sin_state_init(bldc_sin_state_t *st);

void bldc_sin_on_hall_change(bldc_sin_state_t *st, uint8_t hall);

void bldc_sin_advance(bldc_sin_state_t *st);

void bldc_sin_calc_pwm(int16_t pwm, uint16_t angle, int *y, int *b, int *g);

const int16_t *bldc_sin_lut(void);

#endif /* BLDC_SINUSOIDAL_H */
