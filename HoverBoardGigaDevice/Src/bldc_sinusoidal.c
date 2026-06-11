/*
 * Sinusoidal commutation tables and angle tracking (host-testable, no MCU deps).
 */
#include "../Inc/bldc_sinusoidal.h"

/* 256-entry sine LUT: sin(2*pi*i/256)*1000, index = angle_u16 >> 8 */
static const int16_t sine_lut[256] = {
     0,   25,   49,   74,   98,  122,  147,  171,  195,  219,  243,  267,  290,  314,  337,  361,
   384,  406,  429,  451,  473,  494,  515,  535,  556,  576,  595,  615,  634,  652,  670,  688,
   707,  724,  741,  757,  773,  788,  803,  818,  831,  844,  857,  869,  880,  891,  901,  911,
   920,  929,  937,  944,  951,  957,  963,  968,  972,  976,  980,  982,  985,  987,  988,  989,
   990,  989,  988,  987,  985,  982,  980,  976,  972,  968,  963,  957,  951,  944,  937,  929,
   920,  911,  901,  891,  880,  869,  857,  844,  831,  818,  803,  788,  773,  757,  741,  724,
   707,  688,  670,  652,  634,  615,  595,  576,  556,  535,  515,  494,  473,  451,  429,  406,
   384,  361,  337,  314,  290,  267,  243,  219,  195,  171,  147,  122,   98,   74,   49,   25,
     0,  -25,  -49,  -74,  -98, -122, -147, -171, -195, -219, -243, -267, -290, -314, -337, -361,
  -384, -406, -429, -451, -473, -494, -515, -535, -556, -576, -595, -615, -634, -652, -670, -688,
  -707, -724, -741, -757, -773, -788, -803, -818, -831, -844, -857, -869, -880, -891, -901, -911,
  -920, -929, -937, -944, -951, -957, -963, -968, -972, -976, -980, -982, -985, -987, -988, -989,
  -990, -989, -988, -987, -985, -982, -980, -976, -972, -968, -963, -957, -951, -944, -937, -929,
  -920, -911, -901, -891, -880, -869, -857, -844, -831, -818, -803, -788, -773, -757, -741, -724,
  -707, -688, -670, -652, -634, -615, -595, -576, -556, -535, -515, -494, -473, -451, -429, -406,
  -384, -361, -337, -314, -290, -267, -243, -219, -195, -171, -147, -122,  -98,  -74,  -49,  -25
};

const uint8_t bldc_hall_to_pos[8] = {
    0,
    3,
    5,
    4,
    1,
    2,
    6,
    0,
};

const uint16_t bldc_pos_to_angle[8] = {
    0,
    0,
    10923,
    21845,
    32768,
    43691,
    54613,
    0,
};

const int16_t *bldc_sin_lut(void)
{
    return sine_lut;
}

void bldc_sin_state_init(bldc_sin_state_t *st)
{
    st->rotor_angle = 0;
    st->angle_per_tick = 0;
    st->last_angle = 0;
    st->hall_tick = 0;
    st->hall_period = 1600;
    st->last_hall = 0;
}

void bldc_sin_on_hall_change(bldc_sin_state_t *st, uint8_t hall)
{
    uint8_t current_pos = bldc_hall_to_pos[hall & 0x07U];

    if (current_pos == 0U)
    {
        return;
    }

    if (st->hall_tick > 50U && st->hall_tick < 32000U)
    {
        st->hall_period = (st->hall_period * 3U + st->hall_tick) / 4U;
    }
    st->hall_tick = 0;

    st->last_angle = bldc_pos_to_angle[current_pos];
    st->rotor_angle = st->last_angle;
    st->angle_per_tick = (uint16_t)(BLDC_SIN_SECTOR_ANGLE / st->hall_period);
    st->last_hall = hall;
}

void bldc_sin_advance(bldc_sin_state_t *st)
{
    st->hall_tick++;
    st->rotor_angle = st->last_angle +
        (uint16_t)((uint32_t)st->hall_tick * st->angle_per_tick);
}

void bldc_sin_calc_pwm(int16_t pwm, uint16_t angle, int *y, int *b, int *g)
{
    *y = (int)((int32_t)pwm * sine_lut[angle >> 8] / 1000);
    *b = (int)((int32_t)pwm * sine_lut[(uint8_t)((angle + BLDC_SIN_PHASE_B_OFFSET) >> 8)] / 1000);
    *g = (int)((int32_t)pwm * sine_lut[(uint8_t)((angle + BLDC_SIN_PHASE_G_OFFSET) >> 8)] / 1000);
}
