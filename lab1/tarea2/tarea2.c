#include "contiki.h"
#include "dev/leds.h"
#include "sys/etimer.h"
#include "lib/sensors.h"
#include "batmon-sensor.h"
#include <stdio.h>

static struct etimer et;
process_event_t medidaslistas;

#define NUM_SAMPLES 10

PROCESS(sensor_monitor, "Sensor Monitor");
PROCESS(publicar, "Publicar Valores");
AUTOSTART_PROCESSES(&sensor_monitor, &publicar);

PROCESS_THREAD(sensor_monitor, ev, data)
{
  PROCESS_BEGIN();

  static int temp;
  static int volt;
  static int i;
  static int medidas[2] = {};

  etimer_set(&et, CLOCK_SECOND * 5); // Set timer to fire every 5 seconds

  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER && data == &et)
    {
      printf("Event: Timer triggered\n\r");

      temp = 0;
      volt = 0;

      SENSORS_ACTIVATE(batmon_sensor);

      for (i = 0; i < NUM_SAMPLES; i++)
      {
        // IIR, alpha = 0.5
        temp = (batmon_sensor.value(BATMON_SENSOR_TYPE_TEMP) + temp) / 2;
        volt = (((batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5) + volt) / 2;

        etimer_set(&et, CLOCK_SECOND / 4); // Wait for 250 ms between samples
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &et);
      }

      SENSORS_DEACTIVATE(batmon_sensor);

      // Post event
      medidas[0] = temp;
      medidas[1] = volt;
      medidaslistas = process_alloc_event();
      process_post(&publicar, medidaslistas, &medidas);

      // Reset the timer
      etimer_set(&et, CLOCK_SECOND * 10);
    }
  }

  PROCESS_END();
}

PROCESS_THREAD(publicar, ev, data)
{
  PROCESS_BEGIN();

  while (1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == medidaslistas)
    {
      int *medidas = data;
      printf("Temperatura promedio: %d °C\n\r", medidas[0]);
      printf("Voltaje promedio: %d mV\n\r", medidas[1]);

      leds_toggle(LEDS_ALL);
    }
  }
  PROCESS_END();
}