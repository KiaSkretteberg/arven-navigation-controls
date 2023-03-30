/*
 * web.h
 * Communications with the web server API
 * Relies on TLS and Wifi
 * 
 * Implementation based off of https://www.i-programmer.info/programming/hardware/15838-the-picow-in-c-simple-web-client.html
 *
 * Created: 2023-03-29
 * Author: Kia Skretteberg
 */
#include "lwipopts.h"

#define WEB_CLIENT_SERVER       "app.rx-arven.com"
#define WEB_CLIENT_REQUEST_URL  "/api" 
#define WEB_CLIENT_PORT         80 //TODO: Should/can we use a different port?

int web_init(const char *ssid, const char *pass, const char *hostname, 
                ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);

void web_request(char * uriParams);