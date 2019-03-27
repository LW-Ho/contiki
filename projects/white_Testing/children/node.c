/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         A RPL+TSCH node able to act as either a simple node (6ln),
 *         DAG Root (6dr) or DAG Root with security (6dr-sec)
 *         Press use button at startup to configure.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

#include "contiki.h"
#include "contiki-net.h"
#include "rest-engine.h"


#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#if RPL_WITH_NON_STORING
#include "net/rpl/rpl-ns.h"
#endif /* RPL_WITH_NON_STORING */

#if PLATFORM_HAS_BUTTON
#include "dev/button-sensor.h"
#endif


// Testing 
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include "collect_sensorData.h"


#if WITH_ORCHESTRA
#include "orchestra.h"
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#include "dev/leds.h"
#include "dev/sht21.h"  //temporaly

//#include "dev/max44009.h"  //light

//extern resource_t res_hello, res_push, res_toggle, res_collect;
extern resource_t res_arduinoBoard, res_sht21, res_sicslowpan, res_punch; // , res_temperature;

uint32_t * tempData[32];
static int count = 0;
static int count_backup = 0;
static int collect_flag = 0;

/*---------------------------------------------------------------------------*/

PROCESS(er_example_server, "Erbium Example Server");
PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&er_example_server, &node_process);

PROCESS_THREAD(er_example_server, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  PRINTF("Starting Erbium Example Server\n");
  leds_toggle(LEDS_GREEN);
#ifdef RF_CHANNEL
  PRINTF("RF channel: %u\n", RF_CHANNEL);
#endif
#ifdef IEEE802154_PANID
  PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);
#endif

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine();

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  //rest_activate_resource(&res_hello, "test/hello");
  //rest_activate_resource(&res_push, "test/push");
  //rest_activate_resource(&res_toggle, "actuators/toggle");
  //rest_activate_resource(&res_collect, "g/collect");
  rest_activate_resource(&res_arduinoBoard, "g/arduinoBoard");
  rest_activate_resource(&res_sht21, "g/sht21");
  rest_activate_resource(&res_sicslowpan, "g/sicslowpan");
  rest_activate_resource(&res_punch, "g/punch");
  //rest_activate_resource(&res_temperature, "g/res_temperature");

#if PLATFORM_HAS_LEDS
 
#endif

#if WITH_ORCHESTRA
  orchestra_init();
#endif
  /* Define application-specific events here. */
  while(1) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

#include "core/net/mac/tsch/tsch-private.h"
extern struct tsch_asn_t tsch_current_asn;

static void
print_network_status(void)
{

  int i;
  uint8_t state;
  uip_ds6_defrt_t *default_route;
#if RPL_WITH_STORING
  uip_ds6_route_t *route;
#endif /* RPL_WITH_STORING */
#if RPL_WITH_NON_STORING
  rpl_ns_node_t *link;
#endif /* RPL_WITH_NON_STORING */

  PRINTF("--- Network status ---\n");

  /* Our IPv6 addresses */
  PRINTF("- Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINTF("-- ");
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }

  /* Our default route */
  PRINTF("- Default route:\n");
  default_route = uip_ds6_defrt_lookup(uip_ds6_defrt_choose());
  if(default_route != NULL) {
    PRINTF("-- ");
    PRINT6ADDR(&default_route->ipaddr);
    PRINTF(" (lifetime: %lu seconds)\n", (unsigned long)default_route->lifetime.interval);
  } else {
    PRINTF("-- None\n");
  }

#if RPL_WITH_STORING
  /* Our routing entries */
  PRINTF("- Routing entries (%u in total):\n", uip_ds6_route_num_routes());
  route = uip_ds6_route_head();
  while(route != NULL) {
    PRINTF("-- ");
    PRINT6ADDR(&route->ipaddr);
    PRINTF(" via ");
    PRINT6ADDR(uip_ds6_route_nexthop(route));
    PRINTF(" (lifetime: %lu seconds)\n", (unsigned long)route->state.lifetime);
    route = uip_ds6_route_next(route);
  }

#endif /* RPL_WITH_STORING */

#if RPL_WITH_NON_STORING

  /* Our routing links */
  PRINTF("- Routing links (%u in total):\n", rpl_ns_num_nodes());
  link = rpl_ns_node_head();
  while(link != NULL) {
    uip_ipaddr_t child_ipaddr;
    uip_ipaddr_t parent_ipaddr;
    rpl_ns_get_node_global_addr(&child_ipaddr, link);
    rpl_ns_get_node_global_addr(&parent_ipaddr, link->parent);
    PRINTF("-- ");
    PRINT6ADDR(&child_ipaddr);
    if(link->parent == NULL) {
      memset(&parent_ipaddr, 0, sizeof(parent_ipaddr));
      PRINTF(" --- DODAG root ");
    } else {
      PRINTF(" to ");
      PRINT6ADDR(&parent_ipaddr);
    }
    PRINTF(" (lifetime: %lu seconds)\n", (unsigned long)link->lifetime);
    link = rpl_ns_node_next(link);
  }
#endif  /* RPL_WITH_NON_STORING */
  PRINTF("----------------------\n");
}

static void
print_tempAndhumi_status(void) 
{
  //static int16_t sht21_present; //,max44009_present;  //uint16 to int16----important
  static int16_t temperature, humidity; //,light;
  
  PRINTF("============================\n");
  if(sht21.status(SENSORS_READY) == 1) {//sht21_present != SHT21_ERROR
    temperature = sht21.value(SHT21_READ_TEMP);
    PRINTF("Temperature Row Data: %d\n",temperature );
    PRINTF("Temperature Row Data: %16x\n",temperature );
    PRINTF("Temperature: %u.%uC\n", temperature / 100, temperature % 100);
    humidity = sht21.value(SHT21_READ_RHUM);
    PRINTF("Rel. humidity: %u.%u%%\n", humidity / 100, humidity % 100);
    }
    else {
      PRINTF("%u\n",sht21.status(SENSORS_READY));
      PRINTF("SHT21 doesn't open\n");
    }
  PRINTF("============================\n");
}

void
collect_data_send(char* data) 
{
  int split;
  int i;

  INTERRUPTS_DISABLE();
  PRINTF("The data is : %s \n",data);
  //split = strtok(data,",");
  split = atoi(data);
  // found start word, goto default value.
  if(split == 54){
    collect_flag = 1;
    count = 0;
    memset(tempData,'\0',sizeof(tempData)); // free the tempData
  }
  // found end word, goto default value.
  else if(split == 66){
    for (i=0;i<count;i++){
      PRINTF("i : %d , count : %d\n",i, count);
      PRINTF("Each the sensor data %u \n",tempData[i]);
    }
    count_backup = count;
    count = 0;
    collect_flag = 0;
    memset(tempData,'\0',sizeof(tempData)); // free the tempData
  }
  else if(collect_flag){
    //tempData[count] = atoi(split);
    tempData[count] = split;
    count++;
  }
  INTERRUPTS_ENABLE();
  return 0;
}

/*---------------------------------------------------------------------------*/
// MARK: return Sensor Data Array.
uint32_t *
return_Sensor_Data(void)
{
  return tempData;
}
/*---------------------------------------------------------------------------*/
int *
return_collect_data_of_numbers(void)
{
  return count_backup;
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer etaa;

  PROCESS_BEGIN();

  //uart_set_input(1, serial_line_input_byte);
  uart1_set_input(serial_line_input_byte);
  //etimer_set(&etaa, CLOCK_SECOND * 20);
  while(1) {

    //if(sht21.status(SENSORS_READY) == 0) {
      PROCESS_WAIT_EVENT();
      INTERRUPTS_DISABLE();
      if(ev == serial_line_event_message) {
      char * rxdata;
      //leds_toggle(LEDS_RED);

      rxdata = data;
      PRINTF("Data received over UART %s\n", rxdata);

      // the data send to node.c, then compress into packet.
      collect_data_send(rxdata);
      
      PRINTF("Received Done.\n");
      }else {
        PRINTF("Nothing... \n");
      }
      INTERRUPTS_ENABLE();
    //}
    // else {
    //   PROCESS_YIELD_UNTIL(etimer_expired(&etaa));
    //   etimer_reset(&etaa);
    //   print_tempAndhumi_status();
    // }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
