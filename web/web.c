/*
 * web.c
 *
 * Created: 2023-03-29
 * Author: Kia Skretteberg
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "web.h"
#include "../dwm1001/dwm1001.h"


/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile struct Web_Request requests[3];
volatile char bodyBuff[1000];
volatile char headerBuff[1000];
httpc_connection_t settings;
const uint32_t COUNTRY = CYW43_COUNTRY_CANADA;
const uint32_t AUTH = CYW43_AUTH_WPA2_MIXED_PSK;

const char DEVICE_SERIAL[] = "RX-AR2023-0001";

const char PROPERTY_DELIM = ';';
const char PROPERTY_DELIM_STR[] = ";";
const char VALUE_DELIM = ':';

/************************************************************************/
/* Local Definitions (private functions)                                */
/************************************************************************/

void result_callback(void *arg, httpc_result_t httpc_result,
        u32_t rx_content_len, u32_t srv_res, err_t err);

err_t headers_callback(httpc_state_t *connection, void *arg, 
    struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

err_t body_callback(void *arg, struct altcp_pcb *conn, 
                            struct pbuf *p, err_t err);

/************************************************************************/
/* Header Implementation                                                */
/************************************************************************/

int web_init(const char *ssid, const char *pass, const char *hostname, 
                ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
    if (cyw43_arch_init_with_country(COUNTRY))
    {
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (hostname != NULL)
    {
        netif_set_hostname(netif_default, hostname);
    }

    if (cyw43_arch_wifi_connect_async(ssid, pass, AUTH))
    {
        return 2;
    }

    int flashrate = 1000;
    int status = CYW43_LINK_UP + 1;
    while (status != CYW43_LINK_UP)
    {
        int new_status = cyw43_tcpip_link_status(
             &cyw43_state,CYW43_ITF_STA);
        if (new_status != status)
        {
            status = new_status;
            if(status != -1) flashrate = flashrate / (status + 1);
            printf("connect status: %d %d\n", 
                                    status, flashrate);
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(flashrate);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(flashrate);
    }
    if (status < 0)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
    else
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        if (ip != NULL)
        {
            netif_set_ipaddr(netif_default, ip);
        }
        if (mask != NULL)
        {
            netif_set_netmask(netif_default, mask);
        }
        if (gw != NULL)
        {
            netif_set_gw(netif_default, gw);
        }
        printf("IP: %s\n",
        ip4addr_ntoa(netif_ip_addr4(netif_default)));
        printf("Mask: %s\n", 
        ip4addr_ntoa(netif_ip_netmask4(netif_default)));
        printf("Gateway: %s\n",
        ip4addr_ntoa(netif_ip_gw4(netif_default)));
        printf("Host Name: %s\n",
        netif_get_hostname(netif_default));
    }
    settings.result_fn = result_callback;
    settings.headers_done_fn = headers_callback;

    // setup the check schedule request 
    requests[Web_RequestType_CheckSchedule].type = Web_RequestType_CheckSchedule;
    requests[Web_RequestType_CheckSchedule].active = 0;
    strcpy(requests[Web_RequestType_CheckSchedule].body, "");
    strcpy(requests[Web_RequestType_CheckSchedule].headers, "");
    // setup the log delivery request
    requests[Web_RequestType_LogDelivery].type = Web_RequestType_LogDelivery;
    requests[Web_RequestType_LogDelivery].active = 0;
    strcpy(requests[Web_RequestType_LogDelivery].body, "");
    strcpy(requests[Web_RequestType_LogDelivery].headers, "");
    // setup the retrieve dose stats request
    requests[Web_RequestType_RetrieveDoseStats].type = Web_RequestType_RetrieveDoseStats;
    requests[Web_RequestType_RetrieveDoseStats].active = 0;
    strcpy(requests[Web_RequestType_RetrieveDoseStats].body, "");
    strcpy(requests[Web_RequestType_RetrieveDoseStats].headers, "");
    // setup the get user location request
    requests[Web_RequestType_RetrieveDoseStats].type = Web_RequestType_GetUserLocation;
    requests[Web_RequestType_RetrieveDoseStats].active = 0;
    strcpy(requests[Web_RequestType_RetrieveDoseStats].body, "");
    strcpy(requests[Web_RequestType_RetrieveDoseStats].headers, "");
    return status;
}

void web_request(char * uriParams, Web_RequestType type)
{
    char uri[2049];
    strcpy(uri, WEB_CLIENT_REQUEST_URL);
    if(!requests[type].active) {
        requests[type].active = 1;
        printf("\nMake request to: ");
        printf(strcat(uri, uriParams));
        err_t err = httpc_get_file_dns(
                WEB_CLIENT_SERVER,
                WEB_CLIENT_PORT,
                uri,
                &settings,
                body_callback,
                &type, //TODO: Pass a value to track this specific request
                NULL
            ); 
        
        //printf("status %d \n", err);
    }
}

void web_request_check_schedule(void)
{
    // build the url for checking schedule
    char url[2048] = "/check_schedule/device/";
    strcat(url, DEVICE_SERIAL);
    // make the request to the specified url
    web_request(url, Web_RequestType_CheckSchedule);
}

void web_request_retrieve_dose_stats(int schedule_id)
{
    // build up the url for the request
    char url[2048] = "/retrieve_dose_stats/schedule_id/";
    sprintf(url, "%s%i", url, schedule_id);
    // request the new dose amount
    web_request(url, Web_RequestType_RetrieveDoseStats);
}

void web_request_log_delivery(int schedule_id)
{
    // build the url for logging delivery
    char url[2048] = "/log_delivery/device/";
    strcat(url, DEVICE_SERIAL);
    strcat(url, "/schedule_id/");
    sprintf(url, "%s%i", url, schedule_id);
    // make the request to the specified url
    web_request(url, Web_RequestType_LogDelivery);
}

void web_request_get_user_location(void)//int user_id)
{
    // build the url for logging delivery
    char url[2048] = "/get_user_location/device/";
    strcat(url, DEVICE_SERIAL);
    // strcat(url, "/user_id/");
    // sprintf(url, "%s%i", url, user_id);
    // make the request to the specified url
    web_request(url, Web_RequestType_GetUserLocation);
}

int web_response_check_schedule(void)
{
    Web_RequestType type = Web_RequestType_CheckSchedule;
    if(requests[type].active && requests[type].complete) 
    {
        char userId[20] = "";       // "UserID:############"
        char scheduleId[24] = "";   // "ScheduleID:############"
        char * chunk;
        int userIdValue = -1;
        int scheduleIdValue = -1;

        // split out the userid and scheduleid property/value pairs
        strcpy(scheduleId, strchr(requests[type].body, PROPERTY_DELIM));
        strcpy(userId, strtok(requests[type].body, PROPERTY_DELIM_STR));
        
        // grab out the user id value from the property/value pair
        chunk = strchr(userId, VALUE_DELIM);
        ++chunk;
        userIdValue = atoi(chunk);

        // grab out the schedule id value from the property/value pair
        chunk = strchr(scheduleId, VALUE_DELIM);
        ++chunk;
        scheduleIdValue = atoi(chunk);

        // reset the request so a new one can be made
        requests[type].active = 0;
        strcpy(requests[type].headers, "");
        strcpy(requests[type].body, "");
        requests[type].complete = 0;

        // return the schedule id
        return scheduleIdValue;
    }
    return -1;
}

int web_response_retrieve_dose_stats(void)
{
    Web_RequestType type = Web_RequestType_RetrieveDoseStats;
    if(requests[type].active && requests[type].complete) 
    {
        return atoi(requests[type].body);
    }
    return -1;
}

struct DWM1001_Position web_response_get_user_location(void)
{
    struct DWM1001_Position position;
    position.x = 0;
    position.y = 0;
    position.z = 0;
    position.set = 0;

    Web_RequestType type = Web_RequestType_GetUserLocation;
    if(requests[type].active && requests[type].complete) 
    {
        char xCoord[10] = "";       // "x:#######"
        char yCoord[10] = "";       // "y:#######"
        char zCoord[10] = "";       // "y:#######"
        char * chunk;

        // split out the x/y/z key-value pairs
        strcpy(xCoord, strtok(requests[type].body, PROPERTY_DELIM_STR));
        strcpy(yCoord, strtok(NULL, PROPERTY_DELIM_STR));
        strcpy(zCoord, strtok(NULL, PROPERTY_DELIM_STR));
        printf(xCoord);
        printf(yCoord);
        printf(zCoord);
        
        // grab out the x value from the property/value pair
        chunk = strchr(xCoord, VALUE_DELIM);
        ++chunk;
        position.x = atof(chunk);

        // grab out the y value from the property/value pair
        chunk = strchr(yCoord, VALUE_DELIM);
        ++chunk;
        position.y = atof(chunk);

        // grab out the z value from the property/value pair
        chunk = strchr(zCoord, VALUE_DELIM);
        ++chunk;
        position.z = atof(chunk);

        // reset the request so a new one can be made
        requests[type].active = 0;
        strcpy(requests[type].headers, "");
        strcpy(requests[type].body, "");
        requests[type].complete = 0;
        position.set = 1;
    }

    return position;
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

void result_callback(void *arg, httpc_result_t httpc_result,
        u32_t rx_content_len, u32_t srv_res, err_t err)
{
    Web_RequestType type = *((Web_RequestType *)arg);
    // printf("\ntransfer complete\n");
    // printf("local result=%d\n", httpc_result);
    // printf("http result=%d\n", srv_res);
    // if the request was successful, mark this request as complete
    if(srv_res == 200) 
    {
        requests[type].complete = 1;
    }
    else
    {
        requests[type].active = 0;
        strcpy(requests[type].headers, "");
        strcpy(requests[type].body, "");
        requests[type].complete = 0;
    }
}

err_t headers_callback(httpc_state_t *connection, void *arg, 
    struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    Web_RequestType type = *((Web_RequestType *)arg);
    // printf("headers recieved\n");
    // printf("content length=%d\n", content_len);
    // printf("header length %d\n", hdr_len);
    pbuf_copy_partial(hdr, headerBuff, hdr->tot_len, 0);
    // printf("headers \n");
    // printf("%s", buff);
    // strcpy(requests[type].headers, buff);
    return ERR_OK;
}

err_t body_callback(void *arg, struct altcp_pcb *conn, 
                            struct pbuf *p, err_t err)
{
    Web_RequestType type = *((Web_RequestType *)arg);
    //printf("body\n");
    pbuf_copy_partial(p, bodyBuff, p->tot_len, 0);
    // printf("\nBUFF:\n%s", buff);
    strcpy(requests[type].body, bodyBuff);
    return ERR_OK;
}