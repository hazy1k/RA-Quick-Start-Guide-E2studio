/*
 * rtc.h
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */

#ifndef RTC_H_
#define RTC_H_

#include "hal_data.h"

#define YEAR_FIXED 1900

void RTC_Init(void);
bool Rtc_GetTime(rtc_time_t *Nne_time);
void FormatRtcTime(const rtc_time_t *time, char *buffer, size_t size);

#endif /* RTC_H_ */
