#include <rtthread.h>
#include <drivers/spi.h>

#define ARG_CMD_POS             1
#define ARG_DEV_NAME_POS        2
#define ARG_BUS_NAME_POS        3
#define ARG_CS_PIN_POS          4
#define ARG_CONFIG_PARA_POS     3
#define ARG_TRANS_PARA_POS      3

static inline int str2hex_d(char c)
{
    if(c < '0' || c > '9')
    {
        return -1;
    }
    return (c - '0');
}
static inline int str2hex_x(char c)
{
    if(c < 'a' || c > 'f')
    {
        return str2hex_d(c);
    }
    return (c - 'W'); //'W' = 'a' - 10
}
static rt_uint32_t str2hex(const char *str)
{
    volatile rt_uint32_t value = 0;
    volatile rt_uint8_t scale = 0;
    volatile int i;
    int (*volatile func)(char) = RT_NULL;
    if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        i = 2;
        func = str2hex_x;
        scale = 16;
    }
    else
    {
        i = 0;
        func = str2hex_d;
        scale = 10;
    }
    for(;str[i];i++)
    {
        int v = func(str[i]|0x20);
        if(v == -1)
        {
            break;
        }
        value *= scale;
        value += v;
    }
    return value;
}
static void spi_help(void)
{
    rt_kprintf("---------------\n");
    rt_kprintf("spi tools help:\n");
    rt_kprintf("---------------\n");

    rt_kprintf("spi <command> device_name [options]\n");
    rt_kprintf("spi config device_name [options]\n");
    rt_kprintf("\t-s=[speed] spi speed, default=1000000\n");
    rt_kprintf("\t-m=[0-3] spi mode, default=0\n");
    rt_kprintf("\t-l=1:lsb, 0:msb, default=0\n");
    rt_kprintf("\t-b=[value] bits per word, default=8\n");
    rt_kprintf("spi trans device_name [options] [data0 data1...]\n");
    rt_kprintf("\t-v=[value], default=0xFF\n");
    rt_kprintf("\t-l=[value], default=1, max=512\n");
    rt_kprintf("spi init device_name bus_name cs_pin\n");
    rt_kprintf("spi deinit device_name\n\n");
}
static struct rt_spi_device *spi_device = RT_NULL;
static rt_uint8_t trans_buf[512];
static void spi(int argc,char *argv[])
{
    if(argc > 3)
    {
        const char *cmd_str = argv[ARG_CMD_POS];
        const char *dev_name = argv[ARG_DEV_NAME_POS];

        if(!strcmp(cmd_str, "config"))
        {
            struct rt_spi_device *dev = (struct rt_spi_device *)rt_device_find(dev_name);
            if(dev == RT_NULL)
            {
                rt_kprintf("[spi] cant't find device:%s\n", dev_name);
                return ;
            }
            struct rt_spi_configuration cfg = {
                .mode       = RT_SPI_MODE_0|RT_SPI_MASTER|RT_SPI_MSB,
                .data_width = 8,
                .max_hz     = 1000000,
            };
            
            for(int i=ARG_CONFIG_PARA_POS; i<argc; i++)
            {
                const char *para = argv[i];
                if(para[0] == '-' && para[2] == '=')
                {
                    switch(para[1])
                    {
                        case 's':
                            cfg.max_hz = str2hex(&para[3]);
                            break;
                        case 'm':
                            if(para[3] == '1')
                            {
                                cfg.mode |= RT_SPI_MODE_1;
                            }
                            else if(para[3] == '2')
                            {
                                cfg.mode |= RT_SPI_MODE_2;
                            }
                            else if(para[3] == '3')
                            {
                                cfg.mode |= RT_SPI_MODE_3;
                            }
                            break;
                        case 'l':
                            if(para[3] == '1')
                            {
                                cfg.mode &= ~RT_SPI_LSB;
                            }
                            break;
                        case 'b':
                            cfg.data_width = str2hex(&para[3]);
                            break;
                    }
                }
            }
            
            if(RT_EOK != rt_spi_configure(dev, &cfg))
            {
                rt_kputs("[spi] config failed\n");
                return ;
            }
        }

        else if(!strcmp(cmd_str, "trans"))
        {
            int trans_len = 1;
            rt_uint8_t value = 0xFF;
            struct rt_spi_device *dev = (struct rt_spi_device *)rt_device_find(dev_name);
            if(dev == RT_NULL)
            {
                rt_kprintf("[spi] cant't find device:%s\n", dev_name);
                return ;
            }
            
            for(int i=ARG_TRANS_PARA_POS; i<argc; i++)
            {
                const char *para = argv[i];
                if(para[0] == '-' && para[2] == '=')
                {
                    switch(para[1])
                    {
                        case 'v':
                            value = str2hex(&para[3]);
                            break;
                        case 'l':
                            trans_len = str2hex(&para[3]);
                            break;
                    }
                }
            }
            
            for(int i=0; i<trans_len; i++)
            {
                trans_buf[i] = value;
            }
            
            rt_ssize_t ret = rt_spi_transfer(dev, trans_buf, trans_buf, trans_len);
            if(ret < 0)
            {
                rt_kputs("[spi] trans failed\n");
                return ;
            }
            rt_kprintf("recv:%d,[", ret);
            for(int i=0; i<trans_len; i++)
            {
                if(i)
                    rt_kprintf(" %02X", trans_buf[i]);
                else
                    rt_kprintf("%02X", trans_buf[i]);
            }
            rt_kputs("]\n");
        }

        else if(!strcmp(cmd_str, "init"))
        {
            const char *bus_name = argv[ARG_BUS_NAME_POS];
            int cs_pin = atoi(argv[ARG_CS_PIN_POS]);
            if(spi_device == RT_NULL)
            {
                spi_device = rt_malloc(sizeof(struct rt_spi_device));
                if(spi_device == RT_NULL)
                {
                    rt_kputs("[spi] create failed\n");
                    return ;
                }
            }
            if(RT_EOK != rt_spi_bus_attach_device_cspin(spi_device, dev_name, bus_name, cs_pin, RT_NULL))
            {
                rt_kputs("[spi] attach failed!\n");
                return ;
            }
        }
    }
    else
    {
        const char *cmd_str = argv[ARG_CMD_POS];
        const char *dev_name = argv[ARG_DEV_NAME_POS];
        if(!strcmp(cmd_str, "deinit"))
        {
            if(argc == 3)
            {
                rt_device_t dev = rt_device_find(dev_name);
                if(dev != RT_NULL)
                {
                    if(RT_EOK != rt_device_unregister(dev))
                    {
                        rt_kputs("[spi] unregister failed\n");
                    }
                    return ;
                }
                rt_kprintf("[spi] cant't find device:%s\n", dev_name);
                return ;
            }
        }
        spi_help();
    }
}
MSH_CMD_EXPORT(spi, spi tools);