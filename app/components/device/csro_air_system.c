#include "csro_device.h"
#include "modbus_master.h"

static void modbus_master_task(void *pvParameters)
{
    uint8_t result[10];
    uint16_t holidng[10];
	for(;;)
	{
        modbus_master_read_coils(&Master, 1, 0, 8, result);
        for(size_t i = 0; i < 10; i++)
        {
            debug("%d ", result[i]);
        }
        debug("\n");

        modbus_master_read_holding_regs(&Master, 1, 0, 8, holidng);
        for(size_t i = 0; i < 10; i++)
        {
            debug("%d ", holidng[i]);
        }
        debug("\n");
        vTaskDelay(500 / portTICK_RATE_MS);
    }
	vTaskDelete(NULL);
}


void csro_air_system_prepare_basic_message(void)
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

void csro_air_system_handle_self_message(MessageData* data)
{
    char topic[100];
    strncpy(topic, (char *)data->topicName->lenstring.data, data->topicName->lenstring.len);
    debug("topic: %s\n", topic);
    debug("message content: %s\n", (char *)data->message->payload);
}

void csro_air_system_handle_group_message(MessageData* data)
{
    char topic[100];
    strncpy(topic, (char *)data->topicName->lenstring.data, data->topicName->lenstring.len);
    debug("topic: %s\n", topic);
    debug("message content: %s\n", (char *)data->message->payload);
}

void csro_air_system_init(void)
{
    modbus_master_init();
    xTaskCreate(modbus_master_task, "modbus_master_task", 1024, NULL, 5, NULL);
}