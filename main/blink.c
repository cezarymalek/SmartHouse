/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <lwip/sockets.h>
#include <esp_log.h>
#include <errno.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_tls.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
int mode = 5000;
static const char *TAG = "simple wifi";

#define EXAMPLE_ESP_WIFI_MODE_AP   FALSE //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_WIFI_SSID      "CNE30"
#define EXAMPLE_ESP_WIFI_PASS      "z6ugvzx5k7quirfe"

#define PORT_NUMBER 8001

int gpioNumber = 2;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;

}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_i nit_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void blink_task(void *pvParameter)
{

    gpio_pad_select_gpio(gpioNumber);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(gpioNumber, GPIO_MODE_OUTPUT);
    while(1)
    {

        /* Blink off (output low) */
        gpio_set_level(gpioNumber, 0);
        vTaskDelay(mode / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(gpioNumber, 1);
        vTaskDelay(mode / portTICK_PERIOD_MS);
    }
}

static char tag[] = "socket_server";

void yell_task(void *pvParameter)
{
    struct sockaddr_in clientAddress;
    struct sockaddr_in serverAddress;

    // Create a socket that we will listen upon.
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(tag, "socket: %d %s", sock, strerror(errno));
        goto END;
    }

    // Bind our server socket to a port.
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT_NUMBER);
    int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (rc < 0) {
        ESP_LOGE(tag, "bind: %d %s", rc, strerror(errno));
        goto END;
    }

    // Flag the socket as listening for new connections.
    rc = listen(sock, 5);
    if (rc < 0) {
        ESP_LOGE(tag, "listen: %d %s", rc, strerror(errno));
        goto END;
    }
    printf("SŁUCHAM\n");
    while (1) {
        // Listen for a new client connection.
        socklen_t clientAddressLength = sizeof(clientAddress);
        printf("CZEKAM NA KLIENTA\n");
        int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
        printf("MAM KLIENTA\n");
        if (clientSock < 0) {
            ESP_LOGE(tag, "accept: %d %s", clientSock, strerror(errno));
            goto END;
        }

        // We now have a new client ...
        int total =	1024;
        int sizeUsed = 0;
        char* data = malloc(total);

        // Loop reading data.
        while(1) {

            ssize_t sizeRead = recv(clientSock, data , total, 0);
            if (sizeRead < 0) {
                ESP_LOGE(tag, "recv: %d %s", sizeRead, strerror(errno));
                goto END;
            }
            if (sizeRead == 0) {
                break;
            }
            printf("Data:%s\n",data);

            //char* substr = malloc();
            //strncpy(substr, buff+10, 4);
            //if(data[0]=='p')mode=atoi(data);
            //if()

            gpioNumber=atoi(data);
            gpio_pad_select_gpio(gpioNumber);
            /* Set the GPIO as a push/pull output */
            gpio_set_direction(gpioNumber, GPIO_MODE_OUTPUT);



          }

        // Finished reading data.
        ESP_LOGD(tag, "Data read (size: %d) was: %.*s", sizeUsed, sizeUsed, data);
        free(data);
        close(clientSock);
    }
    END:
    vTaskDelete(NULL);
    //uxTaskGetStackHighWaterMark(NULL)
}

void app_main()
{
    //xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    xTaskCreate(&yell_task, "yell_task", 8192, NULL, 5, NULL);
    xTaskCreate(&blink_task, "blink_task", 8192, NULL, 5, NULL);
}
