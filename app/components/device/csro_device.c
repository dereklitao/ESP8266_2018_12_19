#include "csro_device.h"
#include "cJSON.h"


void csro_device_prepare_basic_message(void)
{
    static uint32_t basic_count = 0;
    cJSON *root=cJSON_CreateObject();
    cJSON *air_system;
    cJSON_AddItemToObject(root, "air_system", air_system = cJSON_CreateObject());
    cJSON_AddNumberToObject(air_system, "basic_count", basic_count++);
    cJSON_AddNumberToObject(air_system, "power_count", system_info.power_on_count);
    cJSON_AddNumberToObject(air_system, "free_heap", esp_get_free_heap_size());
    cJSON_AddStringToObject(air_system, "ip", system_info.ip_str);
    cJSON_AddNumberToObject(air_system, "run_sec", datetime_info.time_run);
    char *out = cJSON_PrintUnformatted(root);
	strcpy(mqtt_info.content, out);
	free(out);
	cJSON_Delete(root);
}

void csro_device_handle_self_message(MessageData* data)
{
    char topic[100];
    strncpy(topic, (char *)data->topicName->lenstring.data, data->topicName->lenstring.len);
    debug("topic: %s\n", topic);
    debug("message content: %s\n", (char *)data->message->payload);
}

void csro_device_handle_group_message(MessageData* data)
{
    char topic[100];
    strncpy(topic, (char *)data->topicName->lenstring.data, data->topicName->lenstring.len);
    debug("topic: %s\n", topic);
    debug("message content: %s\n", (char *)data->message->payload);
}


void csro_device_init(void)
{
    #ifdef NLIGHT
		
	#elif defined DLIGHT
		
	#elif defined MOTOR
		
	#elif defined AQI_MONITOR
		
	#elif defined AIR_SYSTEM
	    csro_air_system_init();
	#endif
}