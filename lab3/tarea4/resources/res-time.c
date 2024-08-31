#include "contiki.h"
#include "coap-engine.h"
#include "timestamp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_SAMPLES 5

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_time,
         "title=\"Time\";rt=\"Get/Set Timestamp\"",
         res_get_handler,
         res_put_handler,
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
        snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%d", timestamp_get());
        coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
    }
    else
    {
        coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
        const char *msg = "Supporting content-types text/plain";
        coap_set_payload(response, msg, strlen(msg));
    }
}

static void
res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    const uint8_t *bytes = NULL;
    coap_get_payload(request, &bytes);

    unsigned long timestamp = atol(bytes);
    timestamp_set(timestamp);
}