#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "keyPad.h"

#define	FALSE		0
#define	TRUE		1

#define EXAMPLE_ESP_WIFI_SSID      "HUAWEI-Ganesh"
#define EXAMPLE_ESP_WIFI_PASS      "ganesh234"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;

#define NUM_ROWS	5
#define NUM_COLS	4

#define PIN_ROW_1	GPIO_NUM_16
#define PIN_ROW_2	GPIO_NUM_17
#define PIN_ROW_3	GPIO_NUM_5
#define PIN_ROW_4	GPIO_NUM_18
#define PIN_ROW_5	GPIO_NUM_19

#define PIN_COL_1 	GPIO_NUM_15
#define PIN_COL_2 	GPIO_NUM_2
#define PIN_COL_3 	GPIO_NUM_0
#define PIN_COL_4 	GPIO_NUM_4

uint16_t rowPin[NUM_ROWS] = {PIN_ROW_5,PIN_ROW_4,PIN_ROW_3,PIN_ROW_2,PIN_ROW_1};
uint16_t colPin[NUM_COLS] = {PIN_COL_1,PIN_COL_2,PIN_COL_3,PIN_COL_4};

static const uint8_t keyMatch[NUM_ROWS][NUM_COLS] = {
														{0x01,0x02,0x03,0x04},
														{0x05,0x06,0x07,0x08},
														{0x09,0x0A,0x0B,0x0C},
														{0x0D,0x0E,0x0F,0x10},
														{0x11,0x12,0x13,0x14}
													};

static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
        esp_wifi_connect();
    }
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
		{
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
		else
		{
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void delayMs(const TickType_t mSec)
{
	vTaskDelay(mSec / portTICK_RATE_MS);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,&instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,&instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,pdFALSE,pdFALSE,portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT)
	{
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
	else if (bits & WIFI_FAIL_BIT)
	{
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
	else
	{
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

int kGpioInit(void)
{
	// considering rows, hence o/p mode
	gpio_config_t rConf,cConf;

	//Row Pins
	rConf.pin_bit_mask 	= ( (1ULL << PIN_ROW_1) | (1ULL << PIN_ROW_2) | (1ULL << PIN_ROW_3) | (1ULL << PIN_ROW_4) | (1ULL << PIN_ROW_5) );
	rConf.mode  		= GPIO_MODE_OUTPUT;
	rConf.pull_up_en 	= GPIO_PULLUP_DISABLE;
	rConf.pull_down_en 	= GPIO_PULLDOWN_DISABLE;
	rConf.intr_type 	= GPIO_INTR_DISABLE;
	gpio_config(&rConf);
	gpio_set_level((PIN_ROW_1 | PIN_ROW_2 | PIN_ROW_3 | PIN_ROW_4 | PIN_ROW_5),1);

	//Col Pins
	cConf.pin_bit_mask 	= ( (1ULL << PIN_COL_1) | (1ULL << PIN_COL_2) | (1ULL << PIN_COL_3) | (1ULL << PIN_COL_4));
	cConf.mode  		= GPIO_MODE_INPUT;
	cConf.pull_up_en 	= GPIO_PULLUP_ENABLE; //pull up enable
	cConf.pull_down_en 	= GPIO_PULLDOWN_DISABLE;
	cConf.intr_type 	= GPIO_INTR_DISABLE;
	gpio_config(&cConf);

	ESP_LOGI(TAG,"GPIO Keypad Initialize Success !");

	return 0;
}

uint8_t ctrlGpio(int16_t pin,uint8_t rowOrCol,uint8_t state,uint8_t action)
{
	uint8_t s = state;

	if(action == GPIO_SET)
	{
		if(rowOrCol == ROW_PIN)
			gpio_set_level(rowPin[pin],state);
		else //if(rowOrCol == COL_PIN)
			gpio_set_level(colPin[pin],state);
	}
	else //if(action == GPIO_GET)
	{
		if(rowOrCol == ROW_PIN)
			s = gpio_get_level(rowPin[pin]);
		else //if(rowOrCol == COL_PIN)
			s = gpio_get_level(colPin[pin]);
	}

	return s;
}

void app_main(void)
{
	uint8_t val = 0;
	int retn = -2;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
    wifi_init_sta();

	retn = setKeypadInfo(NUM_ROWS,NUM_COLS,(const uint8_t *)keyMatch,kGpioInit,ctrlGpio);
	keyPadInit();

	/* Live Loop */
	while(TRUE)
	{
		val = readKey();
		if(val != GPIO_NON)
			ESP_LOGI(TAG, "Key Pressed: 0x%02X\n", val);
		delayMs(500);
/* 		gpio_set_level(STATUS_LED,0);
		delayMs(600);
		gpio_set_level(STATUS_LED,1);
		delayMs(600); */
	}
}
