#include "contiki.h"
#include "dev/leds.h"
#include "sys/etimer.h"
#include "lib/sensors.h"
#include "batmon-sensor.h"
#include <stdio.h>

static struct etimer et;

#define NUM_SAMPLES 10

PROCESS(sensor_monitor, "Sensor Monitor");
AUTOSTART_PROCESSES(&sensor_monitor);

PROCESS_THREAD(sensor_monitor, ev, data)
{
  PROCESS_BEGIN();

  static int temp;
  static int volt;
  static int i;

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

      // Show values
      printf("Temperatura promedio: %d °C\n\r", temp);
      printf("Voltaje promedio: %d mV\n\r", volt);

      leds_toggle(LEDS_ALL);

      // Reset the timer
      etimer_set(&et, CLOCK_SECOND * 10);
    }
  }

  PROCESS_END();
}