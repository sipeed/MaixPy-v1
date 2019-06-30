#include "ps2.h"
#include "sleep.h"
#include "sysctl.h"
#include "gpiohs.h"
#include "fpioa.h"

static uint8_t cs_num, clk_num, mosi_num, miso_num;

static uint64_t last_read = 0;
static uint64_t read_delay = 0;
static uint8_t controller_type = 0;
static uint8_t en_Rumble = 0;
static uint8_t en_Pressures = 0;

static uint8_t PS2data[21];
static unsigned int last_buttons = 0;
static unsigned int buttons = 0;

static uint8_t enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
static uint8_t set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
static uint8_t set_bytes_large[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00};
static uint8_t exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};
static uint8_t enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
static uint8_t type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A};

/****************************************************************************************/
static void PS2X_sendCommandString(uint8_t *data, uint8_t len);

/****************************************************************************************/
static inline void PS2X_CLK_SET(void)
{
    gpiohs_set_pin(clk_num, 1);
}

static inline void PS2X_CLK_CLR(void)
{
    gpiohs_set_pin(clk_num, 0);
}

static inline void PS2X_CMD_SET(void)
{
    gpiohs_set_pin(mosi_num, 1);
}

static inline void PS2X_CMD_CLR(void)
{
    gpiohs_set_pin(mosi_num, 0);
}

static inline void PS2X_CS_SET(void)
{
    gpiohs_set_pin(cs_num, 1);
}

static inline void PS2X_CS_CLR(void)
{
    gpiohs_set_pin(cs_num, 0);
}

static inline uint8_t PS2X_DAT_CHK(void)
{
    return gpiohs_get_pin(miso_num);
}

static uint8_t PS2X_gamepad_shiftinout(char data)
{
    uint8_t tmp = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (CHK(data, i))
            PS2X_CMD_SET(); //mosi
        else
            PS2X_CMD_CLR(); //mosi

        PS2X_CLK_CLR(); //clk
        usleep(CTRL_CLK);

        if (PS2X_DAT_CHK())
            tmp += (1 << i);

        PS2X_CLK_SET(); //clk
        usleep(CTRL_CLK_HIGH);
    }
    PS2X_CMD_SET(); //mosi
    usleep(CTRL_BYTE_DELAY);
    return tmp;
}

void PS2X_confg_io(uint8_t cs, uint8_t clk, uint8_t mosi, uint8_t miso)
{
    cs_num = cs;
    clk_num = clk;
    mosi_num = mosi ; 
    miso_num  = miso;

    //cs
    gpiohs_set_drive_mode(cs_num, GPIO_DM_OUTPUT);
    gpiohs_set_pin(cs_num, 0);
    //clk
    gpiohs_set_drive_mode(clk_num, GPIO_DM_OUTPUT);
    gpiohs_set_pin(clk_num, 1);
    //mosi
    gpiohs_set_drive_mode(mosi_num, GPIO_DM_OUTPUT);
    gpiohs_set_pin(mosi_num, 1);
    //miso
    gpiohs_set_drive_mode(miso_num, GPIO_DM_INPUT_PULL_UP);
}
/****************************************************************************************/

uint8_t PS2X_Analog(uint8_t button)
{
    return PS2data[button];
}

uint8_t PS2X_Button(uint16_t button)
{
    return ((~buttons & button) > 0);
}

//motor1 motor2 normal 0x0
/****************************************************************************************/
uint8_t PS2X_read_gamepad(uint8_t motor1, uint8_t motor2)
{
    // uint64_t tm0 = sysctl_get_time_us();

    uint64_t temp = sysctl_get_time_us() - last_read;

    if (temp > 1500*1000) //waited to long
    {
        PS2X_reconfig_gamepad();
    }

    if (temp < read_delay) //waited too short
        usleep(read_delay - temp);

    if (motor2 != 0x00)
    {
        if (motor2 < 0x40)
                motor2 = 0x40;
    }

    char dword[9] = {0x01, 0x42, 0, motor1, motor2, 0, 0, 0, 0};
    uint8_t dword2[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // Try a few times to get valid data...
    for (uint8_t RetryCnt = 0; RetryCnt < 5; RetryCnt++)
    {
        PS2X_CMD_SET();
        PS2X_CLK_SET();
        PS2X_CS_CLR(); // low enable joystick

        usleep(CTRL_BYTE_DELAY);
        //Send the command to send button and joystick data;
        for (int i = 0; i < 9; i++)
        {
            PS2data[i] = PS2X_gamepad_shiftinout(dword[i]);
        }

        if (PS2data[1] == 0x79)
        {
            //if controller is in full data return mode, get the rest of data
            for (int i = 0; i < 12; i++)
            {
                PS2data[i + 9] = PS2X_gamepad_shiftinout(dword2[i]);
            }
        }

        PS2X_CS_SET(); // HI disable joystick
        // Check to see if we received valid data or not.
        // We should be in analog mode for our data to be valid (analog == 0x7_)
        if ((PS2data[1] & 0xf0) == 0x70)
            break;

        // If we got to here, we are not in analog mode, try to recover...
        PS2X_reconfig_gamepad(); // try to get back into Analog mode.
        usleep(read_delay);
    }

    // If we get here and still not in analog mode (=0x7_), try increasing the read_delay...
    if ((PS2data[1] & 0xf0) != 0x70)
    {
        if (read_delay < 10)
            read_delay++; // see if this helps out...
    }

    last_buttons = buttons; //store the previous buttons states

    buttons = (uint16_t)(PS2data[4] << 8) + PS2data[3]; //store as one value for multiple functions

    last_read = sysctl_get_time_us();
    // mp_printf(&mp_plat_print, "%ld\r\n",sysctl_get_time_us()-tm0);
    return ((PS2data[1] & 0xf0) == 0x70); // 1 = OK = analog mode - 0 = NOK
}

/****************************************************************************************/
uint8_t PS2X_config_gamepad(uint8_t pressures, uint8_t rumble)
{
    uint8_t temp[sizeof(type_read)];

    PS2X_CMD_SET();
    PS2X_CLK_SET();

    //new error checking. First, read gamepad a few times to see if it's talking
    PS2X_read_gamepad(0, 0);
    PS2X_read_gamepad(0, 0);

    //see if it talked - see if mode came back.
    //If still anything but 41, 73 or 79, then it's not talking
    if (PS2data[1] != 0x41 && PS2data[1] != 0x73 && PS2data[1] != 0x79)
    {
        // mp_printf(&mp_plat_print, "Controller mode not matched or no controller found\r\n");
        // mp_printf(&mp_plat_print, "Expected 0x41, 0x73 or 0x79, but got 0x%x\r\n", PS2data[1]);
        return 1; //return error code 1
    }

    //try setting mode, increasing delays if need be.
    read_delay = 1;

    for (int y = 0; y <= 10; y++)
    {
        PS2X_sendCommandString(enter_config, sizeof(enter_config)); //start config run

        //read type
        usleep(CTRL_BYTE_DELAY);

        PS2X_CMD_SET();
        PS2X_CLK_SET();
        PS2X_CS_CLR(); // low enable joystick

        usleep(CTRL_BYTE_DELAY);

        for (int i = 0; i < 9; i++)
        {
            temp[i] = PS2X_gamepad_shiftinout(type_read[i]);
        }

        PS2X_CS_SET(); // HI disable joystick

        controller_type = temp[3];

        PS2X_sendCommandString(set_mode, sizeof(set_mode));
        if (rumble)
        {
            PS2X_sendCommandString(enable_rumble, sizeof(enable_rumble));
            en_Rumble = 1;
        }
        if (pressures)
        {
            PS2X_sendCommandString(set_bytes_large, sizeof(set_bytes_large));
            en_Pressures = 1;
        }
        PS2X_sendCommandString(exit_config, sizeof(exit_config));

        PS2X_read_gamepad(0, 0);

        if (pressures)
        {
            if (PS2data[1] == 0x79)
                break;
            if (PS2data[1] == 0x73)
                return 3;
        }

        if (PS2data[1] == 0x73)
            break;

        if (y == 10)
        {
            // mp_printf(&mp_plat_print, "Controller not accepting commands\r\n");
            // mp_printf(&mp_plat_print, "mode stil set at 0x%x\r\n", PS2data[1]);
            return 2; //exit function with error
        }
        read_delay += 1; //add 1ms to read_delay
    }
    last_read = sysctl_get_time_us();
    return 0; //no error if here
}

/****************************************************************************************/
static void PS2X_sendCommandString(uint8_t *data, uint8_t len)
{
#ifdef PS2X_COM_DEBUG
    uint8_t temp[len];
    PS2X_CS_CLR(); // low enable joystick
    msleep(CTRL_BYTE_DELAY);

    for (int y = 0; y < len; y++)
        temp[y] = PS2X_gamepad_shiftinout(data[y]);

    PS2X_CS_SET();      //high disable joystick
    msleep(read_delay); //wait a few

    // mp_printf(&mp_plat_print, "OUT:IN Configure");
    for (int i = 0; i < len; i++)
    {
        // mp_printf(&mp_plat_print, " 0x%x: 0x%x\r\n", data[i], temp[i]);
    }
    // mp_printf(&mp_plat_print, "\r\n");
#else
    PS2X_CS_CLR(); // low enable joystick
    for (int y = 0; y < len; y++)
        PS2X_gamepad_shiftinout(data[y]);
    PS2X_CS_SET();      //high disable joystick
    usleep(read_delay); //wait a few
#endif
}

/****************************************************************************************/
uint8_t PS2X_readType(void)
{
#ifdef PS2X_COM_DEBUG
  uint8_t temp[sizeof(type_read)];
  PS2X_sendCommandString(enter_config, sizeof(enter_config));
  msleep(CTRL_BYTE_DELAY);
  CMD_SET();
  CLK_SET();
  ATT_CLR(); // low enable joystick
  msleep(CTRL_BYTE_DELAY);
  for (int i = 0; i<9; i++) {
    temp[i] = PS2X_gamepad_shiftinout(type_read[i]);
  }
  PS2X_sendCommandString(exit_config, sizeof(exit_config));
  if(temp[3] == 0x03)
    return 1;
  else if(temp[3] == 0x01)
    return 2;
  return 0;
#else

    if (controller_type == 0x03)
        return 1;
    else if (controller_type == 0x01)
        return 2;
    else if (controller_type == 0x0C)
        return 3; //2.4G Wireless Dual Shock PS2 Game Controller

    return 0;
#endif
}

/****************************************************************************************/
void PS2X_enableRumble(void)
{
    PS2X_sendCommandString(enter_config, sizeof(enter_config));
    PS2X_sendCommandString(enable_rumble, sizeof(enable_rumble));
    PS2X_sendCommandString(exit_config, sizeof(exit_config));
    en_Rumble = 1;
}

/****************************************************************************************/
uint8_t PS2X_enablePressures(void)
{
    PS2X_sendCommandString(enter_config, sizeof(enter_config));
    PS2X_sendCommandString(set_bytes_large, sizeof(set_bytes_large));
    PS2X_sendCommandString(exit_config, sizeof(exit_config));

    PS2X_read_gamepad(0, 0);
    PS2X_read_gamepad(0, 0);

    if (PS2data[1] != 0x79)
        return 0;

    en_Pressures = 1;
    return 1;
}

/****************************************************************************************/
void PS2X_reconfig_gamepad(void)
{
    PS2X_sendCommandString(enter_config, sizeof(enter_config));
    PS2X_sendCommandString(set_mode, sizeof(set_mode));
    if (en_Rumble)
        PS2X_sendCommandString(enable_rumble, sizeof(enable_rumble));
    if (en_Pressures)
        PS2X_sendCommandString(set_bytes_large, sizeof(set_bytes_large));
    PS2X_sendCommandString(exit_config, sizeof(exit_config));
}
