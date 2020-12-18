#include <stdlib.h>

#include "esp32_spi.h"
#include "esp32_spi_io.h"
#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"
#include "fpioa.h"
#include "printf.h"
#include "errno.h"


// Cached values of retrieved data
char ssid[32] = {0};
uint8_t mac[32] = {0};
esp32_spi_net_t net_dat;
uint8_t cs_num, rst_num, rdy_num, is_hard_spi;


static void esp32_spi_reset(void);
static void delete_esp32_spi_params(void *arg);
static void delete_esp32_spi_aps_list(void *arg);
static esp32_spi_params_t *esp32_spi_params_alloc_1param(uint32_t len, uint8_t *buf);
static esp32_spi_params_t *esp32_spi_params_alloc_2param(uint32_t len_0, uint8_t *buf_0, uint32_t len_1, uint8_t *buf_1);
static int8_t esp32_spi_send_command(uint8_t cmd, esp32_spi_params_t *params, uint8_t param_len_16);

void esp32_spi_init(uint8_t t_cs_num, uint8_t t_rst_num, uint8_t t_rdy_num, uint8_t t_hard_spi)
{
    cs_num = t_cs_num, rst_num = t_rst_num, rdy_num = t_rdy_num, is_hard_spi = t_hard_spi;
    //cs
    gpiohs_set_drive_mode(cs_num, GPIO_DM_OUTPUT);
    gpiohs_set_pin(cs_num, 1);

    //ready
    gpiohs_set_drive_mode(rdy_num, GPIO_DM_INPUT); //ready

    if ((int8_t)rst_num > 0)
    {
        rst_num -= FUNC_GPIOHS0;
        gpiohs_set_drive_mode(rst_num, GPIO_DM_OUTPUT); //reset
    }

#if ESP32_HAVE_IO0
    gpiohs_set_drive_mode(ESP32_SPI_IO0_HS_NUM, GPIO_DM_INPUT); //gpio0
#endif

    esp32_spi_reset();
}

//Hard reset the ESP32 using the reset pin
static void esp32_spi_reset(void)
{
#if ESP32_SPI_DEBUG
    printk("Reset ESP32\r\n");
#endif

#if ESP32_HAVE_IO0
    gpiohs_set_drive_mode(ESP32_SPI_IO0_HS_NUM, GPIO_DM_OUTPUT); //gpio0
    gpiohs_set_pin(ESP32_SPI_IO0_HS_NUM, 1);
#endif

    //here we sleep 1s
    gpiohs_set_pin(cs_num, 1);

    if ((int8_t)rst_num > 0)
    {
        gpiohs_set_pin(rst_num, 0);
        msleep(500);
        gpiohs_set_pin(rst_num, 1);
        msleep(800);
    }
    else
    {
        //soft reset
        esp32_spi_send_command(SOFT_RESET_CMD, NULL, 0);
        msleep(1500);
    }

#if ESP32_HAVE_IO0
    gpiohs_set_drive_mode(ESP32_SPI_IO0_HS_NUM, GPIO_DM_INPUT); //gpio0
#endif
}

//Wait until the ready pin goes low
// 0 get response
// -1 error, no response
int8_t esp32_spi_wait_for_ready(void)
{
#if (ESP32_SPI_DEBUG >= 3)
    printk("Wait for ESP32 ready\r\n");
#endif

    uint64_t tm = sysctl_get_time_us();
    while ((sysctl_get_time_us() - tm) < 10 * 1000 * 1000) //10s
    {
        if (gpiohs_get_pin(rdy_num) == 0)
            return 0;

#if (ESP32_SPI_DEBUG >= 3)
        printk(".");
#endif
        msleep(1); //FIXME
    }

#if (ESP32_SPI_DEBUG >= 3)
    printk("esp32 not responding\r\n");
#endif

    return -1;
}

#define lc_buf_len 256
uint8_t lc_send_buf[lc_buf_len];
uint8_t lc_buf_flag = 0;

/// Send over a command with a list of parameters
// -1 error
// other right
static int8_t esp32_spi_send_command(uint8_t cmd, esp32_spi_params_t *params, uint8_t param_len_16)
{
    uint32_t packet_len = 0;

    packet_len = 4; // header + end byte
    if (params != NULL)
    {
        for (uint32_t i = 0; i < params->params_num; i++)
        {
            packet_len += params->params[i]->param_len;
            packet_len += 1; // size byte
            if (param_len_16)
                packet_len += 1;
        }
    }
    while (packet_len % 4 != 0)
        packet_len += 1;

    uint8_t *sendbuf = NULL;

    if (packet_len > lc_buf_len)
    {
        sendbuf = (uint8_t *)malloc(sizeof(uint8_t) * packet_len);
        lc_buf_flag = 0;
        if (!sendbuf)
        {
#if (ESP32_SPI_DEBUG)
            printk("%s: malloc error\r\n", __func__);
#endif
            return -1;
        }
    }
    else
    {
        sendbuf = lc_send_buf;
        lc_buf_flag = 1;
    }

    sendbuf[0] = START_CMD;
    sendbuf[1] = cmd & ~REPLY_FLAG;
    if (params != NULL)
        sendbuf[2] = params->params_num;
    else
        sendbuf[2] = 0;

    uint32_t ptr = 3;

    if (params != NULL)
    {
        //handle parameters here
        for (uint32_t i = 0; i < params->params_num; i++)
        {
#if (ESP32_SPI_DEBUG >= 2)
            printk("\tSending param #%d is %d bytes long\r\n", i, params->params[i]->param_len);
#endif

            if (param_len_16)
            {
                sendbuf[ptr] = (uint8_t)((params->params[i]->param_len >> 8) & 0xFF);
                ptr += 1;
            }
            sendbuf[ptr] = (uint8_t)(params->params[i]->param_len & 0xFF);
            ptr += 1;
            memcpy(sendbuf + ptr, params->params[i]->param, params->params[i]->param_len);
            ptr += params->params[i]->param_len;
        }
    }
    sendbuf[ptr] = END_CMD;

    esp32_spi_wait_for_ready();
    gpiohs_set_pin(cs_num, 0);

    uint64_t tm = sysctl_get_time_us();
    while ((sysctl_get_time_us() - tm) < 1000 * 1000)
    {
        if (gpiohs_get_pin(rdy_num))
            break;
        msleep(1);
    }

    if ((sysctl_get_time_us() - tm) > 1000 * 1000)
    {
#if (ESP32_SPI_DEBUG)
        printk("ESP32 timed out on SPI select\r\n");
#endif
        gpiohs_set_pin(cs_num, 1);
        if (lc_buf_flag == 0)
        {
            free(sendbuf);
            sendbuf = NULL;
        }
        else
        {
            memset(sendbuf, 0, packet_len);
        }
        return -1;
    }

    if (is_hard_spi) {
        hard_spi_rw_len(sendbuf, NULL, packet_len);
    } else {
        soft_spi_rw_len(sendbuf, NULL, packet_len);
    }
    gpiohs_set_pin(cs_num, 1);

#if (ESP32_SPI_DEBUG >= 3)
    if (packet_len < 100)
    {
        printk("Wrote buf packet_len --> %d: ", packet_len);
        for (uint32_t i = 0; i < packet_len; i++)
            printk("%02x ", sendbuf[i]);
        printk("\r\n");
    }
#endif
    if (lc_buf_flag == 0)
    {
        free(sendbuf);
        sendbuf = NULL;
    }
    else
    {
        memset(sendbuf, 0, packet_len);
    }
    return 0;
}

/// Read one byte from SPI
uint8_t esp32_spi_read_byte(void)
{
    uint8_t read = 0x0;
    if (is_hard_spi) {
        read = hard_spi_rw(0xff);
    } else {
        read = soft_spi_rw(0xff);
    }

#if (ESP32_SPI_DEBUG >= 3)
    printk("\t\tRead:%02x\r\n", read);
#endif

    return read;
}

///Read many bytes from SPI
void esp32_spi_read_bytes(uint8_t *buffer, uint32_t len)
{
    if (is_hard_spi) {
        hard_spi_rw_len(NULL, buffer, len);
    } else {
        soft_spi_rw_len(NULL, buffer, len);
    }

#if (ESP32_SPI_DEBUG >= 3)
    if (len < 100)
    {
        printk("\t\tRead:");
        for (uint32_t i = 0; i < len; i++)
            printk("%02x ", *(buffer + i));
        printk("\r\n");
    }
#endif
}

///Read a byte with a time-out, and if we get it, check that its what we expect
//0 succ
//-1 error
int8_t esp32_spi_wait_spi_char(uint8_t want)
{
    uint8_t read = 0x0;
    uint64_t tm = sysctl_get_time_us();

    while ((sysctl_get_time_us() - tm) < 100 * 1000)
    {
        read = esp32_spi_read_byte();

        if (read == ERR_CMD)
        {
#if ESP32_SPI_DEBUG
            printk("Error response to command\r\n");
#endif
            return -1;
        }
        else if (read == want)
            return 0;
    }

#if 0
    if ((sysctl_get_time_us() - tm) > 100 * 1000)
    {
#if ESP32_SPI_DEBUG
        printk("Timed out waiting for SPI char\r\n");
#endif
        return -1;
    }
#endif

    return -1;
}

///Read a byte and verify its the value we want
//0 right
//-1 error
uint8_t esp32_spi_check_data(uint8_t want)
{
    uint8_t read = esp32_spi_read_byte();

    if (read != want)
    {
#if ESP32_SPI_DEBUG
        printk("Expected %02X but got %02X\r\n", want, read);
#endif
        return -1;
    }
    return 0;
}

///Wait for ready, then parse the response
//NULL error
esp32_spi_params_t *esp32_spi_wait_response_cmd(uint8_t cmd, uint32_t *num_responses, uint8_t param_len_16)
{
    uint32_t num_of_resp = 0;

    esp32_spi_wait_for_ready();

    gpiohs_set_pin(cs_num, 0);

    uint64_t tm = sysctl_get_time_us();
    while ((sysctl_get_time_us() - tm) < 1000 * 1000)
    {
        if (gpiohs_get_pin(rdy_num))
            break;
        msleep(1);
    }

    if ((sysctl_get_time_us() - tm) > 1000 * 1000)
    {
#if ESP32_SPI_DEBUG
        printk("ESP32 timed out on SPI select\r\n");
#endif
        gpiohs_set_pin(cs_num, 1);
        return NULL;
    }

    if (esp32_spi_wait_spi_char(START_CMD) != 0)
    {
#if ESP32_SPI_DEBUG
        printk("esp32_spi_wait_spi_char START_CMD error\r\n");
#endif
        gpiohs_set_pin(cs_num, 1);
        return NULL;
    }

    if (esp32_spi_check_data(cmd | REPLY_FLAG) != 0)
    {
#if ESP32_SPI_DEBUG
        printk("esp32_spi_check_data cmd | REPLY_FLAG error\r\n");
#endif
        gpiohs_set_pin(cs_num, 1);
        return NULL;
    }

    if (num_responses)
    {
        if (esp32_spi_check_data(*num_responses) != 0)
        {
#if ESP32_SPI_DEBUG
            printk("esp32_spi_check_data num_responses error\r\n");
#endif
            gpiohs_set_pin(cs_num, 1);
            return NULL;
        }
        num_of_resp = *num_responses;
    }
    else
    {
        num_of_resp = esp32_spi_read_byte();
    }

    esp32_spi_params_t *params_ret = (esp32_spi_params_t *)malloc(sizeof(esp32_spi_params_t));

    params_ret->del = delete_esp32_spi_params;
    params_ret->params_num = num_of_resp;
    params_ret->params = (void *)malloc(sizeof(void *) * num_of_resp);

    for (uint32_t i = 0; i < num_of_resp; i++)
    {
        params_ret->params[i] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        params_ret->params[i]->param_len = esp32_spi_read_byte();

        if (param_len_16)
        {
            params_ret->params[i]->param_len <<= 8;
            params_ret->params[i]->param_len |= esp32_spi_read_byte();
        }

#if (ESP32_SPI_DEBUG >= 2)
        printk("\tParameter #%d length is %d\r\n", i, params_ret->params[i]->param_len);
#endif

        params_ret->params[i]->param = (uint8_t *)malloc(sizeof(uint8_t) * params_ret->params[i]->param_len);
        esp32_spi_read_bytes(params_ret->params[i]->param, params_ret->params[i]->param_len);
    }

    if (esp32_spi_check_data(END_CMD) != 0)
    {
#if ESP32_SPI_DEBUG
        printk("esp32_spi_check_data END_CMD error\r\n");
#endif
        gpiohs_set_pin(cs_num, 1);
        return NULL;
    }

    gpiohs_set_pin(cs_num, 1);

    return params_ret;
}

esp32_spi_params_t *esp32_spi_send_command_get_response(uint8_t cmd, esp32_spi_params_t *params, uint32_t *num_resp, uint8_t sent_param_len_16, uint8_t recv_param_len_16)
{
    uint32_t resp_num;

    if (!num_resp)
        resp_num = 1;
    else
        resp_num = *num_resp;

    esp32_spi_send_command(cmd, params, sent_param_len_16);
    return esp32_spi_wait_response_cmd(cmd, &resp_num, recv_param_len_16);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void delete_esp32_spi_params(void *arg)
{
    esp32_spi_params_t *params = (esp32_spi_params_t *)arg;

    for (uint8_t i = 0; i < params->params_num; i++)
    {
        esp32_spi_param_t *param = params->params[i];
        free(param->param);
        free(param);
    }
    free(params->params);
    free(params);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
The status of the ESP32 WiFi core. Can be WL_NO_SHIELD or WL_NO_MODULE
        (not found), WL_IDLE_STATUS, WL_NO_SSID_AVAIL, WL_SCAN_COMPLETED,
        WL_CONNECTED, WL_CONNECT_FAILED, WL_CONNECTION_LOST, WL_DISCONNECTED,
        WL_AP_LISTENING, WL_AP_CONNECTED, WL_AP_FAILED
-1 error
other stat
*/
int8_t esp32_spi_status(void)
{
#if (ESP32_SPI_DEBUG > 1)
    printk("Connection status\r\n");
#endif

    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_CONN_STATUS_CMD, NULL, NULL, 0, 0);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -2;
    }
    int8_t ret = (int8_t)resp->params[0]->param[0];

#if ESP32_SPI_DEBUG
    printk("Conn Connection: %s\r\n", wlan_enum_to_str(ret));
#endif

    resp->del(resp);
    return ret;
}

/// A string of the firmware version on the ESP32
//NULL error
//other ok
char *esp32_spi_firmware_version(char* fw_version)
{
#if ESP32_SPI_DEBUG
    printk("Firmware version\r\n");
#endif

    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_FW_VERSION_CMD, NULL, NULL, 0, 0);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return NULL;
    }

    uint8_t ret_len = resp->params[0]->param_len;
    memcpy(fw_version, resp->params[0]->param, ret_len);
    fw_version[ret_len] = 0;

    resp->del(resp);
    return fw_version;
}

// make params struct with one param
static esp32_spi_params_t *esp32_spi_params_alloc_1param(uint32_t len, uint8_t *buf)
{
    esp32_spi_params_t *ret = (esp32_spi_params_t *)malloc(sizeof(esp32_spi_params_t));

    ret->del = delete_esp32_spi_params;

    ret->params_num = 1;
    ret->params = (void *)malloc(sizeof(void *) * ret->params_num);
    ret->params[0] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
    ret->params[0]->param_len = len;
    ret->params[0]->param = (uint8_t *)malloc(sizeof(uint8_t) * len);
    memcpy(ret->params[0]->param, buf, len);

    return ret;
}

// make params struct with two param
static esp32_spi_params_t *esp32_spi_params_alloc_2param(uint32_t len_0, uint8_t *buf_0, uint32_t len_1, uint8_t *buf_1)
{
    esp32_spi_params_t *ret = (esp32_spi_params_t *)malloc(sizeof(esp32_spi_params_t));

    ret->del = delete_esp32_spi_params;

    ret->params_num = 2;
    ret->params = (void *)malloc(sizeof(void *) * ret->params_num);
    //
    ret->params[0] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
    ret->params[0]->param_len = len_0;
    ret->params[0]->param = (uint8_t *)malloc(sizeof(uint8_t) * len_0);
    memcpy(ret->params[0]->param, buf_0, len_0);
    //
    ret->params[1] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
    ret->params[1]->param_len = len_1;
    ret->params[1]->param = (uint8_t *)malloc(sizeof(uint8_t) * len_1);
    memcpy(ret->params[1]->param, buf_1, len_1);

    return ret;
}

/// A bytearray containing the MAC address of the ESP32
//NULL error
//other ok
uint8_t *esp32_spi_MAC_address(void)
{
#if ESP32_SPI_DEBUG
    printk("MAC address\r\n");
#endif

    uint8_t data = 0xff;

    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &data);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_MACADDR_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return NULL;
    }

    uint8_t ret_len = resp->params[0]->param_len;
    memcpy(mac, resp->params[0]->param, ret_len);

    resp->del(resp);
    return mac;
}

/*
Begin a scan of visible access points. Follow up with a call
        to 'get_scan_networks' for response
-1 error
0 ok 
*/
int8_t esp32_spi_start_scan_networks(void)
{
#if ESP32_SPI_DEBUG
    printk("Start scan\r\n");
#endif

    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(START_SCAN_NETWORKS, NULL, NULL, 0, 0);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    if (resp->params[0]->param[0] != 1)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to start AP scan\r\n");
#endif
        resp->del(resp);
        return -1;
    }

    resp->del(resp);
    return 0;
}

static void delete_esp32_spi_aps_list(void *arg)
{
    esp32_spi_aps_list_t *aps = (esp32_spi_aps_list_t *)arg;

    for (uint8_t i = 0; i < aps->aps_num; i++)
    {
        free(aps->aps[i]);
    }
    free(aps->aps);
    free(aps);
}

/*
The results of the latest SSID scan. Returns a list of dictionaries with
        'ssid', 'rssi' and 'encryption' entries, one for each AP found
-1 error
other ok
*/
esp32_spi_aps_list_t *esp32_spi_get_scan_networks(void)
{

    esp32_spi_send_command(SCAN_NETWORKS, NULL, 0);
    esp32_spi_params_t *resp = esp32_spi_wait_response_cmd(SCAN_NETWORKS, NULL, 0);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return NULL;
    }

    esp32_spi_aps_list_t *aps = (esp32_spi_aps_list_t *)malloc(sizeof(esp32_spi_aps_list_t));
    aps->del = delete_esp32_spi_aps_list;

    aps->aps_num = resp->params_num;
    aps->aps = (void *)malloc(sizeof(void *) * aps->aps_num);

    for (uint32_t i = 0; i < aps->aps_num; i++)
    {
        aps->aps[i] = (esp32_spi_ap_t *)malloc(sizeof(esp32_spi_ap_t));
        memcpy(aps->aps[i]->ssid, resp->params[i]->param, (resp->params[i]->param_len > 32) ? 32 : resp->params[i]->param_len);
        aps->aps[i]->ssid[resp->params[i]->param_len] = 0;

        uint8_t data = i;
        esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &data);

        esp32_spi_params_t *rssi = esp32_spi_send_command_get_response(GET_IDX_RSSI_CMD, send, NULL, 0, 0);

        aps->aps[i]->rssi = (int8_t)(rssi->params[0]->param[0]);
#if ESP32_SPI_DEBUG
	printk("\tSSID:%s", aps->aps[i]->ssid);
        printk("\t\t\trssi:%02x\r\n", rssi->params[0]->param[0]);
#endif
        rssi->del(rssi);

        esp32_spi_params_t *encr = esp32_spi_send_command_get_response(GET_IDX_ENCT_CMD, send, NULL, 0, 0);
        aps->aps[i]->encr = encr->params[0]->param[0];
        encr->del(encr);

        send->del(send);
    }
    resp->del(resp);

    return aps;
}

/*
Scan for visible access points, returns a list of access point details.
         Returns a list of dictionaries with 'ssid', 'rssi' and 'encryption' entries,
         one for each AP found
*/
esp32_spi_aps_list_t *esp32_spi_scan_networks(void)
{
    if (esp32_spi_start_scan_networks() != 0)
    {
#if ESP32_SPI_DEBUG
        printk("esp32_spi_start_scan_networks failed \r\n");
#endif
        return NULL;
    }

    esp32_spi_aps_list_t *retaps = esp32_spi_get_scan_networks();

    if (retaps == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get retaps error!\r\n", __func__);
#endif
        return NULL;
    }

#if (ESP32_SPI_DEBUG >= 2)
    for (uint32_t i = 0; i < retaps->aps_num; i++)
    {
        printk("\t#%d %s RSSI: %d ENCR:%d\r\n", i, retaps->aps[i]->ssid, retaps->aps[i]->rssi, retaps->aps[i]->encr);
    }
#endif
    //need free in call
    return retaps;
}

/**
 Tells the ESP32 to set the access point to the given ssid
 -1 error
  0 ok
 */
int8_t esp32_spi_wifi_set_network(uint8_t *ssid)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(strlen((const char*)ssid), ssid);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(SET_NET_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    if (resp->params[0]->param[0] != 1)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to set network\r\n");
#endif
        resp->del(resp);
        return -1;
    }

    resp->del(resp);

    return 0;
}

/*
Sets the desired access point ssid and passphrase
-1 error
0 ok
*/
int8_t esp32_spi_wifi_wifi_set_passphrase(uint8_t *ssid, uint8_t *passphrase)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_2param(strlen((const char*)ssid), ssid, strlen((const char*)passphrase), passphrase);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(SET_PASSPHRASE_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    if (resp->params[0]->param[0] != 1)
    {
#if ESP32_SPI_DEBUG
        printk("%s: Failed to set passphrase\r\n", __func__);
#endif
        resp->del(resp);
        return -1;
    }

    resp->del(resp);
    return 0;
}

/*
The name of the access point we're connected to
NULL error
other ok
*/
char *esp32_spi_get_ssid(void)
{

    uint8_t data = 0xff;

    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &data);

    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_CURR_SSID_CMD, send, NULL, 0, 0);

    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return NULL;
    }

#if (ESP32_SPI_DEBUG >= 2)
    printk("connect ssid:%s\r\n", resp->params[0]->param);
#endif

    int8_t ret_len = resp->params[0]->param_len;
    memcpy(ssid, resp->params[0]->param, ret_len);

    resp->del(resp);
    return ssid;
}

/*
The receiving signal strength indicator for the access point we're connected to
-1 error
other ok
*/
int8_t esp32_spi_get_rssi(void)
{
    uint8_t data = 0xff;
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &data);

    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_CURR_RSSI_CMD, send, NULL, 0, 0);

    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    int8_t r = (int8_t)(resp->params[0]->param[0]);
    resp->del(resp);

#if ESP32_SPI_DEBUG
    printk("connect rssi:%d\r\n", r);
#endif

    return r;
}

/*
A dictionary containing current connection details such as the 'ip_addr',
        'netmask' and 'gateway
    net_data[0]     ip_addr
    net_data[1]     netmask
    net_data[2]     gateway
*/
// NULL error
// other ok
esp32_spi_net_t *esp32_spi_get_network_data(void)
{
    uint8_t data = 0xff;
    uint32_t num_resp = 3;

    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &data);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_IPADDR_CMD, send, &num_resp, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return NULL;
    }

    if (resp->params_num != 3)
    {
#if ESP32_SPI_DEBUG
        printk("%s: resp->params_num  error!\r\n", __func__);
#endif
        resp->del(resp);
        return NULL;
    }

    memcpy(net_dat.localIp, resp->params[0]->param, resp->params[0]->param_len);
    memcpy(net_dat.subnetMask, resp->params[1]->param, resp->params[1]->param_len);
    memcpy(net_dat.gatewayIp, resp->params[2]->param, resp->params[2]->param_len);

    resp->del(resp);

    return &net_dat;
}

/// Our local IP address
//use esp32_spi_get_network_data to get
int8_t esp32_spi_ip_address(uint8_t *net_data)
{
    return 0;
}

//Whether the ESP32 is connected to an access point
// 0 not connect
// 1 connected
uint8_t esp32_spi_is_connected(void)
{
    int8_t stat = esp32_spi_status();

    if (stat == -2)
    {
#if ESP32_SPI_DEBUG
        printk("%s get status error \r\n", __func__);
#endif
        esp32_spi_reset();
        return 2;
    }
    else if (stat == WL_CONNECTED)
        return 0;

    return 1;
}

//Connect to an access point using a secrets dictionary
//  that contains a 'ssid' and 'password' entry
void esp32_spi_connect(uint8_t *secrets)
{
    // printk("%s not Support Yet!\r\n", __func__);
    return;
}

//Connect to an access point with given name and password.
//      Will retry up to 10 times and return on success or raise
//      an exception on failure
//-1 connect failed
//0 connect succ
int8_t esp32_spi_connect_AP(uint8_t *ssid, uint8_t *password, uint8_t retry_times)
{
#if ESP32_SPI_DEBUG
    printk("Connect to AP--> ssid: %s password:%s\r\n", ssid, password);
#endif
    if (password)
        esp32_spi_wifi_wifi_set_passphrase(ssid, password);
    else
        esp32_spi_wifi_set_network(ssid);

    int8_t stat = -1;

    for (uint8_t i = 0; i < retry_times; i++)
    {
        stat = esp32_spi_status();

        if (stat == -1)
        {
#if ESP32_SPI_DEBUG
            printk("%s get status error \r\n", __func__);
#endif
            esp32_spi_reset();
            return -1;
        }
        else if (stat == WL_CONNECTED)
            return 0;
        else if (stat == WL_CONNECT_FAILED)
            return -2;
        sleep(1);
    }
    stat = esp32_spi_status();

    if (stat == WL_CONNECT_FAILED || stat == WL_CONNECTION_LOST || stat == WL_DISCONNECTED)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to connect to ssid: %s\r\n", ssid);
#endif

        return -3;
    }

    if (stat == WL_NO_SSID_AVAIL)
    {
#if ESP32_SPI_DEBUG
        printk("No such ssid: %s\r\n", ssid);
#endif

        return -4;
    }

#if ESP32_SPI_DEBUG
    printk("Unknown error 0x%02X", stat);
#endif

    return -5;
}

int8_t esp32_spi_disconnect_from_AP(void)
{
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(DISCONNECT_CMD, NULL, NULL, 0, 0);
    if (resp == NULL)
    {
        return -1;
    }
    int8_t ret = (int8_t)resp->params[0]->param[0];
    resp->del(resp);
    return ret;
}

//Converts a bytearray IP address to a dotted-quad string for printing
void esp32_spi_pretty_ip(uint8_t *ip, uint8_t *str_ip)
{
    sprintf((char*)str_ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return;
}

void esp32_spi_unpretty_ip(uint8_t *ip)
{
    // printk("%s: Not Support Yet!\r\n", __func__);
    return;
}

//Convert a hostname to a packed 4-byte IP address. Returns a 4 bytearray
//-1 error
//0 ok
int esp32_spi_get_host_by_name(uint8_t *hostname, uint8_t *ip)
{
#if ESP32_SPI_DEBUG
    printk("*** Get host by name\r\n");
#endif

    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(strlen((const char*)hostname), hostname);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(REQ_HOST_BY_NAME_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return EIO;
    }

    if (resp->params[0]->param[0] != 1)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to request hostname\r\n");
#endif
        resp->del(resp);
        return EINVAL;
    }
    resp->del(resp);

    resp = esp32_spi_send_command_get_response(GET_HOST_BY_NAME_CMD, NULL, NULL, 0, 0);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return EIO;
    }

#if (ESP32_SPI_DEBUG >= 2)
    printk("get_host_by_name:%s-->", hostname);
    for (uint8_t i = 0; i < resp->params[0]->param_len; i++)
    {
        printk("%d", resp->params[0]->param[i]);
        if (i < resp->params[0]->param_len - 1)
            printk(".");
    }
    printk("\r\n");
#endif

    memcpy(ip, resp->params[0]->param, resp->params[0]->param_len);
    resp->del(resp);

    return 0;
}

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

// Ping a destination IP address or hostname, with a max time-to-live
//      (ttl). Returns a millisecond timing value
// dest_type
//          0 ip array
//          1 hostname
//-1 error
//time
int32_t esp32_spi_ping(uint8_t *dest, uint8_t dest_type, uint8_t ttl)
{
    uint8_t sttl = 0;
    sttl = MAX(0, MIN(ttl, 255));

#if ESP32_SPI_DEBUG
    printk("sttl:%d\r\n", sttl);
#endif

    uint8_t dest_array[6];
    if (dest_type)
    {
        if (esp32_spi_get_host_by_name(dest, dest_array) != 0)
        {
#if ESP32_SPI_DEBUG
            printk("get host by name error\r\n");
#endif
            return -2;
        }
    }
    else
    {
        memcpy(dest_array, dest, 4);
    }

    esp32_spi_params_t *send = esp32_spi_params_alloc_2param(4, dest_array, 1, &sttl);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(PING_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    uint16_t time = resp->params[0]->param[1] << 8 | resp->params[0]->param[0];

#if ESP32_SPI_DEBUG
    printk("time:%dms\r\n", time);
#endif

    resp->del(resp);

    return time;
}

// Request a socket from the ESP32, will allocate and return a number that can then be passed to the other socket commands
// 0xff error
// other ok
uint8_t esp32_spi_get_socket(void)
{
#if ESP32_SPI_DEBUG
    printk("*** Get socket\r\n");
#endif
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_SOCKET_CMD, NULL, NULL, 0, 0);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return 0xff;
    }

    uint8_t socket = resp->params[0]->param[0];

    if (socket == 255)
    {
#if ESP32_SPI_DEBUG
        printk("No sockets available\r\n");
#endif
        resp->del(resp);
        return 0xff;
    }

#if ESP32_SPI_DEBUG
    printk("Allocated socket #%d\r\n", socket);
#endif

    resp->del(resp);
    return (int16_t)socket;
}

/*
Open a socket to a destination IP address or hostname
        using the ESP32's internal reference number. By default we use
        'conn_mode' TCP_MODE but can also use UDP_MODE or TLS_MODE
        (dest must be hostname for TLS_MODE!)
dest_type 
            0 ip array
            1 hostname
!!! hostname test failed
-1 error
0 ok
*/
int8_t esp32_spi_socket_open(uint8_t sock_num, uint8_t *dest, uint8_t dest_type, uint16_t port, esp32_socket_mode_enum_t conn_mode)
{
    uint8_t port_arr[2];

    port_arr[0] = (uint8_t)(port >> 8);
    port_arr[1] = (uint8_t)(port);

#if (ESP32_SPI_DEBUG > 1)
    printk("port: 0x%02x 0x%02x\r\n", port_arr[0], port_arr[1]);
#endif

    esp32_spi_params_t *send = (esp32_spi_params_t *)malloc(sizeof(esp32_spi_params_t));
    send->del = delete_esp32_spi_params;

    uint32_t param_len = 0;

    if (dest_type)
    {
        send->params_num = 5;
        send->params = (void *)malloc(sizeof(void *) * send->params_num);
        //
        param_len = strlen((const char*)dest);
        send->params[0] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[0]->param_len = param_len;
        send->params[0]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        memcpy(send->params[0]->param, dest, param_len);
        //
        param_len = 4;
        send->params[1] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[1]->param_len = param_len;
        send->params[1]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        memset(send->params[1]->param, 0, param_len);
        //
        param_len = 2;
        send->params[2] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[2]->param_len = param_len;
        send->params[2]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        memcpy(send->params[2]->param, port_arr, param_len);
        //
        param_len = 1;
        send->params[3] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[3]->param_len = param_len;
        send->params[3]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        send->params[3]->param[0] = sock_num;
        //
        param_len = 1;
        send->params[4] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[4]->param_len = param_len;
        send->params[4]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        send->params[4]->param[0] = conn_mode;
    }
    else
    {
        send->params_num = 4;
        send->params = (void *)malloc(sizeof(void *) * send->params_num);
        //
        param_len = 4;
        send->params[0] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[0]->param_len = param_len;
        send->params[0]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        memcpy(send->params[0]->param, dest, param_len);
        //
        param_len = 2;
        send->params[1] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[1]->param_len = param_len;
        send->params[1]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        memcpy(send->params[1]->param, port_arr, param_len);
        //
        param_len = 1;
        send->params[2] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[2]->param_len = param_len;
        send->params[2]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        send->params[2]->param[0] = sock_num;
        //
        param_len = 1;
        send->params[3] = (esp32_spi_param_t *)malloc(sizeof(esp32_spi_param_t));
        send->params[3]->param_len = param_len;
        send->params[3]->param = (uint8_t *)malloc(sizeof(uint8_t) * param_len);
        send->params[3]->param[0] = conn_mode;
    }

    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(START_CLIENT_TCP_CMD, send, NULL, 0, 0);

    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    if (resp->params[0]->param[0] != 1)
    {
#if ESP32_SPI_DEBUG
        printk("Could not connect to remote server\r\n");
#endif
        resp->del(resp);

        return -1;
    }
    resp->del(resp);
    return 0;
}

//Get the socket connection status, can be SOCKET_CLOSED, SOCKET_LISTEN,
//        SOCKET_SYN_SENT, SOCKET_SYN_RCVD, SOCKET_ESTABLISHED, SOCKET_FIN_WAIT_1,
//        SOCKET_FIN_WAIT_2, SOCKET_CLOSE_WAIT, SOCKET_CLOSING, SOCKET_LAST_ACK, or
//        SOCKET_TIME_WAIT
// -1 error
// enum ok
esp32_socket_enum_t esp32_spi_socket_status(uint8_t socket_num)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &socket_num);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_CLIENT_STATE_TCP_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return 0xff;
    }
    esp32_socket_enum_t ret;

    ret = (esp32_socket_enum_t)resp->params[0]->param[0];

    resp->del(resp);

#if (ESP32_SPI_DEBUG > 1)
    printk("sock stat :%d\r\n", ret);
#endif
    return ret;
}

//Test if a socket is connected to the destination, returns boolean true/false
uint8_t esp32_spi_socket_connected(uint8_t socket_num)
{
    return (esp32_spi_socket_status(socket_num) == SOCKET_ESTABLISHED);
}

//Write the bytearray buffer to a socket
//0 error
//len ok
uint32_t esp32_spi_socket_write(uint8_t socket_num, uint8_t *buffer, uint16_t len)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_2param(1, &socket_num, len, buffer);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(SEND_DATA_TCP_CMD, send, NULL, 1, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return 0;
    }
    uint16_t sent = ( ((uint16_t)(resp->params[0]->param[1]) << 8) & 0xff00 ) | (uint16_t)(resp->params[0]->param[0]);
    // if (sent != len) //TODO: the firmware is nonblock, so return value maybe < len
    if (sent == 0)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to send %d bytes (sent %d)", len, sent);
#endif
        resp->del(resp);
        return 0;
    }

//     resp->del(resp);

//     send = esp32_spi_params_alloc_1param(1, &socket_num);
//     resp = esp32_spi_send_command_get_response(DATA_SENT_TCP_CMD, send, NULL, 0, 0);
//     send->del(send);

//     if (resp == NULL)
//     {
//  #if ESP32_SPI_DEBUG
//         printk("%s: get resp error!\r\n", __func__);
//  #endif
//         return 0;
//     }

//     if (resp->params[0]->param[0] != 1)
//     {
//  #if ESP32_SPI_DEBUG
//         printk("%s: Failed to verify data sent\r\n", __func__);
//  #endif
//         resp->del(resp);
//         return 0;
//     }

    resp->del(resp);
    return sent;
}
int8_t esp32_spi_add_udp_data(uint8_t socket_num, uint8_t* data, uint16_t data_len)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_2param(1, &socket_num, data_len, data );
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(ADD_UDP_DATA_CMD, send, NULL, 1, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("Failed  get response\r\n");
#endif
        return -2;
    }

    uint8_t ok = resp->params[0]->param[0];

    if (ok != 1)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to sendto\r\n");
#endif
        resp->del(resp);
        return -1;
    }

    resp->del(resp);
    return 0;
}

int8_t esp32_spi_send_udp_data(uint8_t socket_num)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &socket_num);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(SEND_UDP_DATA_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("Failed  get response\r\n");
#endif
        return -2;
    }

    uint8_t ok = resp->params[0]->param[0];

    if (ok != 1)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to send udp data\r\n");
#endif
        resp->del(resp);
        return -1;
    }

    resp->del(resp);
    return 0;
}

//Determine how many bytes are waiting to be read on the socket
int esp32_spi_socket_available(uint8_t socket_num)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &socket_num);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(AVAIL_DATA_TCP_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    int reply = 0;

    reply = (int)((uint16_t)(resp->params[0]->param[1] << 8) | (uint16_t)(resp->params[0]->param[0]));

#if ESP32_SPI_DEBUG
    if(reply > 0)
        printk("ESPSocket: %d bytes available\r\n", reply);
#endif

    resp->del(resp);
    return reply;
}

//Read up to 'size' bytes from the socket number. Returns a bytearray
int esp32_spi_socket_read(uint8_t socket_num, uint8_t *buff, uint16_t size)
{
#if ESP32_SPI_DEBUG
    printk("Reading %d bytes from ESP socket with status %s\r\n", size, socket_enum_to_str(esp32_spi_socket_status(socket_num)));
#endif

    uint8_t len[2];

    len[0] = (uint8_t)(size & 0xff);
    len[1] = (uint8_t)((size >> 8) & 0xff);

#if (ESP32_SPI_DEBUG > 2)
    printk("len_0:%02x\tlen_1:%02x\r\n", len[0], len[1]);
#endif

    esp32_spi_params_t *send = esp32_spi_params_alloc_2param(1, &socket_num, 2, len);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_DATABUF_TCP_CMD, send, NULL, 1, 1);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

#if 0
    if (resp->params[0]->param_len != size)
    {
#if ESP32_SPI_DEBUG
        printk("read error len\r\n");
#endif
        resp->del(resp);
        return -1;
    }
#endif

    uint16_t real_read_size = resp->params[0]->param_len;

    if (real_read_size != 0)
        memcpy(buff, resp->params[0]->param, real_read_size);
    resp->del(resp);

    return real_read_size;
}

int8_t esp32_spi_get_remote_info(uint8_t socket_num, uint8_t* ip, uint16_t* port)
{
    uint32_t recv_num = 2;
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &socket_num);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_REMOTE_INFO_CMD, send, &recv_num, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
        return -1;
    }
    if(resp->params_num!=2 || resp->params[0]->param_len != 4 || resp->params[1]->param_len != 2)
    {
        resp->del(resp);
        return -1;
    }
    memcpy(ip, resp->params[0]->param, 4);
    *port = ( ((uint16_t)resp->params[1]->param[0])<<8 | resp->params[1]->param[1]);
    resp->del(resp);
    return 0;
}

// Open and verify we connected a socket to a destination IP address or hostname
//         using the ESP32's internal reference number. By default we use
//         'conn_mode' TCP_MODE but can also use UDP_MODE or TLS_MODE (dest must
//         be hostname for TLS_MODE!)
// dest_type
//             0 ip array
//             1 hostname
// !!! hostname test failed
//-1 error
//0 ok
int8_t esp32_spi_socket_connect(uint8_t socket_num, uint8_t *dest, uint8_t dest_type, uint16_t port, esp32_socket_mode_enum_t conn_mod)
{
#if ESP32_SPI_DEBUG
    printk("*** Socket connect mode:%d\r\n", conn_mod);
#endif
    int8_t ret = esp32_spi_socket_open(socket_num, dest, dest_type, port, conn_mod);
    if ( ret == -2 )
    {
        return -2;
    }
    if(ret == -1)
    {
        return -1;
    }
    if(conn_mod == UDP_MODE)
        return 0;
    uint64_t tm = sysctl_get_time_us();

    while ((sysctl_get_time_us() - tm) < 3 * 1000 * 1000) //3s
    {
        uint8_t ret = esp32_spi_socket_status(socket_num);
        if (ret == SOCKET_ESTABLISHED)
            return 0;
        else if(ret == 0xff) // EIO
        {
            return -2;
        }
        // msleep(100);
    }
    return -3;
}

// Close a socket using the ESP32's internal reference number
//-1 error
//0 ok
int8_t esp32_spi_socket_close(uint8_t socket_num)
{
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(1, &socket_num);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(STOP_CLIENT_TCP_CMD, send, NULL, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }

    if (resp->params[0]->param[0] != 1)
    {

#if ESP32_SPI_DEBUG
        printk("Failed to close socket\r\n");
#endif
        resp->del(resp);
        return -1;
    }
    resp->del(resp);
    return 0;
}


//PIN36 PIN39 PIN34 PIN35 PIN32
//get esp32 adc val
//0 ok
//-1 error
int8_t esp32_spi_get_adc_val(uint8_t* channels, uint8_t len, uint16_t *val)
{
    if (val == NULL || len == 0)
    {
        return -1;
    }

    uint32_t num = len;
    esp32_spi_params_t *send = esp32_spi_params_alloc_1param(len, channels);
    esp32_spi_params_t *resp = esp32_spi_send_command_get_response(GET_ADC_VAL_CMD, send, &num, 0, 0);
    send->del(send);

    if (resp == NULL)
    {
#if ESP32_SPI_DEBUG
        printk("%s: get resp error!\r\n", __func__);
#endif
        return -1;
    }
    if (resp->params_num != len)
    {
#if ESP32_SPI_DEBUG
        printk("Failed to sample adc\r\n");
#endif
        resp->del(resp);
        return -1;
    }

    for (uint8_t i = 0; i < len; i++)
    {
        if (resp->params[i]->param_len != 3)
        {
#if ESP32_SPI_DEBUG
            printk("adc val len error\r\n");
#endif
            resp->del(resp);
            return -1;
        }
        *(val + i) = resp->params[i]->param[1] << 8 | resp->params[i]->param[2];
    }
    resp->del(resp);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
#define ENUM_TO_STR(x) \
    case (x):          \
        return (#x)

char *socket_enum_to_str(esp32_socket_enum_t x)
{
    switch (x)
    {
        ENUM_TO_STR(SOCKET_CLOSED);
        ENUM_TO_STR(SOCKET_LISTEN);
        ENUM_TO_STR(SOCKET_SYN_SENT);
        ENUM_TO_STR(SOCKET_SYN_RCVD);
        ENUM_TO_STR(SOCKET_ESTABLISHED);
        ENUM_TO_STR(SOCKET_FIN_WAIT_1);
        ENUM_TO_STR(SOCKET_FIN_WAIT_2);
        ENUM_TO_STR(SOCKET_CLOSE_WAIT);
        ENUM_TO_STR(SOCKET_CLOSING);
        ENUM_TO_STR(SOCKET_LAST_ACK);
        ENUM_TO_STR(SOCKET_TIME_WAIT);
    }
    return "unknown";
}

char *wlan_enum_to_str(esp32_wlan_enum_t x)
{
    switch (x)
    {
        ENUM_TO_STR(WL_IDLE_STATUS);
        ENUM_TO_STR(WL_NO_SSID_AVAIL);
        ENUM_TO_STR(WL_SCAN_COMPLETED);
        ENUM_TO_STR(WL_CONNECTED);
        ENUM_TO_STR(WL_CONNECT_FAILED);
        ENUM_TO_STR(WL_CONNECTION_LOST);
        ENUM_TO_STR(WL_DISCONNECTED);
        ENUM_TO_STR(WL_AP_LISTENING);
        ENUM_TO_STR(WL_AP_CONNECTED);
        ENUM_TO_STR(WL_AP_FAILED);
        ENUM_TO_STR(WL_NO_MODULE);
    }
    return "unknown";
}

//-1 error
//sock ok
uint8_t connect_server_port(char *host, uint16_t port)
{
    uint8_t sock = esp32_spi_get_socket();

    if (sock != 0xff)
    {
        uint8_t ip[6];
        if (esp32_spi_get_host_by_name((uint8_t*)host, ip) == 0)
        {
            if (esp32_spi_socket_connect(sock, ip, 0, port, TCP_MODE) != 0)
            {
                return 0xff;
            }
        }
        else
        {
            return 0xff;
        }
    }
    else
    {
        return 0xff;
    }
    return sock;
}
