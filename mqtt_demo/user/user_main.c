/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "user_config.h"
#include "gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "espressif/espconn.h"
#include "espressif/airkiss.h"
#include "mqtt/MQTTClient.h"
#include "key.h"
#include "uart.h"


LOCAL uint8 device_status;
enum {
    DEVICE_GOT_IP=39,
    DEVICE_CONNECTING,
    DEVICE_ACTIVE_DONE,
    DEVICE_ACTIVE_FAIL,
    DEVICE_CONNECT_SERVER_FAIL
};

extern send_bz;
extern send_delay;
extern topicFilter;
extern retain;
extern payload[30];
int val= 0;//LED OFF
int val1= 0;
int val2= 0;
extern MQTTPub(const char *topicFilter, char *payload, int retain, int send_delay);
int key1=0;
int key2=0;
int key3=0;
int cnt_pub=0;
#define PLUG_KEY_NUM            3
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[PLUG_KEY_NUM];
LOCAL struct led_saved_param led_param;
LOCAL struct led1_saved_param led1_param;
LOCAL struct led2_saved_param led2_param;



void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
 switch(status) {
    case SC_STATUS_WAIT:
      os_printf("SC_STATUS_WAIT\n");
      break;
    case SC_STATUS_FIND_CHANNEL:
      os_printf("SC_STATUS_FIND_CHANNEL\n");
      break;
    case SC_STATUS_GETTING_SSID_PSWD:
      os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
      sc_type *type = pdata;
      if (*type == SC_TYPE_ESPTOUCH) {
        os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
      } else {
        os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
      }
      break;
    case SC_STATUS_LINK:
      os_printf("SC_STATUS_LINK\n");
      struct station_config *sta_conf = pdata;
      os_printf("ssidhhhh   %s\n",sta_conf->ssid);
      os_printf("ssidhhhh   %s\n",sta_conf->password);

      user_conn_init();
      wifi_station_set_config(sta_conf);
      wifi_station_disconnect();
      wifi_station_connect();
      break;
    case SC_STATUS_LINK_OVER:
      os_printf("SC_STATUS_LINK_OVER\n");
      if (pdata != NULL) {
        uint8 phone_ip[4] = {0};
        memcpy(phone_ip, (uint8*)pdata, 4);
        os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
      }
      smartconfig_stop();

      device_status = DEVICE_GOT_IP;

      break;
      }
 }


void ICACHE_FLASH_ATTR
smartconfig_task(void *pvParameters)
{
    smartconfig_start(smartconfig_done);
    vTaskDelete(NULL);
}

void
user_esp_platform_set_connect_status(uint8 status)
{
    device_status = status;
}

enum{
    AP_DISCONNECTED = 0,
    AP_CONNECTED,
    DNS_SUCESSES,
    DNS_FAIL,
};
uint8
user_esp_platform_get_connect_status(void)
{
    uint8 status = wifi_station_get_connect_status();

    if (status == STATION_GOT_IP) {
        status = (device_status == 0) ? DEVICE_CONNECTING : device_status;
    }

    os_printf("status %d\n", status);
    return status;
}

LOCAL BOOL
user_esp_platform_check_conection(void)
{
    u8 single_ap_retry_count = 0;
    u8 dns_retry_count = 0;
    u8 dns_ap_retry_count= 0;

    struct ip_info ipconfig;
    struct hostent* phostent = NULL;

    memset(&ipconfig, 0, sizeof(ipconfig));
    wifi_get_ip_info(STATION_IF, &ipconfig);


    struct station_config *sta_config = (struct station_config *)zalloc(sizeof(struct station_config)*5);
    int ap_num = wifi_station_get_ap_info(sta_config);
    free(sta_config);

#define MAX_DNS_RETRY_CNT 20
#define MAX_AP_RETRY_CNT 200

#ifdef USE_DNS

    do{
        //what if the ap connect well but no ip offer, wait and wait again, time could be more longer
        if(dns_retry_count == MAX_DNS_RETRY_CNT){
            if(ap_num >= 2) user_esp_platform_ap_change();
        }
        dns_retry_count = 0;

        while( (ipconfig.ip.addr == 0 || wifi_station_get_connect_status() != STATION_GOT_IP)){

            /* if there are wrong while connecting to some AP, change to next and reset counter */
            if (wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                    wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                    wifi_station_get_connect_status() == STATION_CONNECT_FAIL ||
                    (single_ap_retry_count++ > MAX_AP_RETRY_CNT)) {
                if(ap_num >= 2){
                    ESP_DBG("try other APs ...\n");
                    user_esp_platform_ap_change();
                }
                ESP_DBG("connecting...\n");
                single_ap_retry_count = 0;
            }

            vTaskDelay(3000/portTICK_RATE_MS);
            wifi_get_ip_info(STATION_IF, &ipconfig);
        }

        //***************************
#if LIGHT_DEVICE
            user_mdns_conf();
#endif
        //***************************

        do{
            ESP_DBG("STA trying to parse esp domain name!\n");
            phostent = (struct hostent *)gethostbyname(ESP_DOMAIN);

            if(phostent == NULL){
                vTaskDelay(500/portTICK_RATE_MS);
            }else{
                printf("Get DNS OK!\n");
                break;
            }

        }while(dns_retry_count++ < MAX_DNS_RETRY_CNT);

    }while(NULL == phostent && dns_ap_retry_count++ < AP_CACHE_NUMBER);

    if(phostent!=NULL){

        if( phostent->h_length <= 4 )
            memcpy(&esp_server_ip,(char*)(phostent->h_addr_list[0]),phostent->h_length);
        else
            os_printf("ERR:arr_overflow,%u,%d\n",__LINE__, phostent->h_length );


        printf("ESP_DOMAIN IP address: %s\n", inet_ntoa(esp_server_ip));

        ping_status = TRUE;
        free(phostent);
        phostent == NULL;

        return DNS_SUCESSES;
    } else {
        return DNS_FAIL;
    }

#else

    while( (ipconfig.ip.addr == 0 || wifi_station_get_connect_status() != STATION_GOT_IP)){

        os_printf("STA trying to connect AP!\n");
        /* if there are wrong while connecting to some AP, change to next and reset counter */
        if (wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL ||
                (single_ap_retry_count++ > MAX_AP_RETRY_CNT)) {

            user_esp_platform_ap_change();
            single_ap_retry_count = 0;
        }

        vTaskDelay(300);
        wifi_get_ip_info(STATION_IF, &ipconfig);
    }

    //memcpy(&esp_server_ip, esp_domain_ip, 4);
    //os_printf("ESP_DOMAIN IP address: %s\n", inet_ntoa(esp_server_ip));

#endif

    if(ipconfig.ip.addr != 0){
        return AP_CONNECTED;
    }else{
        return AP_DISCONNECTED;
    }
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
//void wifi_event_handler_cb(System_Event_t *event)
//{
//    if (event == NULL) {
//        return;
//    }
//
//    switch (event->event_id) {
//        case EVENT_STAMODE_GOT_IP:
//            os_printf("sta got ip ,create task and free heap size is %d\n", system_get_free_heap_size());
//            user_conn_init();
//            break;
//
//        case EVENT_STAMODE_CONNECTED:
//            os_printf("sta connected\n");
//            break;
//
//        case EVENT_STAMODE_DISCONNECTED:
//            wifi_station_connect();
//            break;
//
//        default:
//            break;
//    }
//}


uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
/**********************************SAMPLE CODE*****************************/
/*void io_intr_handler(void *args)
{
    uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);          //READ STATUS OF INTERRUPT
    if (status & BUTTON_IO_PIN) //KIEM TRA XEM NGAT XUAT PHAT TU GPIO13 KHONG
    {
        if (val == 0) {
            GPIO_OUTPUT_SET(LED_IO_NUM, 1);
            MQTTPub("cmnd/109/abcdkeke2/1/led","ON",0,1);
            gpio16_output_set(0);
            val = 1;
        } else {
            GPIO_OUTPUT_SET(LED_IO_NUM, 0);
            MQTTPub("cmnd/109/abcdkeke2/1/led","OFF",0,1);
            gpio16_output_set(1);
            val = 0;
        }
        WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(status & 1));
    }
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);       //CLEAR THE STATUS IN THE W1 INTERRUPT REGISTER

}
static void Handle_Button(void *pvParameters)
{
	GPIO_ConfigTypeDef io_out_conf;
	io_out_conf.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;    //disable interrupt
	io_out_conf.GPIO_Mode = GPIO_Mode_Output;             //set as output mode
	io_out_conf.GPIO_Pin = LED_IO_PIN;                    //bit mask of the pins that you want to set
	io_out_conf.GPIO_Pullup = GPIO_PullUp_DIS;            //disable pull-up mode
	gpio_config(&io_out_conf);

	printf("SETUP GPIO13 INTERRUPT CONFIGURE..\r\n");
	GPIO_ConfigTypeDef io_in_conf;
	io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE; //2    //enable interrupt
	io_in_conf.GPIO_Mode = GPIO_Mode_Input; //0             //set as input mode
	io_in_conf.GPIO_Pin = BUTTON_IO_PIN;
    io_in_conf.GPIO_Pullup = GPIO_PullUp_EN; //1           //enable pull-up mode
	gpio_config(&io_in_conf);
//	gpio_config(&B2);

	gpio_intr_handler_register(io_intr_handler, NULL);
//	gpio_intr_handler_register(io_intr_button2, NULL);

//	gpio16_output_set(1);
	ETS_GPIO_INTR_ENABLE();
	for(;;)
	{
	}
}*/


void led_setup(){
	GPIO_OUTPUT_SET(LED_IO_NUM, 0);
	GPIO_OUTPUT_SET(LED_IO2_NUM, 0);
}

uint8
user_led_get_status(void)
{
    return led_param.status;
}
uint8
user_led1_get_status(void)
{
    return led1_param.status1;
}
uint8
user_led2_get_status(void)
{
    return led2_param.status2;
}



void
user_led_set_status(bool status)
{
    if (status != led_param.status) {
        if (status > 1) {
            printf("error status input!\n");
            return;
        }


        if(val==0 && val2==0){
        	val=1;
        	//printf("val=1\n");
        	MQTTPub("state/109/abcdkeke2/1/led","1",0,1);
        	GPIO_OUTPUT_SET(LED_IO_NUM, 1);
        	os_printf("Bat led1\n");
        }
        else if(val==1){
        	val=0;
        		//printf("val=0\n");
//        		MQTTPub("state/109/abcdkeke2/1/led","0",0,1);
//        		GPIO_OUTPUT_SET(LED_IO_NUM, 0);
//        		os_printf("Tat led1\n");
        }
        else if(val2==1 && val==0){
        	val2=0;
        	led_setup();
        	GPIO_OUTPUT_SET(LED_IO_NUM, 1);
        	os_printf("Bat led1\n");
        }
        led_param.status = status;
        key1=1;
        //printf("status input! %d\n", status);
    }
}

void
user_led1_set_status(bool status1)
{
    if (status1 != led1_param.status1) {
        if (status1 > 1) {
            printf("error status input!\n");
            return;
        }
    GPIO_OUTPUT_SET(LED_IO_NUM, 0);
    GPIO_OUTPUT_SET(LED_IO2_NUM, 0);
//        if(val1==0){
//        		val1=1;
//        		//printf("val1=1\n");
//        		MQTTPub("state/109/abcdkeke2/2/led","1",0,1);
//        		GPIO_OUTPUT_SET(LED_IO2_NUM, 1);
//        		os_printf("Bat led2\n");
//        }
//        else if(val1==1){
//        		val1=0;
//        		//printf("val1=0\n");
//        		MQTTPub("state/109/abcdkeke2/2/led","0",0,1);
//        		GPIO_OUTPUT_SET(LED_IO2_NUM, 0);
//        		os_printf("Tat led2\n");
//        }
        //printf("status input! %d\n", status1);
        led1_param.status1 = status1;
        key2=1;

    }
}


void
user_led2_set_status(bool status2)
{
    if (status2 != led2_param.status2) {
        if (status2 > 1) {
            printf("error status input!\n");
            return;
        }
        if(val2==0 && val==0){
        		val2=1;
        		//printf("val2=1\n");
        		MQTTPub("state/109/abcdkeke2/3/led","1",0,1);
        		GPIO_OUTPUT_SET(LED_IO1_NUM, 1);
        		os_printf("Bat led3\n");
        }
        else if( val2==1){
        		val2=0;
        		//printf("val2=0\n");
//        		MQTTPub("state/109/abcdkeke2/3/led","0",0,1);
//        		GPIO_OUTPUT_SET(LED_IO1_NUM, 0);
//        		os_printf("Tat led3\n");
        }
        else if(val==1&&val2==0){
        	val2=0;
        	led_setup();
        	GPIO_OUTPUT_SET(LED_IO1_NUM, 1);
        	os_printf("Bat led3\n");
        }
        //printf("status input! %d\n", status2);
        led2_param.status2 = status2;
        key3=1;

    }
}

void user_button_long_press(void){
	 system_restart();

}
LOCAL void
user_button_short_press(void)
{


    user_led_set_status((~led_param.status) & 0x01);
//    spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
//    spi_flash_write((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
//                (uint32 *)&led_param, sizeof(struct led_saved_param));
}
void user_button1_long_press(void){
	system_restart();
}
void user_button1_short_press(void){
	user_led2_set_status((~led2_param.status2) & 0x01);
}
void user_button2_long_press(void){
	system_restart();
}
void  user_button2_short_press(void){
	user_led1_set_status((~led1_param.status1) & 0x01);

}



LOCAL void
user_link_led_init(void)
{
    PIN_FUNC_SELECT(LED_IO_MUX, LED_IO_FUNC);
    PIN_FUNC_SELECT(LED_IO1_MUX, LED_IO1_FUNC);
    PIN_FUNC_SELECT(LED_IO2_MUX, LED_IO2_FUNC);
}



BOOL
user_get_key_status(void)
{
    return get_key_status(single_key[0]);
}
BOOL
user_get_key_status1(void)
{
    return get_key_status(single_key[1]);
}
BOOL
user_get_key_status2(void)
{
    return get_key_status(single_key[2]);
}

//LOCAL void
//user_link_led_timer_cb(void)
//{
//    link_led_level = (~link_led_level) & 0x01;
//    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_IO_NUM), link_led_level);
//}
//
//void
//user_link_led_timer_init(int time)
//{
//    os_timer_disarm(&link_led_timer);
//    os_timer_setfn(&link_led_timer, (os_timer_func_t *)user_link_led_timer_cb, NULL);
//    os_timer_arm(&link_led_timer, time, 1);
//    link_led_level = 0;
//    GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_IO_NUM), link_led_level);
//}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());

    /* need to set opmode before you set config */
    wifi_set_opmode(STATION_MODE);
    user_link_led_init();
    //wifi_station_set_auto_connect(1);
//    wifi_station_get_config();
//    wifi_station_set_config();
//    wifi_station_connect();

    //printf(" aaaaaa   %s\n",device_status);
    xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);
    //wifi_status_led_install(LED_IO3_NUM, LED_IO3_MUX, LED_IO3_FUNC);
    //xTaskCreate(Handle_Button,"Handle_Button",256,NULL,2,NULL);
//    xTaskCreate(Blynk_led,"Blynk_led",256,NULL,3,NULL);
    single_key[0] = key_init_single(BUTTON_IO_NUM, BUTTON_IO_MUX, BUTTON_IO_FUNC,user_button_long_press, user_button_short_press);
    single_key[1] = key_init_single(BUTTON_IO1_NUM, BUTTON_IO1_MUX, BUTTON_IO1_FUNC,user_button1_long_press, user_button1_short_press);
    single_key[2] = key_init_single(BUTTON_IO2_NUM, BUTTON_IO2_MUX, BUTTON_IO2_FUNC,user_button2_long_press, user_button2_short_press);
    keys.key_num = PLUG_KEY_NUM;
    keys.single_key = single_key;
    key_init(&keys);



//    printf("THIS IS A UART LOOPBACK DEMO...\n");
//        uart_init_new();    //UART0 Initialize
//        while (1) {
//            uint8 alpha = 'A';
//            char* test_str = "Your String Here\r\n";
//            uart0_tx_buffer(&alpha, sizeof(alpha));
//            uart0_tx_buffer(test_str, strlen(test_str));
//            break;
//        }

//    struct station_config config;
//    bzero(&config, sizeof(struct station_config));
//    sprintf(config.ssid, SSID);
//    sprintf(config.password, PASSWORD);
//    wifi_station_set_config(&config);
//
//    wifi_set_event_handler_cb(wifi_event_handler_cb);
//
//    wifi_station_connect();
//    PIN_FUNC_SELECT(LED_IO_MUX,LED_IO_FUNC);

        // default to be off, for safety.
//        if (led_param.status == 0xff) {
//            led_param.status = 0;
//        }

//        LED_STATUS_OUTPUT(LED_IO_NUM, led_param.status);


}


