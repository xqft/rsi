#include "contiki.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "dev/leds.h"
#include "sys/node-id.h"
#include <stdint.h>
#include <inttypes.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_PORT	5678

#define SEND_INTERVAL		  (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_process, "UDP multicast");
AUTOSTART_PROCESSES(&udp_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

	leds_toggle(LEDS_RED);
  LOG_INFO("Received message '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[36];
	static radio_value_t ch_num;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

	/* Initialize NodeId identification */
	node_id_init();

	NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL,&ch_num);
	LOG_INFO("RF_CHANNEL: %d\n", ch_num);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    /* Generates multicast address */
    uip_create_linklocal_allnodes_mcast(&dest_ipaddr);

    LOG_INFO("Sending multicast (IP: ");
    LOG_INFO_6ADDR(&dest_ipaddr);
    LOG_INFO_(")\n");

    snprintf(str, sizeof(str), "Hello everybody! I am node %x.", node_id);
    simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
		leds_toggle(LEDS_GREEN);

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

