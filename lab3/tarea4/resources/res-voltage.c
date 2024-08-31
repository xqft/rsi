#include "contiki.h"

#include <string.h>
#include <stdio.h>
#include "coap-engine.h"

#define NUM_SAMPLES 5

#ifdef CONTIKI_TARGET_CC26X0_CC13X0
#pragma message("Utilizando el target cc26x0-cc13x0")
#include "batmon-sensor.h"
#else
#pragma message("Utilizando un target distinto al cc26x0-cc13x0")
#endif

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_voltage,
         "title=\"Voltage\";rt=\"Voltage\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    unsigned int accept = -1;
    coap_get_header_accept(request, &accept);

    if (accept == -1 || accept == TEXT_PLAIN)
    {
        coap_set_header_content_format(response, TEXT_PLAIN);

        #ifdef CONTIKI_TARGET_CC26X0_CC13X0
        // Initialize temp and voltage
        volt = 0;

        // Start measuring
        SENSORS_ACTIVATE(batmon_sensor);
        for (i = 0; i < NUM_SAMPLES; i++)
        {
            // IIR, alpha = 0.5
            volt = (((batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5) + volt) / 2;
        }
        SENSORS_DEACTIVATE(batmon_sensor);

        snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%d", volt);
        #else
        snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "3.3v (const)");
        #endif

        coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
    }
    else
    {
        coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
        const char *msg = "Supporting content-types text/plain";
        coap_set_payload(response, msg, strlen(msg));
    }
}
