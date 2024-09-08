#include "contiki.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "dev/leds.h"
#include "sys/node-id.h"
#include <stdint.h>
#include <inttypes.h>
#include "dev/button-hal.h"

#include "sys/log.h"

#define LOG_MODULE "Client"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_PORT 5678

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn; // udp_conn saves info of the comunication
const int GROUP_NUM = 1;

process_event_t new_destination_id;

/*---------------------------------------------------------------------------*/
PROCESS(udp_process, "UDP client");
PROCESS(dest_config, "Destination node ID config");
AUTOSTART_PROCESSES(&udp_process, &dest_config);
/*---------------------------------------------------------------------------*/
static void // When msg recived this function is excecuted
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
  leds_toggle(LEDS_RED);
  LOG_INFO("Mensaje recibido '%.*s' de ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[36];
  static radio_value_t ch_num;
  static uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  uip_ip6addr(&dest_ipaddr, 0xfd00, 0x0000, 0x0000, 0x0000, 0x0201, 0x0001, 0x0001, 0x0001);

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  /* Initialize NodeId identification */
  node_id_init();

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &ch_num);
  LOG_INFO("RF_CHANNEL: %d\n", ch_num);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL); // random time 0 to 10 s
  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER && data == &periodic_timer)
    {
      LOG_INFO("Conectando con servidor en IP: ");
      LOG_INFO_6ADDR(&dest_ipaddr); // all motes connect to this IP, IP of multicast
      LOG_INFO_("\n\r");

      snprintf(str, sizeof(str), "Hola soy el cliente %x", node_id);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr); // send info to multicast IP
      leds_toggle(LEDS_GREEN);

      /* Add some jitter */
      etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND))); // random time 9 to 11 s
    }
    else if (ev == new_destination_id)
    {
      uint16_t *node_id = data;
      LOG_INFO("Nuevo nodo destino de ID: %d\n\r", *node_id);
      uip_ip6addr(&dest_ipaddr, 0xfd00, 0x0000, 0x0000, 0x0000, 0x0200 + *node_id, *node_id, *node_id, *node_id);
    }
  }

  PROCESS_END();
}

PROCESS_THREAD(dest_config, ev, data)
{
  static struct etimer timer;
  static int btn_count = 0;
  static int node_id;
  static button_hal_button_t *btn;

  PROCESS_BEGIN();

  new_destination_id = process_alloc_event();

  while (1)
  {
    PROCESS_WAIT_EVENT();

    // Some button was pressed
    if (ev == button_hal_press_event)
    {
      btn = (button_hal_button_t *)data;
      LOG_INFO("Boton (%s) presionado\n\r", BUTTON_HAL_GET_DESCRIPTION(btn));

      // Check that the pressed button is BTN-1
      if (btn == button_hal_get_by_id(BUTTON_HAL_ID_BUTTON_ZERO))
      {
        btn_count++;
        LOG_INFO("BTN-1 detectado, nueva cuenta: %d\n\r", btn_count);
        etimer_set(&timer, 3 * CLOCK_SECOND);
      }
    }

    // Send event and reset node_id when timeout
    if (ev == PROCESS_EVENT_TIMER && data == &timer)
    {
      node_id = btn_count;
      btn_count = 0;
      process_post(&udp_process, new_destination_id, &node_id);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/