/*
 * dac.h
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */

#ifndef DAC_H_
#define DAC_H_

#include "hal_data.h"

void dac_set(uint16_t value);
void dac_init(void);
uint16_t voltage_transition(float value);

#endif /* DAC_H_ */
