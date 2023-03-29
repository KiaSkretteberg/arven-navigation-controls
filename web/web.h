/*
 * web.h
 * Communications with the web server API
 * Relies on TLS and Wifi
 *
 * Created: 2023-03-29
 * Author: Kia Skretteberg
 */

#define WEB_CLIENT_SERVER        "worldtimeapi.org"
#define WEB_CLIENT_HTTP_REQUEST  "GET /api HTTP/1.1\r\n" \
                                 "Host: " WEB_CLIENT_SERVER "\r\n" \
                                 "Connection: close\r\n" \
                                 "\r\n"
#define WEB_CLIENT_TIMEOUT_SECS  15

typedef struct WEB_CLIENT {
    struct altcp_pcb *pcb;
    bool complete;
} WEB_CLIENT;