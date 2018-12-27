#include "csro_device.h"

static csro_switch      channel[3];
static TimerHandle_t	key_timer = NULL;
static TimerHandle_t	led_timer = NULL;

static void nlight_channel_set(csro_switch *ch, uint8_t cmd, uint8_t src)
{
    if((ch->index > NLIGHT) || (cmd > 2)) { return; }
    if(cmd == 2) { cmd = !ch->status; }
    //gpio_set_level(ch->pin_num, cmd);
    ch->status = cmd;
    if(ch->status == 1) {  ch->on_src = src; ch->on_tim = datetime_info.time_now; }
    else                { ch->off_src = src; ch->off_tim = datetime_info.time_now; }

    debug("%d-%d-%d,%d:%d:%d\n", datetime_info.time_info.tm_year+1900, datetime_info.time_info.tm_mon+1, datetime_info.time_info.tm_mday, datetime_info.time_info.tm_hour, datetime_info.time_info.tm_min, datetime_info.time_info.tm_sec);

    xSemaphoreGive(basic_msg_semaphore);
    xTimerReset(basic_msg_timer, 0);
}


static void key_timer_callback( TimerHandle_t xTimer )
{

}

static void led_timer_callback( TimerHandle_t xTimer )
{

}


static void nlight_add_channel_json(cJSON *target, cJSON *json_channel, csro_switch *channel)
{
	char name[10];
	sprintf(name, "ch%d", channel->index);
	cJSON_AddItemToObject(target, name, json_channel = cJSON_CreateObject());
	cJSON_AddNumberToObject(json_channel, "on", channel->status);
	cJSON_AddNumberToObject(json_channel, "onsrc", channel->on_src);
    cJSON_AddNumberToObject(json_channel, "ontim", (u_long)channel->on_tim);
	cJSON_AddNumberToObject(json_channel, "offsrc", channel->off_src);
    cJSON_AddNumberToObject(json_channel, "offtim", (u_long)channel->off_tim);
}

void csro_nlight_prepare_basic_message(void)
{
    cJSON *root=cJSON_CreateObject();
    cJSON *switch1 = NULL;
    cJSON *switch2 = NULL;
    cJSON *switch3 = NULL;
    
    if ( NLIGHT >= 1 ) {  nlight_add_channel_json(root, switch1, &channel[0]); }
    if ( NLIGHT >= 2 ) {  nlight_add_channel_json(root, switch2, &channel[1]); }
    if ( NLIGHT >= 3 ) {  nlight_add_channel_json(root, switch3, &channel[2]); }

    char *out = cJSON_PrintUnformatted(root);
	strcpy(mqtt_info.content, out);
	free(out);
	cJSON_Delete(root);
}

void csro_nlight_prepare_timer_message(void)
{}

void csro_nlight_handle_self_message(MessageData* data)
{
    uint32_t value = 0;
    if(csro_system_parse_json_number(data->message->payload, &value, "command", "interval")) {
        if (value >= 2 && value <= 120) {
            nvs_handle handle;
            nvs_open("system_info", NVS_READWRITE, &handle);
            nvs_set_u16(handle, "interval", value);
            nvs_commit(handle);
            nvs_close(handle);
            mqtt_info.interval = value;
            xTimerChangePeriod(basic_msg_timer, (mqtt_info.interval * 1000)/portTICK_RATE_MS, 0);
            xTimerReset(basic_msg_timer, 0);
        }
    }

    if(csro_system_parse_json_number(data->message->payload, &value, "command", "ch1")) { nlight_channel_set(&channel[0], value, 1); }
    if(csro_system_parse_json_number(data->message->payload, &value, "command", "ch2")) { nlight_channel_set(&channel[1], value, 1); }
    if(csro_system_parse_json_number(data->message->payload, &value, "command", "ch3")) { nlight_channel_set(&channel[2], value, 1); }
}

void csro_nlight_handle_group_message(MessageData* data)
{
    uint32_t value = 0;
    if(csro_system_parse_json_number(data->message->payload, &value, "command", "update_basic")) {
        if(value == 1) {
            xSemaphoreGive(basic_msg_semaphore);
            xTimerReset(basic_msg_timer, 0);
        }
    }
}
void csro_nlight_alarm_action(uint16_t action)
{}

void csro_nlight_init(void)
{
    //key gpio init;
    //led gpio init;
    channel[0].index = 1;
    channel[1].index = 2;
    channel[2].index = 3;
    key_timer = xTimerCreate("key_timer", 25/portTICK_RATE_MS, pdTRUE, (void *)0, key_timer_callback);
    led_timer = xTimerCreate("led_timer", 25/portTICK_RATE_MS, pdTRUE, (void *)0, led_timer_callback);
    if (key_timer != NULL) { xTimerStart(key_timer, 0); }
    if (led_timer != NULL) { xTimerStart(led_timer, 0); }
}