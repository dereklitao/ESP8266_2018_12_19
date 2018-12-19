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

}

void csro_air_system_handle_self_message(MessageData* data)
{

}

void csro_air_system_handle_group_message(MessageData* data)
{

}

void csro_air_system_init(void)
{
    modbus_master_init();
    xTaskCreate(modbus_master_task, "modbus_master_task", 1024, NULL, 5, NULL);
}