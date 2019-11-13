
#include "i2c.h"
#include "sipeed_i2c.h"
#include "platform.h"
#include "sysctl.h"
#include "utils.h"
#include "math.h"
#include "sleep.h"

bool is_master_mode[I2C_DEVICE_MAX] = {false, false, false};
static uint16_t slaves_addr[I2C_DEVICE_MAX] = {0,0,0};

typedef struct _i2c_slave_context
{
    uint32_t i2c_num;
    const i2c_slave_handler_t *slave_handler;
} i2c_slave_context_t;

i2c_slave_context_t maix_slave_context[I2C_DEVICE_MAX];

static int maix_i2c_slave_irq(void *userdata);

static void i2c_clk_init(i2c_device_number_t i2c_num)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    sysctl_clock_enable(SYSCTL_CLOCK_I2C0 + i2c_num);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2C0 + i2c_num, 3);
}

void maix_i2c_init(i2c_device_number_t i2c_num, uint32_t address_width,
              uint32_t i2c_clk)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    configASSERT(address_width == 7 || address_width == 10);

    volatile i2c_t *i2c_adapter = i2c[i2c_num];
    uint8_t speed_mode = I2C_CON_SPEED_STANDARD;

    i2c_clk_init(i2c_num);

    uint32_t v_i2c_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_I2C0 + i2c_num);
    uint16_t v_period_clk_cnt = floor( (v_i2c_freq*1.0 / i2c_clk / 2) + 0.5 );

    if(v_period_clk_cnt <= 6)
        v_period_clk_cnt = 6;
    if(v_period_clk_cnt >= 65525)//65535-10
        v_period_clk_cnt = 65525;
    if((i2c_clk>100000) && (i2c_clk<=1000000))
        speed_mode = I2C_CON_SPEED_FAST;
    else
        speed_mode = I2C_CON_SPEED_HIGH;
    i2c_adapter->enable = 0;
    i2c_adapter->con = I2C_CON_MASTER_MODE | I2C_CON_SLAVE_DISABLE | I2C_CON_RESTART_EN |
                       (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) | I2C_CON_SPEED(speed_mode);
    i2c_adapter->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(v_period_clk_cnt);
    i2c_adapter->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(v_period_clk_cnt);

    i2c_adapter->intr_mask = 0;
    i2c_adapter->dma_cr = 0x3;
    i2c_adapter->dma_rdlr = 0;
    i2c_adapter->dma_tdlr = 4;
    i2c_adapter->enable = I2C_ENABLE_ENABLE;
    is_master_mode[i2c_num] = true;
}

void maix_i2c_init_as_slave(i2c_device_number_t i2c_num, uint32_t slave_address, uint32_t address_width,
    const i2c_slave_handler_t *handler)
{
    configASSERT(address_width == 7 || address_width == 10);
    volatile i2c_t *i2c_adapter = i2c[i2c_num];
    
    maix_slave_context[i2c_num].i2c_num = i2c_num;
    maix_slave_context[i2c_num].slave_handler = handler;

    i2c_clk_init(i2c_num);
    i2c_adapter->enable = 0;
    i2c_adapter->con = (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) | I2C_CON_SPEED(1) | I2C_CON_STOP_DET_IFADDRESSED;
    i2c_adapter->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(37);
    i2c_adapter->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(40);
    i2c_adapter->sar = I2C_SAR_ADDRESS(slave_address);
    i2c_adapter->rx_tl = I2C_RX_TL_VALUE(0);
    i2c_adapter->tx_tl = I2C_TX_TL_VALUE(0);
    i2c_adapter->intr_mask = I2C_INTR_MASK_RX_FULL | I2C_INTR_MASK_START_DET | I2C_INTR_MASK_STOP_DET | I2C_INTR_MASK_RD_REQ;

    plic_set_priority(IRQN_I2C0_INTERRUPT + i2c_num, 1);
    plic_irq_register(IRQN_I2C0_INTERRUPT + i2c_num, maix_i2c_slave_irq, maix_slave_context+i2c_num);
    plic_irq_enable(IRQN_I2C0_INTERRUPT + i2c_num);

    i2c_adapter->enable = I2C_ENABLE_ENABLE;
    is_master_mode[i2c_num] = false;
}

#define time_ms() (unsigned long)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000))

/**
 * 
 * @reutrn 0: success  
 *        <0: error
 */
int maix_i2c_send_data(i2c_device_number_t i2c_num, uint32_t slave_address, const uint8_t *send_buf, size_t send_buf_len, uint16_t timeout_ms)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    volatile i2c_t* i2c_adapter = i2c[i2c_num];
    size_t fifo_len, index;
    unsigned long time_start = time_ms();

    if(is_master_mode[i2c_num] && slaves_addr[i2c_num] != slave_address)
    {
        i2c_adapter->enable = 0;
        i2c_adapter->tar = I2C_TAR_ADDRESS(slave_address);
        slaves_addr[i2c_num] = (uint16_t)I2C_TAR_ADDRESS(slave_address);
        i2c_adapter->enable = 1;
    }
    while (send_buf_len)
    {
        if(time_ms() - time_start > timeout_ms)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -3;
        }
        fifo_len = 8 - i2c_adapter->txflr;
        fifo_len = send_buf_len < fifo_len ? send_buf_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            i2c_adapter->data_cmd = I2C_DATA_CMD_DATA(*send_buf++);
        if (i2c_adapter->tx_abrt_source != 0)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -1;
        }
        send_buf_len -= fifo_len;
    }
    while ( (i2c_adapter->status & I2C_STATUS_ACTIVITY) ||
            !(i2c_adapter->status & I2C_STATUS_TFE) )
    {
        if(time_ms() - time_start > timeout_ms)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -3;
        }
        if (i2c_adapter->tx_abrt_source != 0)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -2;
        }
    }

    return 0;
}


/**
 * 
 * @reutrn 0: success  
 *        <0: error
 */
int maix_i2c_recv_data(i2c_device_number_t i2c_num, uint32_t slave_address, const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                  size_t receive_buf_len, uint16_t timeout_ms)
{
    configASSERT(i2c_num < I2C_MAX_NUM);

    size_t fifo_len, index;
    // size_t buf_len; 
    size_t rx_len;
    rx_len = receive_buf_len;
    // buf_len = rx_len;
    volatile i2c_t* i2c_adapter = i2c[i2c_num];
    unsigned long time_start = time_ms();
    
    if(is_master_mode[i2c_num] && slaves_addr[i2c_num] != slave_address)
    {
        i2c_adapter->enable = 0;
        i2c_adapter->tar = I2C_TAR_ADDRESS(slave_address);
        slaves_addr[i2c_num] = (uint16_t)I2C_TAR_ADDRESS(slave_address);
        i2c_adapter->enable = 1;
    }

    while (send_buf_len)
    {
        if(time_ms() - time_start > timeout_ms)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -3;
        }
        fifo_len = 8 - i2c_adapter->txflr;
        fifo_len = send_buf_len < fifo_len ? send_buf_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            i2c_adapter->data_cmd = I2C_DATA_CMD_DATA(*send_buf++);
        if (i2c_adapter->tx_abrt_source != 0)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -1;
        }
        send_buf_len -= fifo_len;
    }

    while (receive_buf_len || rx_len)
    {
        if(time_ms() - time_start > timeout_ms)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            return -3;
        }
        fifo_len = i2c_adapter->rxflr;
        fifo_len = rx_len < fifo_len ? rx_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            *receive_buf++ = (uint8_t)i2c_adapter->data_cmd;
        rx_len -= fifo_len;
        fifo_len = 8 - i2c_adapter->txflr;
        fifo_len = receive_buf_len < fifo_len ? receive_buf_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            i2c_adapter->data_cmd = I2C_DATA_CMD_CMD;
        if (i2c_adapter->tx_abrt_source != 0)
        {
            i2c_adapter->clr_tx_abrt;
            i2c_adapter->enable = 0;
            i2c_adapter->enable = 1;
            usleep(10);
            // if(receive_buf_len == buf_len)
                return -2;
            // return buf_len - receive_buf_len;
        }
        receive_buf_len -= fifo_len;
    }
    return 0;
}

void maix_i2c_deinit(i2c_device_number_t i2c_num)
{
    configASSERT(i2c_num < I2C_MAX_NUM);

    volatile i2c_t *i2c_adapter = i2c[i2c_num];

    i2c_adapter->enable = 0;
    sysctl_clock_disable(SYSCTL_CLOCK_I2C0 + i2c_num);
    if( !is_master_mode[i2c_num] )
    {
        plic_irq_deregister(IRQN_I2C0_INTERRUPT + i2c_num);
        plic_irq_disable(IRQN_I2C0_INTERRUPT + i2c_num);
    }
}



static int maix_i2c_slave_irq(void *userdata)
{
    i2c_slave_context_t *context = (i2c_slave_context_t *)userdata;
    volatile i2c_t *i2c_adapter = i2c[context->i2c_num];
    uint32_t status = i2c_adapter->intr_stat;
    if (status & I2C_INTR_STAT_START_DET)
    {
        context->slave_handler->on_event(I2C_EV_START);
        readl(&i2c_adapter->clr_start_det);
    }
    if (status & I2C_INTR_STAT_RX_FULL)
    {
        context->slave_handler->on_receive(i2c_adapter->data_cmd);
    }
    if (status & I2C_INTR_STAT_RD_REQ)
    {
        i2c_adapter->data_cmd = context->slave_handler->on_transmit();
        readl(&i2c_adapter->clr_rd_req);
    }
    if (status & I2C_INTR_STAT_STOP_DET)
    {
        context->slave_handler->on_event(I2C_EV_STOP);
        readl(&i2c_adapter->clr_stop_det);
    }
    return 0;
}

