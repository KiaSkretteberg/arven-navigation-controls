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
#include "lwip/apps/http_client.h"

#define WEB_CLIENT_SERVER       "api.rx-arven.com"
#define WEB_CLIENT_REQUEST_URL  "/api" 
#define WEB_CLIENT_PORT         80 //TODO: Should/can we use a different port?

typedef enum
{
	Web_RequestType_CheckSchedule = 0,
	Web_RequestType_LogDelivery = 1,
    Web_RequestType_RetrieveDoseStats = 2
} Web_RequestType;

struct Web_Request {
    bool active;
    char headers[1000];
    char body[1000];
    bool complete;
    Web_RequestType type;
};


int web_init(const char *ssid, const char *pass, const char *hostname, 
                ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);

void web_request(char * uriParams, Web_RequestType type);

int web_response_check_schedule(void);
int web_response_retrieve_dose_stats(void);

void web_request_check_schedule(void);
void web_request_retrieve_dose_stats(int schedule_id);
void web_request_log_delivery(int schedule_id);