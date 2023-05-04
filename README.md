# spi-tools

## 1、介绍

spi-tools 包含了一些很方便对 spi 设备进行调试的小工具。

```
msh />spi init spi0-1 spi0 5
msh />spi config spi0-1 -s=5000000
msh />spi trans spi0-1 -v=0x5b -l=10
recv:10,[5B 5B 5B 5B 5B 5B 5B 5B 5B 5B]
```

SPI总线和设备 (可以 list device(list_device) 查看 spi 设备)：
```
    ---------------
    spi tools help:
    ---------------
    spi <command> device_name [options]
    spi config [options]
        -s --speed=[value] spi speed, default=1000000
        -m --mode=[0-3] spi mode, default=0
        -l --lsb=1:lsb, 0:msb, default=0
        -b --bits=[value] bits per word, default=8
    spi trans bus_name [options] [data0 data1...]
        -v --value=[value] master to slave default data, default value=0xFF
        -l --len=[value], default=1
```

## 2、联系方式

* 维护：Vandoul
* 联系：https://github.com/vandoul/rt-spi-tools/issues
