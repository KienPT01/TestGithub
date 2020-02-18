#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mqtt/MQTTClient.h"
#include "gpio.h"
#include "user_config.h"
#include "c_types.h"
#include "esp_common.h"

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  2048
#define MQTT_CLIENT_THREAD_PRIO         8

int send_bz;
int send_delay;
const char* topicFilter;
int retain;
char *payload[30];
extern val; //LED OFF
extern val1;
extern val2;
extern key1;
extern key2;
extern key3;

int retry=0;
MQTTMessage message;
MQTTClient client;
Network network;

int split (const char *txt, char delim, char ***tokens)
{
    int *tklen, *t, count = 1;
    char **arr, *p = (char *) txt;

    while (*p != '\0') if (*p++ == delim) count += 1;
    t = tklen = calloc (count, sizeof (int));
    for (p = (char *) txt; *p != '\0'; p++) *p == delim ? *t++ : (*t)++;
    *tokens = arr = malloc (count * sizeof (char *));//cap phat bo nho yeu cau
    t = tklen;
    p = *arr++ = calloc (*(t++) + 1, sizeof (char *));
    while (*txt != '\0')
    {
        if (*txt == delim)
        {
            p = *arr++ = calloc (*(t++) + 1, sizeof (char *));
            txt++;
        }
        else *p++ = *txt++;
    }
    free (tklen);
    return count;
}

LOCAL xTaskHandle mqttc_client_handle;
static void messageArrived(MessageData* data)
{

  char topicBuf[25];
  char payload[25];
  uint16 data_len = data->message->payloadlen;
  char A[25]  = "ON|On|on|BAT|bat|Bat";
  char B[25]  = "OFF|Off|off|TAT|Tat|tat";


  strlcpy(topicBuf, data->topicName->lenstring.data,data->topicName->lenstring.len + 1);
  strlcpy(payload, data->message->payload,data_len + 1);
  char **tokens;
  char **msg;
  int count, i,cnt;
  const char *str=topicBuf;
  const char *str2=payload;

  count = split (str, '/', &tokens);
  cnt=split(str2,'|',&msg);
  for (i = 0; i < count; i++) free (tokens[i]);
  free (tokens);

  /*count2 = split(str2,'/',&tokens2);
  for (i = 0; i < count2; i++) free (tokens2[i]);
  free (tokens2);*/

  printf("%s\n",topicBuf);
  printf("Message Arrived on topic is: %s %s %s\n",tokens[4],tokens[3],msg[0]);
 // printf("Sent by: %s\n",mng[1]);
  if(strcmp(tokens[4],"led")==0 && strcmp(tokens[3],"1")==0)
      {
      	if(val==0  && strstr(A,msg[0]))
          {
      		GPIO_OUTPUT_SET(LED_IO_NUM, 1);
      		val=1;
      		os_printf("Bat led1\n");
          }
      	if(val==1  && strstr(B,msg[0]))
          {
          	GPIO_OUTPUT_SET(LED_IO_NUM, 0);
      		val=0;
      		os_printf("Tat led1\n");
          }
      	os_printf("Val                        : [%d] \n",val);
      }
   else if(strcmp(tokens[4],"led")==0 && strcmp(tokens[3],"2")==0)
      {
          if(val1==0 && strstr(A,msg[0]))
          {
          	GPIO_OUTPUT_SET(LED_IO1_NUM, 1);
          	val1=1;
          	os_printf("Bat led2\n");
          }
          if(val1==1 && strstr(B,msg[0]))
          {
              GPIO_OUTPUT_SET(LED_IO1_NUM, 0);
          	val1=0;
          	os_printf("Tat led2\n");
          }
      	os_printf("Val1                       : [%d]\n",val1);
      }
   else if(strcmp(tokens[4],"led")==0 && strcmp(tokens[3],"3")==0)
      {
          if(val2==0 &&  strstr(A,msg[0]))
          {
          	GPIO_OUTPUT_SET(LED_IO2_NUM, 1);
          	val2=1;
          	os_printf("Bat led3\n");
          }
          if(val2==1  && strstr(B,msg[0]))
          {
              GPIO_OUTPUT_SET(LED_IO2_NUM, 0);
          	val2=0;
          	os_printf("Tat led3\n");
          }
      	os_printf("Val2                       : [%d]\n",val2);
       }
    else os_printf("Error!!!\n");
  }


static void mqtt_client_thread(void* pvParameters)
{
    printf("mqtt client thread starts\n");
    unsigned char sendbuf[80], readbuf[80] = {0};
    int rc = 0, count = 0;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    pvParameters = 0;
    NetworkInit(&network);
    MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

    char* address = MQTT_BROKER;



    if ((rc = NetworkConnect(&network, address, MQTT_PORT)) != 0) {
        printf("Return code from network connect is %d\n", rc);
    }

#if defined(MQTT_TASK)

    if ((rc = MQTTStartTask(&client)) != pdPASS) {
        printf("Return code from start tasks is %d\n", rc);
    } else {
        printf("Use MQTTStartTask\n");
    }

#endif

    startconnect:
           os_printf("AAAAA \n");
           rc = NetworkConnect(&network, address, MQTT_PORT);
           printf("rc==== %d\n",rc);
    connectData.MQTTVersion = 3;
    connectData.clientID.cstring = "ESP8266_sample";

    if ((rc = MQTTConnect(&client, &connectData)) != 0) {
        printf("Return code from MQTT connect is %d\n", rc);
    } else {
        printf("MQTT Connected\n");

    }

    if ((rc = MQTTSubscribe(&client, "cmnd/109/abcdkeke2/1/led", 0, messageArrived)) != 0) {
        printf("Return code from MQTT subscribe is %d\n", rc);
    } else {
        printf("MQTT subscribe to topic \"cmnd/109/abcdkeke2/1/led\"\n");
        //printf("%d isrc1\n",rc);
    }

    if ((rc = MQTTSubscribe(&client, "cmnd/109/abcdkeke2/2/led", 0, messageArrived)) != 0) {
            printf("Return code from MQTT subscribe is %d\n", rc);
    } else {
            printf("MQTT subscribe to topic \"cmnd/109/abcdkeke2/2/led\"\n");
            //printf("%d isrc2\n",rc);
    }

    if ((rc = MQTTSubscribe(&client, "cmnd/109/abcdkeke2/3/led", 0, messageArrived)) != 0) {
            printf("Return code from MQTT subscribe is %d\n", rc);
    } else {
            printf("MQTT subscribe to topic \"cmnd/109/abcdkeke2/3/led\"\n");
            //printf("%d isrc3\n",rc);
    }
    while(1)
    {
    	  rc = MQTTIsConnected(&client);
    	         //        os_printf("MQTTconnect %d, retry count %d\n",rc, count);

    	                  if (!rc){
    	                    retry++;
    	                    //os_printf("%d\n",retry);
    	                    //led_int(1, 700);

    	                    os_printf("MQTT is disconnect %d  %d\n", rc, retry);

    	                  } else{
    	                    retry =0;
    	                   // led_int(0, 0);
    	                  }
    	                  if (retry >  100) {

    	                    os_printf("disconnect\n");

    	                    // bz=0;
    	                    MQTTDisconnect(&client);
    	//                    myConfig.flag.mqtt_connected = 0;
    	//                    lwt = 1;
    	                    retry =0;
    	                    vTaskDelay(100);
    	                    goto startconnect;
    	                  }
      vTaskDelay(1);
      if(send_bz>1)
      {
        send_bz--;
      }
      if(send_bz)
        {
          if(key1==1){
        	  key1=0;
        	  if ((rc = MQTTPublish(&client,"state/109/abcdkeke2/1/led", &message)) == 0)
        	            {
        	              //os_printf("message.payloadlen=%d,message.payload=%s\n",message.payloadlen,message.payload);
        	              send_bz=0;
        	              os_printf("1success\n");
        	            }
        	  else
        	            {
        	              send_bz=0;
        	              os_printf("fail\n");
        	            }
          }

          if(key2==1){
                  	  key2=0;
                  	  if ((rc = MQTTPublish(&client,"state/109/abcdkeke2/2/led", &message)) == 0)
                  	            {
                  	              //os_printf("message.payloadlen=%d,message.payload=%s\n",message.payloadlen,message.payload);
                  	              send_bz=0;
                  	              os_printf("2success\n");
                  	            }
                  	  else
                  	            {
                  	              send_bz=0;
                  	              os_printf("fail\n");
                  	            }
                    }

          if(key3==1){
                  	  key3=0;
                  	  if ((rc = MQTTPublish(&client,"state/109/abcdkeke2/3/led", &message)) == 0)
                  	            {
                  	              //os_printf("message.payloadlen=%d,message.payload=%s\n",message.payloadlen,message.payload);
                  	              send_bz=0;
                  	              os_printf("3success\n");
                  	            }
                  	  else
                  	            {
                  	              send_bz=0;
                  	              os_printf("fail\n");
                  	            }
                    }




        }
    }
    printf("mqtt_client_thread going to be deleted\n");
    vTaskDelete(NULL);
    return;
}
/****************************************************************************/
void MQTTPub(const char *topicFilter, char *payload, int retain, int send_delay)
{
  message.qos = QOS2;
  message.retained = retain;
  message.payload = payload;
  message.payloadlen = strlen(payload);
  send_bz = 1;

  //os_printf("%d,%d,%s,%d,%d\n",message.qos,message.retained,message.payload,send_bz, message.payloadlen);
}


/****************************************************************************/
void user_conn_init(void)
{
    int ret;
    ret = xTaskCreate(mqtt_client_thread,
                      MQTT_CLIENT_THREAD_NAME,
                      MQTT_CLIENT_THREAD_STACK_WORDS,
                      NULL,
                      MQTT_CLIENT_THREAD_PRIO,
                      &mqttc_client_handle);
    if (ret != pdPASS)  {
            printf("mqtt create client thread %s failed\n", MQTT_CLIENT_THREAD_NAME);
        }
    }
