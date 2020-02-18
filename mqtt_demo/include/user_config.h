/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__


//#define SSID         "GratiotDemo"        /* Wi-Fi SSID */
//#define PASSWORD     "zaq12345**"     /* Wi-Fi Password */
#define server_ip "192.168.101.142"
#define server_port 9669


#define DEVICE_TYPE 		"gh_9e2cff3dfa51" //wechat public number
#define DEVICE_ID 			"122475" //model ID

#define DEFAULT_LAN_PORT 	12476

#define MQTT_BROKER  "broker.hivemq.com"  /* MQTT Broker Address*/
#define MQTT_PORT    1883             /* MQTT Port*/

#define ETS_GPIO_INTR_ENABLE()  _xt_isr_unmask(1 << ETS_GPIO_INUM)  //ENABLE INTERRUPTS
#define ETS_GPIO_INTR_DISABLE() _xt_isr_mask(1 << ETS_GPIO_INUM)    //DISABLE INTERRUPTS

//GPIO OUTPUT SETTINGS
#define  LED_IO_MUX  PERIPHS_IO_MUX_MTDO_U
#define  LED_IO_NUM  15
#define  LED_IO_FUNC FUNC_GPIO15
#define  LED_IO_PIN  GPIO_Pin_15

#define  LED_IO1_MUX  PERIPHS_IO_MUX_GPIO4_U
#define  LED_IO1_NUM  4
#define  LED_IO1_FUNC FUNC_GPIO4
#define  LED_IO1_PIN  GPIO_Pin_4

#define  LED_IO2_MUX  PERIPHS_IO_MUX_GPIO5_U
#define  LED_IO2_NUM  5
#define  LED_IO2_FUNC FUNC_GPIO5
#define  LED_IO2_PIN  GPIO_Pin_5





//GPIO INPUT SETTINGS
#define  BUTTON_IO1_MUX  PERIPHS_IO_MUX_MTCK_U
#define  BUTTON_IO1_NUM   13
#define  BUTTON_IO1_FUNC  FUNC_GPIO13
#define  BUTTON_IO1_PIN   GPIO_Pin_13

#define  BUTTON_IO2_MUX  PERIPHS_IO_MUX_MTMS_U
#define  BUTTON_IO2_NUM  14
#define  BUTTON_IO2_FUNC FUNC_GPIO14
#define  BUTTON_IO2_PIN  GPIO_Pin_14

#define  BUTTON_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define  BUTTON_IO_NUM  12
#define  BUTTON_IO_FUNC FUNC_GPIO12
#define  BUTTON_IO_PIN GPIO_Pin_12

//void user_plug_init(void);
//uint8 user_led_get_status(void);
//uint8 user_led1_get_status(void);
//uint8 user_led2_get_status(void);
//void user_led_set_status(bool status);
//void user_led1_set_status(bool status1);
//void user_led2_set_status(bool status2);
struct led_saved_param {
    uint8_t status;
    uint8_t pad[3];
};
struct led1_saved_param {
    uint8_t status1;
    uint8_t pad[3];
};
struct led2_saved_param {
    uint8_t status2;
    uint8_t pad[3];
};

#endif

