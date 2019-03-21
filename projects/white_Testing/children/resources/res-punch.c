/**
 * \file
 *      Bcollect resource
 * \author
 *      White_CPS
 */

#include <string.h>
#include "rest-engine.h"
#include "er-coap.h"


// for sensor
#include "lib/sensors.h"
#include "dev/sht21.h"
#include "../collect_sensorData.h"

#include "core/net/rpl/rpl.h"
#include "core/net/link-stats.h"

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

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

PERIODIC_RESOURCE(res_punch,
                  "title=\"Binary collect\";obs",
                  res_get_handler,
                  res_post_handler,
                  NULL,
                  NULL,
                  1 * CLOCK_SECOND,
                  res_periodic_handler);

/*
 * Use local resource state that is accessed by res_get_handler() and altered by res_periodic_handler() or PUT or POST.
 */
static uint32_t event_counter = 0;

/* inter-packet time we generate a packet to send to observer */
static uint8_t event_threshold = 20;

/* record last change event threshold's event_counter */
static uint32_t event_threshold_last_change = 0;

/* Record the packet have been generated. (Server perspective) */
static uint32_t packet_counter = 0;

static uint8_t packet_priority = 0;

#include "core/net/mac/tsch/tsch-private.h"
extern struct tsch_asn_t tsch_current_asn;

static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /*
   * For minimal complexity, request query and options should be ignored for GET on observable resources.
   * Otherwise the requests must be stored with the observer list and passed by REST.notify_subscribers().
   * This would be a TODO in the corresponding files in contiki/apps/erbium/!
   */

  // static int8_t sht21_present=0; //, max44009_present=0, adxl346_present=0; 
  // static int16_t temperature_temp, humidity_temp; //, light, accelx, accely, accelz;
  int count = 0;
  memcpy(count, return_collect_data_of_numbers(), sizeof(count));

  uint32_t * sensorData[count]={}; //restore sensor data from node.c by UART.h

  // call main function, get the sensor data.
  //sensorData = return_Sensor_Data();
  memcpy(sensorData, return_Sensor_Data(), sizeof(sensorData));

  {
      struct 
      {
        // // 32bits to 1 block
        uint8_t flag[2];  // 0 1
        // uint8_t priority; // 2
        // int8_t gasAlarm; // 3
        // // Done padding int8_t // null
        // uint32_t start_asn; // 4 5 6 7
        // uint32_t end_asn; // 8 9 10 11
        // uint32_t event_counter; // 12 13 14 15
        // uint8_t event_threshold; // 16
        // // padding 3 int8_t and int16_t // 17, 18, 19
        // uint32_t event_threshold_last_change; // 20, 21, 22, 23
        // uint32_t packet_counter; // 24, 25, 26, 27
        // unsigned char parent_address[2]; // uint8[0] , uint8[1] 28,29
        // uint16_t rank; // 30, 31
        // uint16_t parnet_link_etx; //32, 33
        // int16_t parent_link_rssi; // 34, 35
        // int16_t gasValue; // 36, 37
        // //int8_t gasAlarm; // X
        // int16_t temperature; // 38, 39
        // int16_t humidity; // 40, 41
        // uint8_t end_flag[2]; // 42, 43
        // // Done padding int16_t //X null
        // // total size = 44
        uint32_t motoData[int(count)]; // 4 5 ~ 130 131 132 133
      } message;
      memset(&message, 0, sizeof(message));

      message.flag[0] = 0x54;
      message.flag[1] = 0x66;

      int temp;
      for (temp = 0;temp<count ;temp++){
        message.motoData[temp] = sensorData[temp];
      }
      PRINTF("Copy Done.\n");
      // message.end_flag[0] = 0xf0;
      // message.end_flag[1] = 0xff;

      // message.event_counter = event_counter;
      // message.event_threshold = event_threshold;
      // message.event_threshold_last_change = event_threshold_last_change;
      // message.packet_counter = packet_counter;

      // message.start_asn = tsch_current_asn.ls4b;

      // // for CPS enviorment Data.
      // message.gasValue = sensorData[0];
      // message.gasAlarm =  sensorData[1];
      // message.temperature = sensorData[2];
      // message.humidity = sensorData[3];

      // // for priority
      // message.priority = packet_priority;

      // uint8_t packet_length = 0;
      // rpl_dag_t *dag;
      // rpl_parent_t *preferred_parent;
      // linkaddr_t parent;
      // linkaddr_copy(&parent, &linkaddr_null);
      // struct link_stats *parent_link_stats;

      // if (message.gasAlarm) {
      //   packet_priority = 2;
      //   event_threshold = 1;
      // } else {
      //   event_threshold = 20;
      //   packet_priority = 0;
      // }


      PRINTF("I am sicslowpan res_get hanlder!\n");
      REST.set_header_content_type(response, REST.type.APPLICATION_OCTET_STREAM);
      REST.set_header_max_age(response, res_punch.periodic->period / CLOCK_SECOND);

      // packet_counter
      // memcpy(buffer,&packet_counter, sizeof(packet_counter));
      // packet_counter += sizeof(packet_counter);
      
      // dag = rpl_get_any_dag();
      // if(dag != NULL) {
      //   preferred_parent = dag->preferred_parent;
      //   if(preferred_parent != NULL) {
      //     uip_ds6_nbr_t *nbr;
      //     nbr = uip_ds6_nbr_lookup(rpl_get_parent_ipaddr(preferred_parent));
      //     if(nbr != NULL) {
      //       /* Use parts of the IPv6 address as the parent address, in reversed byte order. */
      //       parent.u8[LINKADDR_SIZE - 1] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 2];
      //       parent.u8[LINKADDR_SIZE - 2] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1];
      //       parent_link_stats = rpl_get_parent_link_stats(preferred_parent);
      //       message.parnet_link_etx = parent_link_stats->etx;
      //       message.parent_link_rssi = parent_link_stats->rssi;
      //     }
      //   }
      //   message.rank = dag->rank;
      // } else {
      // }

      // message.parent_address[0] = parent.u8[LINKADDR_SIZE - 1];
      // message.parent_address[1] = parent.u8[LINKADDR_SIZE - 2];

      memcpy(buffer, &message, sizeof(message));

      //coap_set_uip_traffic_class(packet_priority);
      REST.set_response_payload(response, buffer, sizeof(message));

  }

  // REST.set_response_payload(response, buffer, snprintf((char *)buffer, preferred_size, "[Collect] ec: %lu, et: %lu, lc, %lu, pc: %lu", event_counter, event_threshold, event_threshold_last_change,packet_counter));

  /* The REST.subscription_handler() will be called for observable resources by the REST framework. */
}


/* Used for update the threshold */
static void
res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *threshold_c = NULL;
  const char *priority_c = NULL;

  int threshold = -1;
  int priority = -1;

  if(REST.get_query_variable(request, "thd", &threshold_c)) {
    threshold = (uint8_t)atoi(threshold_c);
  }

  if(REST.get_query_variable(request, "pp", &priority_c)) {
    priority = (uint8_t)atoi(priority_c);
  }

  if(threshold < 1 && (priority<0||priority>2)) {
    /* Threashold is too smaill ignore it! */
    REST.set_response_status(response, REST.status.BAD_REQUEST);
  } else {
    if(threshold>=1){
      /* Update to new threshold */
      event_threshold = threshold;
      event_threshold_last_change = event_counter;
    }
    if(priority>=0 && priority<= 2)
    {
      packet_priority = priority;
    }
  }
}

/*
 * Additionally, a handler function named [resource name]_handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
  /* This periodic handler will be called every second */
  ++event_counter;

  /* Will notify subscribers when inter-packet time is match */
  if(event_counter % event_threshold == 0) {
    ++packet_counter;
    PRINTF("Generate a new packet! , %08x. \n",tsch_current_asn.ls4b);

    /* Notify the registered observers which will trigger the res_get_handler to create the response. */
    REST.notify_subscribers(&res_punch);
  }
}
