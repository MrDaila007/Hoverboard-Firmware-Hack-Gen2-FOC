/*
 * Host unit tests for sinusoidal commutation tables and angle tracking.
 * Build: make -C test
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../Inc/bldc_sinusoidal.h"

static int tests_run;
static int tests_failed;

#define ASSERT(cond) do { \
    tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        tests_failed++; \
    } \
} while (0)

#define ASSERT_EQ_INT(a, b) ASSERT((a) == (b))
#define ASSERT_NEAR(val, expected, tol) ASSERT(fabs((double)(val) - (double)(expected)) <= (tol))

static void test_lut_shape(void)
{
    const int16_t *lut = bldc_sin_lut();

    ASSERT_EQ_INT(lut[0], 0);
    ASSERT_NEAR(lut[64], 1000, 15);   /* 90°  → ~sin(π/2)*1000 */
    ASSERT_NEAR(lut[128], 0, 2);      /* 180° */
    ASSERT_NEAR(lut[192], -1000, 15); /* 270° */
    ASSERT_EQ_INT(lut[256 - 1], lut[1] * -1); /* odd symmetry at wrap */
}

static void test_hall_tables(void)
{
    ASSERT_EQ_INT(bldc_hall_to_pos[1], 3);
    ASSERT_EQ_INT(bldc_hall_to_pos[2], 5);
    ASSERT_EQ_INT(bldc_hall_to_pos[6], 6);
    ASSERT_EQ_INT(bldc_hall_to_pos[0], 0);
    ASSERT_EQ_INT(bldc_hall_to_pos[7], 0);

    ASSERT_EQ_INT(bldc_pos_to_angle[1], 0);
    ASSERT_EQ_INT(bldc_pos_to_angle[2], BLDC_SIN_SECTOR_ANGLE);
    ASSERT_EQ_INT(bldc_pos_to_angle[4], 32768U);
    ASSERT_EQ_INT(bldc_pos_to_angle[6], 54613U);
}

static void test_hall_snap_and_interpolate(void)
{
    bldc_sin_state_t st;

    bldc_sin_state_init(&st);
    st.hall_period = 1000;
    bldc_sin_on_hall_change(&st, 1); /* hall 1 → pos 3 → 120° */

    ASSERT_EQ_INT(st.rotor_angle, bldc_pos_to_angle[3]);
    ASSERT_EQ_INT(st.last_hall, 1);
    ASSERT_EQ_INT(st.angle_per_tick, (uint16_t)(BLDC_SIN_SECTOR_ANGLE / 1000U));

    bldc_sin_advance(&st);
    ASSERT_EQ_INT(st.hall_tick, 1U);
    ASSERT_EQ_INT(st.rotor_angle, st.last_angle + st.angle_per_tick);
}

static void test_invalid_hall_ignored(void)
{
    bldc_sin_state_t st;

    bldc_sin_state_init(&st);
    st.rotor_angle = 1234;
    bldc_sin_on_hall_change(&st, 0);
    bldc_sin_on_hall_change(&st, 7);

    ASSERT_EQ_INT(st.rotor_angle, 1234);
}

static void test_pwm_at_zero_angle(void)
{
    int y, b, g;

    bldc_sin_calc_pwm(1000, 0, &y, &b, &g);
    ASSERT_EQ_INT(y, 0);
    ASSERT_NEAR(b, 866, 20);
    ASSERT_NEAR(g, -866, 20);
}

static void test_pwm_zero_drive(void)
{
    int y, b, g;

    bldc_sin_calc_pwm(0, 10923, &y, &b, &g);
    ASSERT_EQ_INT(y, 0);
    ASSERT_EQ_INT(b, 0);
    ASSERT_EQ_INT(g, 0);
}

int main(void)
{
    test_lut_shape();
    test_hall_tables();
    test_hall_snap_and_interpolate();
    test_invalid_hall_ignored();
    test_pwm_at_zero_angle();
    test_pwm_zero_drive();

    if (tests_failed != 0)
    {
        fprintf(stderr, "%d of %d checks failed\n", tests_failed, tests_run);
        return EXIT_FAILURE;
    }

    printf("All %d checks passed\n", tests_run);
    return EXIT_SUCCESS;
}
