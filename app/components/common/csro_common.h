#ifndef CSRO_COMMON_H_
#define CSRO_COMMON_H_

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/uart.h"


#include "MQTTClient.h"
#include "MQTTFreeRTOS.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"

#define     DEBUG
#ifdef      DEBUG
   #define  debug(format, ...) printf(format, ## __VA_ARGS__)
#else
   #define  debug(format, ...)
#endif

#define     MQTT_BUFFER_LENGTH      1000
#define     MQTT_TOPIC_LENGTH       100
#define     MQTT_NAME_ID_LENGTH     50
#define     ALARM_NUMBER            20

#define     MIN_INTERVAL            2
#define     MAX_INTERVAL            120



 #define     NLIGHT                  3
// #define     DLIGHT                  1
// #define     MOTOR                   2
// #define     AIR_MONITOR
// #define     AIR_SYSTEM


typedef enum 
{
    IDLE = 0,
    SMARTCONFIG = 1,
    SMARTCONFIG_TIMEOUT = 2,
    NORMAL_START_NOWIFI = 3,
    NORMAL_START_NOSERVER = 4,
    NORMAL_START_OK = 5,
    RESET_PENDING = 6
} system_status;


typedef struct 
{
    system_status   status;
    char            router_ssid[50];
    char            router_pass[50];

    uint8_t         mac[6];
    uint8_t         ip[4];

    char            host_name[20];
    char            dev_type[20];
    char            mac_str[20];
    char            ip_str[20];

    uint32_t        power_on_count;
    uint32_t        wifi_conn_count;
    uint32_t        serv_conn_count;
    uint32_t        publish_count;
} system_struct;


typedef struct 
{
    char            id[MQTT_NAME_ID_LENGTH];
    char            name[MQTT_NAME_ID_LENGTH];
    char            pass[MQTT_NAME_ID_LENGTH];

    char            sub_topic_self[MQTT_TOPIC_LENGTH];
    char            sub_topic_group[MQTT_TOPIC_LENGTH];
    char            pub_topic[MQTT_TOPIC_LENGTH];

    uint8_t         send_buf[MQTT_BUFFER_LENGTH];
    uint8_t         recv_buf[MQTT_BUFFER_LENGTH];
    char            content[MQTT_BUFFER_LENGTH];

    char            broker[50];
    char            prefix[50];

    uint16_t        interval;
    struct Network  network;
    MQTTClient      client;
    MQTTMessage     message;
} mqtt_struct;


typedef struct 
{
    time_t          time_on;
    time_t          time_conn;
    time_t          time_now;
    time_t          time_run;
    struct tm       time_info;
    char            time_str[64];
} datetime_struct;


typedef struct 
{
    bool            valid;
    uint8_t         weekday;
    uint16_t        minutes;
    uint16_t         action;
} alarm_struct;


extern system_struct        system_info;
extern datetime_struct      datetime_info;
extern mqtt_struct          mqtt_info;

extern SemaphoreHandle_t    basic_msg_semaphore;
extern SemaphoreHandle_t    system_msg_semaphore;
extern SemaphoreHandle_t    timer_msg_semaphore;
extern TimerHandle_t        basic_msg_timer;


void csro_system_init(void);
void csro_system_set_status(system_status status);
bool csro_system_get_wifi_status(void);
void csro_system_prepare_message(void);
bool csro_system_parse_json_number(char *msg, uint32_t *dest, char *object_name, char *sub_object_name);
bool csro_system_parse_json_string(char *msg, char *dest, char *object_name, char *sub_object_name);


void csro_datetime_init(void);
void csro_datetime_set(char *time_str);

void csro_task_mqtt(void *pvParameters);
void csro_task_smartconfig(void *pvParameters);

void csro_alarm_add(uint8_t weekday, uint16_t minutes, uint16_t action);
void csro_alarm_modify(uint8_t index, uint8_t weekday, uint16_t minutes, uint16_t action);
void csro_alarm_delete(uint8_t index);
void csro_alarm_clear(void);


#endif
