#include "contiki.h"
#include "dev/leds.h"
#include "sys/etimer.h"
#include "lib/sensors.h"
#include "batmon-sensor.h"
#include "dev/button-hal.h"

#include <stdio.h>

static struct etimer et;
process_event_t medidaslistas;
process_event_t evboton;

#define NUM_SAMPLES 10

PROCESS(sensor_monitor, "Sensor monitor");
PROCESS(publicar, "Publicar valores");
PROCESS(boton, "Boton presionado");

AUTOSTART_PROCESSES(&sensor_monitor, &publicar, &boton);

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
    }
    else if (ev == evboton)
    {
      int *cuentasboton = data;
      printf("Cuentas de boton: %d\n\r", *cuentasboton);
    }

    leds_toggle(LEDS_ALL);
  }
  PROCESS_END();
}

PROCESS_THREAD(boton, ev, data)
{

  PROCESS_BEGIN();
  static int cuentasboton = 0;
  static button_hal_button_t *btn;

  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == button_hal_press_event)
    {
      btn = (button_hal_button_t *)data;
      printf("Boton (%s) presionado\n\r", BUTTON_HAL_GET_DESCRIPTION(btn));
      if (btn == button_hal_get_by_id(BUTTON_HAL_ID_BUTTON_ZERO))
      {
        printf("BTN-1 detectado, aumentando cuenta\n\r");
        cuentasboton++;
        evboton = process_alloc_event();
        process_post(&publicar, evboton, &cuentasboton);
      }
    }
  }
  PROCESS_END();
}
