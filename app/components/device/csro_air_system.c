#include "csro_device.h"
#include "modbus_master.h"

struct air_sys_register
{
    uint8_t     coil[80];
    bool        coil_flag[80];
    uint16_t    holding[80];
    bool        holding_flag[80];
} air_system_reg;

static SemaphoreHandle_t    master_mutex;
static SemaphoreHandle_t    write_semaphore;


static void modbus_master_read_task(void *pvParameters)
{
    while(true)
	{
        if ( xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE ) {
            uint8_t temp_coil[80];
            modbus_master_read_coils(&Master, 1, 1, 58, &temp_coil[1]);
            debug("read coil.\n");
            for(size_t i = 1; i < 59; i++)
            {
               if (air_system_reg.coil_flag[i] == false) {
                   air_system_reg.coil[i] = temp_coil[i];
               }
            }
            xSemaphoreGive(master_mutex);
        }

        if ( xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE ) {
            uint16_t temp_holding[80];
            modbus_master_read_holding_regs(&Master, 1, 1, 42, &temp_holding[1]);
            debug("read holding.\n");
            for(size_t i = 1; i < 43; i++)
            {
               if (air_system_reg.holding_flag[i] == false) {
                   air_system_reg.holding[i] = temp_holding[i];
               }
            }
            xSemaphoreGive(master_mutex);
        }       
        vTaskDelay(200 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void modbus_master_write_task(void *pvParameters)
{
    static uint16_t value37 = 0;
    static uint16_t value38 = 0;
    while(true)
	{
        if (xSemaphoreTake(write_semaphore, portMAX_DELAY) == pdTRUE ) {
            if (xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE) {
                if ( air_system_reg.holding_flag[37] == true) {
                    modbus_master_Write_single_holding_reg(&Master, 1, 37, value37);
                    air_system_reg.holding_flag[37] = false;
                    value37++;
                    debug("write 37.\n");
                }
                if ( air_system_reg.holding_flag[38] == true) {
                    modbus_master_Write_single_holding_reg(&Master, 1, 38, value38);
                    air_system_reg.holding_flag[38] = false;
                    value38++;
                    debug("write 38.\n");
                }
                xSemaphoreGive(master_mutex);
            }
        }
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
    air_system_reg.holding_flag[38] = true;
    xSemaphoreGive(write_semaphore);

    uint32_t value = 0;
    if(csro_system_parse_json_number(data->message->payload, &value, "test", "value")) {
        debug("value = %d\n", value);
    }

    char mqtt_msg[200];
    bzero(mqtt_msg, 200);
    if(csro_system_parse_json_string(data->message->payload, mqtt_msg, "test", "string")) {
        debug("string = %s\n", mqtt_msg);
    }
}

void csro_air_system_handle_group_message(MessageData* data)
{
    char topic[100];
    strncpy(topic, (char *)data->topicName->lenstring.data, data->topicName->lenstring.len);
    debug("topic: %s\n", topic);
    debug("message content: %s\n", (char *)data->message->payload);
    air_system_reg.holding_flag[37] = true;
    xSemaphoreGive(write_semaphore);
}

void csro_air_system_init(void)
{
    master_mutex = xSemaphoreCreateMutex();
    write_semaphore = xSemaphoreCreateBinary();
    modbus_master_init();
    xTaskCreate(modbus_master_read_task, "modbus_master_read_task", 1024, NULL, 5, NULL);
    xTaskCreate(modbus_master_write_task, "modbus_master_write_task", 1024, NULL, 8, NULL);
}