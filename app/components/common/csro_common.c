#include "csro_common.h"
#include "csro_device.h"
#include "ssl/ssl_crypto.h"

system_struct   system_info;
datetime_struct datetime_info;
mqtt_struct     mqtt_info;

static void get_system_info(void)
{
    nvs_handle handle;
    nvs_open("system_info", NVS_READWRITE, &handle);
    nvs_get_u32(handle, "power_count", &system_info.power_on_count);
    nvs_set_u32(handle, "power_count", (system_info.power_on_count + 1));
    nvs_commit(handle);
    nvs_close(handle);

    #ifdef NLIGHT
		sprintf(system_info.device_type, "nlight%d", NLIGHT);
	#elif defined DLIGHT
		sprintf(system_info.device_type, "dlight%d", DLIGHT);
	#elif defined MOTOR
		sprintf(system_info.device_type, "motor%d", MOTOR);
	#elif defined AQI_MONITOR
		sprintf(system_info.device_type, "air_monitor");
	#elif defined AIR_SYSTEM
		sprintf(system_info.dev_type, "air_system");
	#endif

    esp_wifi_get_mac(WIFI_MODE_STA, system_info.mac);
    sprintf(system_info.mac_str, "%02x%02x%02x%02x%02x%02x", system_info.mac[0], system_info.mac[1], system_info.mac[2], system_info.mac[3], system_info.mac[4], system_info.mac[5]);
    sprintf(system_info.host_name, "csro_%s", system_info.mac_str);
    
    sprintf(mqtt_info.id, "csro/%s", system_info.mac_str);
    sprintf(mqtt_info.name, "csro/%s/%s", system_info.mac_str, system_info.dev_type);

    sprintf(mqtt_info.sub_topic_self, "%s/%s/%s/command", mqtt_info.prefix, system_info.mac_str, system_info.dev_type);
    sprintf(mqtt_info.sub_topic_group, "%s/group", mqtt_info.prefix);

    uint8_t sha1_decrypt[20];
    SHA1_CTX *pass_ctx=(SHA1_CTX *)malloc(sizeof(SHA1_CTX));
    SHA1_Init(pass_ctx);
	SHA1_Update(pass_ctx, (const uint8_t *)system_info.mac_str, strlen(system_info.mac_str));
	SHA1_Final(sha1_decrypt, pass_ctx);
	free(pass_ctx);
	sprintf(mqtt_info.pass, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
    sha1_decrypt[0],  sha1_decrypt[2],  sha1_decrypt[4],  sha1_decrypt[6],  sha1_decrypt[8],  sha1_decrypt[1],  sha1_decrypt[3],  sha1_decrypt[5],  sha1_decrypt[7],  sha1_decrypt[9], 
    sha1_decrypt[10], sha1_decrypt[12], sha1_decrypt[14], sha1_decrypt[16], sha1_decrypt[18], sha1_decrypt[11], sha1_decrypt[13], sha1_decrypt[15], sha1_decrypt[17], sha1_decrypt[19]);

    debug("id = %s.\nname = %s.\npass = %s.\n", mqtt_info.id, mqtt_info.name, mqtt_info.pass);
}


static bool system_get_wifi_info(void)
{
    bool        result = false;
    uint8_t     router_flag = 0;
    nvs_handle  handle;
    
    nvs_open("system_info", NVS_READONLY, &handle);
    nvs_get_u8(handle, "router_flag", &router_flag);
    if(router_flag == 1) {
        size_t ssid_len = 0;
        size_t pass_len = 0;
        nvs_get_str(handle, "ssid", NULL, &ssid_len);
        nvs_get_str(handle, "ssid", system_info.router_ssid, &ssid_len);
        nvs_get_str(handle, "pass", NULL, &pass_len);
        nvs_get_str(handle, "pass", system_info.router_pass, &pass_len);
        result = true;
    }
    nvs_close(handle);
    return result;
}






void csro_system_init(void)
{
    nvs_flash_init();
    csro_datetime_init();
    get_system_info();
    // csro_device_init();

    if (system_get_wifi_info()) {
        xTaskCreate(csro_task_mqtt, "csro_task_mqtt", 4096, NULL, 10, NULL);
    }
    else {
        xTaskCreate(csro_task_smartconfig, "csro_task_smartconfig", 3072, NULL, 10, NULL);
    }

}

void csro_system_set_status(system_status status)
{
    if (system_info.status == RESET_PENDING) {
        return;
    }
    if (system_info.status != status) {
        system_info.status = status;
    }
}

bool csro_system_get_wifi_status(void)
{
    static bool is_connect = false;
    tcpip_adapter_ip_info_t info;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &info);
    if (info.ip.addr != 0) {
        if (is_connect == false) {
            is_connect = true;
            csro_system_set_status(NORMAL_START_NOSERVER);
            system_info.ip[0] = info.ip.addr&0xFF;
            system_info.ip[1] = (info.ip.addr&0xFF00)>>8;
            system_info.ip[2] = (info.ip.addr&0xFF0000)>>16;
            system_info.ip[3] = (info.ip.addr&0xFF000000)>>24;
            sprintf(system_info.ip_str, "%d.%d.%d.%d", system_info.ip[0], system_info.ip[1], system_info.ip[2], system_info.ip[3]);
        } 
    }
    else {
        csro_system_set_status(NORMAL_START_NOWIFI);
        is_connect = false;
    }
    return is_connect;
}