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


static void write_coil_with_flag_and_range_check(uint8_t index)
{
    if (air_system_reg.coil_flag[index] == false) {
        return;
    }
    if (air_system_reg.coil[index] == 1 || air_system_reg.coil[index] == 0) {
        modbus_master_write_single_coil(&Master, 1, index, air_system_reg.coil[index]);
    }
    air_system_reg.coil_flag[index] = false;
}

static void write_holding_with_flag_and_range_check(uint8_t index, uint16_t min, uint16_t max)
{    
    if (air_system_reg.holding_flag[index] == false) {
        return;
    }
    if (air_system_reg.holding[index] >= min && air_system_reg.holding[index] <= max) {
        modbus_master_Write_single_holding_reg(&Master, 1, index, air_system_reg.holding[index]);
    }
    air_system_reg.holding_flag[index] = false;
}


static void modbus_master_write_task(void *pvParameters)
{
    while(true)
    {
        if (xSemaphoreTake(write_semaphore, portMAX_DELAY) == pdTRUE ) {
            if (xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE) {
                write_coil_with_flag_and_range_check(40);
                write_coil_with_flag_and_range_check(41);
                write_holding_with_flag_and_range_check(21, 1, 2);
                xSemaphoreGive(master_mutex);
                xSemaphoreGive(basic_msg_semaphore);
                xTimerReset(basic_msg_timer, 0);
            }
        }
    }
    vTaskDelete(NULL);
}


void csro_air_system_prepare_basic_message(void)
{
    cJSON *root=cJSON_CreateObject();

    cJSON *air_condition, *air_purifier, *water_valve;
    cJSON_AddItemToObject(root, "air_condition", air_condition = cJSON_CreateObject());
    cJSON_AddItemToObject(root, "air_purifier", air_purifier = cJSON_CreateObject());
    cJSON_AddItemToObject(root, "water_valve", water_valve = cJSON_CreateObject());

    uint32_t warnning = 0;
    for(size_t i = 1; i < 26; i++) { warnning = warnning | (((uint32_t)(air_system_reg.coil[i] == 1 ? 1 : 0)) << (i - 1)); }
    
    cJSON_AddNumberToObject(air_condition, "warning", warnning);
    cJSON_AddNumberToObject(air_condition, "comp_on", air_system_reg.coil[26]);
    cJSON_AddNumberToObject(air_condition, "pump_on", air_system_reg.coil[27]);
    cJSON_AddNumberToObject(air_condition, "fan_on", air_system_reg.coil[28]);
    cJSON_AddNumberToObject(air_condition, "fourvalve", air_system_reg.coil[31]);
    cJSON_AddNumberToObject(air_condition, "on", air_system_reg.coil[40]);
    cJSON_AddNumberToObject(air_condition, "status", air_system_reg.holding[20]);
    cJSON_AddNumberToObject(air_condition, "mode", air_system_reg.holding[21]);
    cJSON_AddNumberToObject(air_condition, "cold_set", air_system_reg.holding[24]);
    cJSON_AddNumberToObject(air_condition, "hot_set", air_system_reg.holding[25]);

    cJSON_AddNumberToObject(air_condition, "supply_t", air_system_reg.holding[1]);
    cJSON_AddNumberToObject(air_condition, "return_t", air_system_reg.holding[2]);
    cJSON_AddNumberToObject(air_condition, "outdoor_t", air_system_reg.holding[3]);
    cJSON_AddNumberToObject(air_condition, "back_air_t", air_system_reg.holding[4]);
    cJSON_AddNumberToObject(air_condition, "exhaust_air_t", air_system_reg.holding[5]);
    cJSON_AddNumberToObject(air_condition, "low_p", air_system_reg.holding[6]);
    cJSON_AddNumberToObject(air_condition, "high_p", air_system_reg.holding[7]);
    cJSON_AddNumberToObject(air_condition, "pump_fb", air_system_reg.holding[8]);
    cJSON_AddNumberToObject(air_condition, "water_p", air_system_reg.holding[9]);
    cJSON_AddNumberToObject(air_condition, "outdoor_water_t", air_system_reg.holding[10]);
    cJSON_AddNumberToObject(air_condition, "expan_open", air_system_reg.holding[11]);
    cJSON_AddNumberToObject(air_condition, "overheat", air_system_reg.holding[12]);
    cJSON_AddNumberToObject(air_condition, "comp_freq", air_system_reg.holding[13]);
    cJSON_AddNumberToObject(air_condition, "pump_per", air_system_reg.holding[14]);
    cJSON_AddNumberToObject(air_condition, "fan_voltage", air_system_reg.holding[15]);
    cJSON_AddNumberToObject(air_condition, "version", air_system_reg.holding[16]);

    char *out = cJSON_PrintUnformatted(root);
	strcpy(mqtt_info.content, out);
	free(out);
	cJSON_Delete(root);
}

void csro_air_system_prepare_timer_message(void)
{
    ;
}



static void modify_coil_reg(uint8_t index, uint8_t value)
{
    if(value == 1 || value == 0) {
        air_system_reg.coil[index] = value;
        air_system_reg.coil_flag[index] = true;
        xSemaphoreGive(write_semaphore);
    }
}

static void modify_holding_reg(uint8_t index, uint8_t value)
{
    air_system_reg.holding[index] = value;
    air_system_reg.holding_flag[index] = true;
    xSemaphoreGive(write_semaphore);
}


void csro_air_system_handle_self_message(MessageData* data)
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

    if(csro_system_parse_json_number(data->message->payload, &value, "ac_command", "on")) { modify_coil_reg(40, value); }
    if(csro_system_parse_json_number(data->message->payload, &value, "ac_command", "mode")) { modify_holding_reg(21, value); }
    if(csro_system_parse_json_number(data->message->payload, &value, "ac_command", "cold_set")) { modify_holding_reg(24, value); }
    if(csro_system_parse_json_number(data->message->payload, &value, "ac_command", "hot_set")) { modify_holding_reg(25, value); }




}

void csro_air_system_handle_group_message(MessageData* data)
{
    uint32_t value = 0;
    if(csro_system_parse_json_number(data->message->payload, &value, "command", "update_basic")) {
        if(value == 1) {
            xSemaphoreGive(basic_msg_semaphore);
            xTimerReset(basic_msg_timer, 0);
        }
    }
}

void csro_air_system_init(void)
{
    master_mutex = xSemaphoreCreateMutex();
    write_semaphore = xSemaphoreCreateBinary();
    modbus_master_init();
    xTaskCreate(modbus_master_read_task, "modbus_master_read_task", 1024, NULL, 5, NULL);
    xTaskCreate(modbus_master_write_task, "modbus_master_write_task", 1024, NULL, 8, NULL);
}