#include "csro_device.h"

void csro_device_prepare_basic_message(void)
{
    #ifdef NLIGHT
        csro_nlight_prepare_basic_message();
    #elif defined DLIGHT
        csro_dlight_prepare_basic_message();
    #elif defined MOTOR
        csro_motor_prepare_basic_message();
    #elif defined AQI_MONITOR
        csro_air_monitor_prepare_basic_message();
    #elif defined AIR_SYSTEM
        csro_air_system_prepare_basic_message();
    #endif
}

void csro_device_prepare_timer_message(void)
{
    #ifdef NLIGHT
        csro_nlight_prepare_timer_message();
    #elif defined DLIGHT
        csro_dlight_prepare_timer_message();
    #elif defined MOTOR
        csro_motor_prepare_timer_message();
    #elif defined AQI_MONITOR
        csro_air_monitor_prepare_timer_message();
    #elif defined AIR_SYSTEM
        csro_air_system_prepare_timer_message();
    #endif
}

void csro_device_handle_self_message(MessageData* data)
{
    #ifdef NLIGHT
        csro_nlight_handle_self_message(data);
    #elif defined DLIGHT
        csro_dlight_handle_self_message(data);
    #elif defined MOTOR
        csro_motor_handle_self_message(data);
    #elif defined AQI_MONITOR
        csro_air_monitor_handle_self_message(data);
    #elif defined AIR_SYSTEM
        csro_air_system_handle_self_message(data);
    #endif
}

void csro_device_handle_group_message(MessageData* data)
{
    #ifdef NLIGHT
        csro_nlight_handle_group_message(data);
    #elif defined DLIGHT
        csro_dlight_handle_group_message(data);
    #elif defined MOTOR
        csro_motor_handle_group_message(data);
    #elif defined AQI_MONITOR
        csro_air_monitor_handle_group_message(data);
    #elif defined AIR_SYSTEM
        csro_air_system_handle_group_message(data);
    #endif
}

void csro_device_alarm_action(uint16_t action)
{
    #ifdef NLIGHT
        csro_nlight_alarm_action(action);
    #elif defined DLIGHT
        csro_dlight_alarm_action(action);
    #elif defined MOTOR
        csro_motor_alarm_action(action);
    #elif defined AQI_MONITOR
        csro_air_monitor_alarm_action(action);
    #elif defined AIR_SYSTEM
        csro_air_system_alarm_action(action);
    #endif
}

void csro_device_init(void)
{
    #ifdef NLIGHT
        csro_nlight_init();
    #elif defined DLIGHT
        csro_dlight_init();
    #elif defined MOTOR
        csro_motor_init();
    #elif defined AQI_MONITOR
        csro_air_monitor_init();
    #elif defined AIR_SYSTEM
        csro_air_system_init();
    #endif
}