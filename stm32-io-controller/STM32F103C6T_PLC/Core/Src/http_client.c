#include "main.h"
#include "http_client.h"
#include "wizchip_conf.h"
#include "socket.h"
#include <stdio.h>
#include <string.h>

/* static buffer — 1 KB, RAM constraint: 20 KB total */
static uint8_t resp_buf[1024];
static char    req_buf[512];

int HTTP_Post(const uint8_t *host_ip, uint16_t port, const char *json_body)
{
    uint16_t body_len = (uint16_t)strlen(json_body);
    uint16_t req_len;
    int32_t  srcc;

    /* build HTTP/1.1 POST request */
    req_len = (uint16_t)snprintf(req_buf, sizeof(req_buf),
        "POST %s HTTP/1.1\r\n"
        "Host: %u.%u.%u.%u:%u\r\n"
        "Content-Type: application/json\r\n"
        "x-api-key: %s\r\n"
        "Content-Length: %u\r\n"
        "\r\n"
        "%s",
        API_PATH,
        host_ip[0], host_ip[1], host_ip[2], host_ip[3], port,
        API_KEY,
        body_len,
        json_body
    );

    /* open socket */
    int8_t s = socket(HTTP_SOCK, Sn_MR_TCP, 0, 0);
    if (s < 0) {
        printf("[http] socket open fail rc=%d\n", s);
        return -1;
    }

    /* connect */
    srcc = connect(s, (uint8_t *)host_ip, port, 4);
    if (srcc < 0) {
        printf("[http] connect fail rc=%d\n", (int)srcc);
        close(s);
        return -1;
    }

    /* send */
    int32_t sent = send(s, (uint8_t *)req_buf, req_len);
    if (sent < 0) {
        printf("[http] send fail rc=%d\n", (int)sent);
        close(s);
        return -1;
    }

    /* read response — read what we can, stop after first chunk or timeout */
    uint32_t t0 = HAL_GetTick();
    int32_t  recvd = 0;
    while (recvd < (int32_t)sizeof(resp_buf)) {
        int32_t r = recv(s, resp_buf + recvd, sizeof(resp_buf) - recvd);
        if (r > 0)       recvd += r;
        else if (r == 0) break;   /* socket closed by peer */
        else             break;   /* error/timeout */
        if (HAL_GetTick() - t0 > 5000) break;  /* hard timeout ~5 s */
        /* if we got a chunk, stop — response is small enough */
        if (r > 0) break;
    }

    close(s);

    printf("[http] %s → %d bytes rcv\n", API_PATH, (int)recvd);
    return 0;
}
