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


/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

volatile char buff[1000];
httpc_connection_t settings;
const uint32_t COUNTRY = CYW43_COUNTRY_CANADA;
const uint32_t AUTH = CYW43_AUTH_WPA2_MIXED_PSK;

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
    while (status >= 0 && status != CYW43_LINK_UP)
    {
        int new_status = cyw43_tcpip_link_status(
             &cyw43_state,CYW43_ITF_STA);
        if (new_status != status)
        {
            status = new_status;
            flashrate = flashrate / (status + 1);
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
    return status;
}

void web_request(char * uriParams)
{
    printf("Make request to: ");
    printf(uriParams);
    printf(strcat(uriParams, WEB_CLIENT_REQUEST_URL));
    err_t err = httpc_get_file_dns(
            WEB_CLIENT_SERVER,
            WEB_CLIENT_PORT,
            strcat(uriParams, WEB_CLIENT_REQUEST_URL),
            &settings,
            body_callback,
            NULL, //TODO: Pass a value to track this specific request
            NULL
        ); 
    
    printf("status %d \n", err);
}

/************************************************************************/
/* Local  Implementation                                                */
/************************************************************************/

void result_callback(void *arg, httpc_result_t httpc_result,
        u32_t rx_content_len, u32_t srv_res, err_t err)
{
    printf("transfer complete\n");
    printf("local result=%d\n", httpc_result);
    printf("http result=%d\n", srv_res);
}

err_t headers_callback(httpc_state_t *connection, void *arg, 
    struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    printf("headers recieved\n");
    printf("content length=%d\n", content_len);
    printf("header length %d\n", hdr_len);
    pbuf_copy_partial(hdr, buff, hdr->tot_len, 0);
    printf("headers \n");
    printf("%s", buff);
    return ERR_OK;
}

err_t body_callback(void *arg, struct altcp_pcb *conn, 
                            struct pbuf *p, err_t err)
{
    printf("body\n");
    pbuf_copy_partial(p, buff, p->tot_len, 0);
    printf("%s", buff);
    return ERR_OK;
}