/**
 * \file
 *         Common code between collect sensor data from Arduino
 * \author
 *         White.H
 */


#ifndef COLLECT_SENSORDATA_H_
#define COLLECT_SENSORDATA_H_

// #include "contiki.h"

// unsigned int uart1_send_bytes(const unsigned char *s, unsigned int len);

// void collect_data_send(char* data);
// void print_debug_information(void);

// PROCESS_NAME(collect_sensor_data);

uint32_t * return_Sensor_Data(void);
int * return_collect_data_of_numbers(void);

#endif /* COLLECT_SENSORDATA_H_ */