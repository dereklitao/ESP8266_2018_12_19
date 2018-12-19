#include "csro_common.h"

static TimerHandle_t	second_timer = NULL;
static bool	            time_sync = false;
static alarm_struct     alarms[ALARM_NUMBER];


static void load_alarm_from_nvs(void)
{
    nvs_handle handle;
    uint8_t alarm_count = 0;
    nvs_open("alarms", NVS_READONLY, &handle);
    nvs_get_u8(handle, "alarm_count", &alarm_count);
    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        if (i < alarm_count) {
            char alarm_key[20];
            uint32_t alarm_value = 0;
            sprintf(alarm_key, "alarm%d", i);
            nvs_get_u32(handle, alarm_key, &alarm_value);
            alarms[i].valid = true;
            alarms[i].weekday = (uint8_t)((alarm_value & 0xFF000000) >> 24);
            alarms[i].minutes = (uint16_t)((alarm_value & 0x00FFFF00) >> 8);
            alarms[i].action = (uint8_t)(alarm_value & 0x000000FF);
            debug("alarm %d: weekday = %d, minutes = %d, action = %d\r\n", i, alarms[i].weekday, alarms[i].minutes, alarms[i].action);
        }
        else {
            alarms[i].valid = false;
            debug("alarm number: %d\r\n", i);
            break;
        }
    }
    nvs_close(handle);
}


static void save_alarm_to_nvs(void)
{
    nvs_handle handle;
    uint8_t alarm_count = 0;
    nvs_open("alarms", NVS_READWRITE, &handle);
    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        if (alarms[i].valid == true) {
            alarm_count++;
            char alarm_key[20];
            uint32_t alarm_value = 0;
            sprintf(alarm_key, "alarm%d", i);
            alarm_value = (((uint32_t)alarms[i].weekday) << 24) + (((uint32_t)alarms[i].minutes) << 8) + alarms[i].action;
            nvs_set_u32(handle, alarm_key, alarm_value);
        }
        else {
            break;
        }
    }
    nvs_set_u8(handle, "alarm_count", alarm_count);
    nvs_commit(handle);
    nvs_close(handle);
}


void csro_alarm_add(uint8_t weekday, uint16_t minutes, uint8_t action)
{
    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        if (alarms[i].valid == true) {
            if (alarms[i].weekday == weekday && alarms[i].minutes == minutes) {
                alarms[i].action = action;
                save_alarm_to_nvs();
                debug("alarm exist alarm %d, ", i);
                return;
            }
        }
    }

    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        if (alarms[i].valid == false) {
            alarms[i].valid = true;
            alarms[i].weekday = weekday;
            alarms[i].minutes = minutes;
            alarms[i].action = action;
            save_alarm_to_nvs();
            debug("add new alarm alarm %d", i);
            return;
        }
    }
}

void csro_alarm_modify(uint8_t index, uint8_t weekday, uint16_t minutes, uint8_t action)
{
    if (index > ALARM_NUMBER - 1) {
        return;
    }
    if (alarms[index].valid == true)
    {
        alarms[index].weekday = weekday;
        alarms[index].minutes = minutes;
        alarms[index].action = action;
        save_alarm_to_nvs();
    }
}

void csro_alarm_delete(uint8_t index)
{
    if (index > ALARM_NUMBER - 1) {
        return;
    }
    if (alarms[index].valid == true)
    {
        alarms[index].valid = false;
        for(size_t i = 0; i < ALARM_NUMBER - 1; i++)
        {
            if (i >= index && alarms[i+1].valid == true) {
                alarms[i].valid = true;
                alarms[i].weekday = alarms[i + 1].weekday;
                alarms[i].minutes = alarms[i + 1].minutes;
                alarms[i].action = alarms[i + 1].action;
                alarms[i + 1].valid = false;
            }
        }
        save_alarm_to_nvs();
    }
}

void csro_alarm_clear(void)
{
    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        alarms[i].valid = false;
    }
    save_alarm_to_nvs();
}


static void datetime_check_alarm(void)
{
    debug("check alarm\r\n");
    uint16_t now_minutes = datetime_info.time_info.tm_hour * 60 + datetime_info.time_info.tm_min;
    uint8_t now_weekday = datetime_info.time_info.tm_wday;
    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        if (alarms[i].valid == true && alarms[i].minutes == now_minutes)
        {
            if ((alarms[i].weekday & (0x01<<now_weekday)) == (0x01<<now_weekday)) {
                debug("alarm on @ %d, %d, %d\r\n", now_weekday, now_minutes, alarms[i].action);
            }
        }
    }
}

static void second_timer_callback( TimerHandle_t xTimer )
{
    debug("second_tick\n");
    datetime_info.time_run++;
    if (time_sync == true) {
        datetime_info.time_now++;
        localtime_r(&datetime_info.time_now, &datetime_info.time_info);
        if (datetime_info.time_info.tm_sec == 0) {
            datetime_check_alarm();
        }
    }
}

void csro_datetime_init(void)
{
    second_timer = xTimerCreate("second_timer", 1000/portTICK_RATE_MS, pdTRUE, (void *)0, second_timer_callback);
    if (second_timer != NULL) {
        xTimerStart(second_timer, 0);
        load_alarm_from_nvs();
    }
}

void csro_datetime_set(char *time_str)
{
    uint32_t tim[6];
    sscanf(time_str, "%d-%d-%d %d:%d:%d", &tim[0], &tim[1], &tim[2], &tim[3], &tim[4], &tim[5]);
    if ((tim[0]<2018) || (tim[1]>12) || (tim[2]>31) || (tim[3]>23) || (tim[4]>59) || (tim[5]>59)) {
        return;
    }
    if (time_sync == false || tim[5] == 30) {
        datetime_info.time_info.tm_year     = ((tim[0] % 2000) + 2000) - 1900;
        datetime_info.time_info.tm_mon      = tim[1] - 1;
        datetime_info.time_info.tm_mday     = tim[2];
        datetime_info.time_info.tm_hour     = tim[3];
        datetime_info.time_info.tm_min      = tim[4];
        datetime_info.time_info.tm_sec      = tim[5];
        datetime_info.time_now              = mktime(&datetime_info.time_info);

        if (time_sync == false) {
            time_sync = true;
            datetime_info.time_on = datetime_info.time_now - datetime_info.time_run;
        }
        datetime_info.time_run = datetime_info.time_now - datetime_info.time_on;
    }
}