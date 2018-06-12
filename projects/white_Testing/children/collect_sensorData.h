/**
 * \file
 *         Common code between collect sensor data from Arduino
 * \author
 *         White.H
 */


#ifndef COLLECT_SENSORDATA_H_
#define COLLECT_SENSORDATA_H_

#include "contiki.h"

unsigned int uart1_send_bytes(const unsigned char *s, unsigned int len);

void collect_data_send(char* data);
void print_debug_information(void);

PROCESS_NAME(collect_sensor_data);

#endif /* COLLECT_SENSORDATA_H_ */