/*
* This file is part of the hoverboard-firmware-hack-V2 project. The 
* firmware is used to hack the generation 2 board of the hoverboard.
* These new hoverboards have no mainboard anymore. They consist of 
* two Sensorboards which have their own BLDC-Bridge per Motor and an
* ARM Cortex-M3 processor GD32F130C8.
*
* Copyright (C) 2018 Florian Staeblein
* Copyright (C) 2018 Jakob Broemauer
* Copyright (C) 2018 Kai Liebich
* Copyright (C) 2018 Christoph Lehnert
*
* The program is based on the hoverboard project by Niklas Fauth. The 
* structure was tried to be as similar as possible, so that everyone 
* could find a better way through the code.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gd32f1x0.h"
#include "../Inc/setup.h"
#include "../Inc/defines.h"
#include "../Inc/config.h"

// Internal constants
const int16_t pwm_res = 72000000 / 2 / PWM_FREQ; // = 2000

// 256-entry sine LUT: sin(2*pi*i/256)*1000, index = angle_u16 >> 8
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

// Global variables for voltage and current
float batteryVoltage = 40.0;
float currentDC = 0.0;
float realSpeed = 0.0;

// Timeoutvariable set by timeout timer
extern FlagStatus timedOut;

// Variables to be set from the main routine
int16_t bldc_inputFilterPwm = 0;
FlagStatus bldc_enable = RESET;

// ADC buffer to be filled by DMA
adc_buf_t adc_buffer;

// Internal calculation variables
uint8_t hall_a;
uint8_t hall_b;
uint8_t hall_c;
uint8_t hall;
uint8_t pos;
uint8_t lastPos;
int16_t bldc_outputFilterPwm = 0;
int32_t filter_reg;
FlagStatus buzzerToggle = RESET;
uint8_t buzzerFreq = 0;
uint8_t buzzerPattern = 0;
uint16_t buzzerTimer = 0;
int16_t offsetcount = 0;
int16_t offsetdc = 2000;
uint32_t speedCounter = 0;

//----------------------------------------------------------------------------
// Commutation table
//----------------------------------------------------------------------------
const uint8_t hall_to_pos[8] =
{
	// annotation: for example SA=0 means hall sensor pulls SA down to Ground
  0, // hall position [-] - No function (access from 1-6) 
  3, // hall position [1] (SA=1, SB=0, SC=0) -> PWM-position 3
  5, // hall position [2] (SA=0, SB=1, SC=0) -> PWM-position 5
  4, // hall position [3] (SA=1, SB=1, SC=0) -> PWM-position 4
  1, // hall position [4] (SA=0, SB=0, SC=1) -> PWM-position 1
  2, // hall position [5] (SA=1, SB=0, SC=1) -> PWM-position 2
  6, // hall position [6] (SA=0, SB=1, SC=1) -> PWM-position 6
  0, // hall position [-] - No function (access from 1-6)
};

// Maps bldc pos (1..6) to rotor electrical angle in uint16 units (0=0°, 65536=360°)
// Sector boundary angles: pos1=0°, pos2=60°, pos3=120°, pos4=180°, pos5=240°, pos6=300°
// NOTE: if motor spins wrong direction on hardware, reverse order to {0, 0, 54613, 43691, 32768, 21845, 10923, 0}
static const uint16_t pos_to_angle[8] = {0, 0, 10923, 21845, 32768, 43691, 54613, 0};

//----------------------------------------------------------------------------
// Block PWM calculation based on position
//----------------------------------------------------------------------------
__INLINE void blockPWM(int pwm, int pwmPos, int *y, int *b, int *g)
{
  switch(pwmPos)
	{
    case 1:
      *y = 0;
      *b = pwm;
      *g = -pwm;
      break;
    case 2:
      *y = -pwm;
      *b = pwm;
      *g = 0;
      break;
    case 3:
      *y = -pwm;
      *b = 0;
      *g = pwm;
      break;
    case 4:
      *y = 0;
      *b = -pwm;
      *g = pwm;
      break;
    case 5:
      *y = pwm;
      *b = -pwm;
      *g = 0;
      break;
    case 6:
      *y = pwm;
      *b = 0;
      *g = -pwm;
      break;
    default:
      *y = 0;
      *b = 0;
      *g = 0;
  }
}

//----------------------------------------------------------------------------
// Set motor enable
//----------------------------------------------------------------------------
void SetEnable(FlagStatus setEnable)
{
	bldc_enable = setEnable;
}

//----------------------------------------------------------------------------
// Set pwm -1000 to 1000
//----------------------------------------------------------------------------
void SetPWM(int16_t setPwm)
{
	bldc_inputFilterPwm = CLAMP(setPwm, -1000, 1000);
}

//----------------------------------------------------------------------------
// Calculation-Routine for BLDC => calculates with 16kHz
//----------------------------------------------------------------------------
void CalculateBLDC(void)
{
	int y = 0;     // yellow = phase A
	int b = 0;     // blue   = phase B
	int g = 0;     // green  = phase C
	
	// Calibrate ADC offsets for the first 1000 cycles
  if (offsetcount < 1000)
	{  
    offsetcount++;
    offsetdc = (adc_buffer.current_dc + offsetdc) / 2;
    return;
  }
	
	// Calculate battery voltage every 100 cycles
  if (buzzerTimer % 100 == 0)
	{
    batteryVoltage = batteryVoltage * 0.999 + ((float)adc_buffer.v_batt * ADC_BATTERY_VOLT) * 0.001;
  }
	
#ifdef MASTER
	// Create square wave for buzzer
  buzzerTimer++;
  if (buzzerFreq != 0 && (buzzerTimer / 5000) % (buzzerPattern + 1) == 0)
	{
    if (buzzerTimer % buzzerFreq == 0)
		{
			buzzerToggle = buzzerToggle == RESET ? SET : RESET; // toggle variable
		  gpio_bit_write(BUZZER_PORT, BUZZER_PIN, buzzerToggle);
    }
  }
	else
	{
		gpio_bit_write(BUZZER_PORT, BUZZER_PIN, RESET);
  }
#endif
	
	// Calculate current DC
	currentDC = ABS((adc_buffer.current_dc - offsetdc) * MOTOR_AMP_CONV_DC_AMP);

  // Disable PWM when current limit is reached (current chopping), enable is not set or timeout is reached
	if (currentDC > DC_CUR_LIMIT || bldc_enable == RESET || timedOut == SET)
	{
		timer_automatic_output_disable(TIMER_BLDC);		
  }
	else
	{
		timer_automatic_output_enable(TIMER_BLDC);
  }
	
  // Read hall sensors
	hall_a = gpio_input_bit_get(HALL_A_PORT, HALL_A_PIN);
  hall_b = gpio_input_bit_get(HALL_B_PORT, HALL_B_PIN);
	hall_c = gpio_input_bit_get(HALL_C_PORT, HALL_C_PIN);
  
	// Determine current position based on hall sensors
  hall = hall_a * 1 + hall_b * 2 + hall_c * 4;
  pos = hall_to_pos[hall];
	
	// Calculate low-pass filter for pwm value
	filter_reg = filter_reg - (filter_reg >> FILTER_SHIFT) + bldc_inputFilterPwm;
	bldc_outputFilterPwm = filter_reg >> FILTER_SHIFT;
	
  // Update PWM channels based on position y(ellow), b(lue), g(reen)
  blockPWM(bldc_outputFilterPwm, pos, &y, &b, &g);
	
	// Set PWM output (pwm_res/2 is the mean value, setvalue has to be between 10 and pwm_res-10)
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, CLAMP(g + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, CLAMP(b + pwm_res / 2, 10, pwm_res-10));
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, CLAMP(y + pwm_res / 2, 10, pwm_res-10));
	
	// Increments with 62.5us
	if(speedCounter < 4000) // No speed after 250ms
	{
		speedCounter++;
	}
	
	// Every time position reaches value 1, one round is performed (rising edge)
	if (lastPos != 1 && pos == 1)
	{
		realSpeed = 1991.81f / (float)speedCounter; //[km/h]
		speedCounter = 0;
	}
	else
	{
		if (speedCounter >= 4000)
		{
			realSpeed = 0;
		}
	}

	// Safe last position
	lastPos = pos;
}
