#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdint.h>

#define API_PORT   1880
#define API_KEY    "CHANGE_ME_API_KEY"
#define API_PATH   "/rest/v1/event"
#define HTTP_SOCK  0

/* 192.168.1.105 */
static const uint8_t API_HOST_IP[4] = {192, 168, 1, 105};

int HTTP_Post(const uint8_t *host_ip, uint16_t port, const char *json_body);

#endif
