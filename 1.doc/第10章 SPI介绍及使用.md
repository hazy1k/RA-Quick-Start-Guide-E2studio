# 第十章 SPI介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **SPI（Serial Peripheral Interface）模块配置、主从模式、DMA高速传输、多设备片选管理、QSPI Flash接口、时序控制与实战应用**，实现高速、可靠的串行外设通信。

---

## 1. RA6E2 SPI 模块简介

RA6E2 内置多个 **SPI 通道（如 SPI0~SPI5）** 和 **QSPI（Quad SPI）** 模块，支持全双工高速通信，广泛用于连接：

- 外部 Flash（W25Q128、MX25L 等）
- LCD 屏幕（TFT、OLED）
- 高速 ADC/DAC
- 传感器（如 ADXL345、MAX31855）
- SD 卡（SPI 模式）

### 1.1 SPI 核心特性

| 特性                 | 说明                                      |
| ------------------ | --------------------------------------- |
| 工作模式               | 主模式（Master）、从模式（Slave）                  |
| 最高波特率              | ≤ PCLK/2，最高可达 **60 MHz**（系统时钟 120MHz）   |
| 数据宽度               | 8/16/32 位（可配置）                          |
| 时钟极性/相位（CPOL/CPHA） | 支持四种 SPI 模式（Mode 0~3）                   |
| FIFO 缓冲            | 发送/接收 FIFO（提高效率，减少中断）                   |
| DTC 支持             | ✅ 支持 DTC 自动收发，实现 DMA 级传输                |
| 多设备支持              | 支持软件片选（GPIO）或硬件片选（SSn）                  |
| QSPI 支持            | 可连接 Quad/Octal SPI Flash，实现 XIP（代码直接执行） |

📌 **RA6E2 SPI vs QSPI 区别**：

| 项目   | SPI（本章）        | QSPI（后续扩展）           |
| ---- | -------------- | -------------------- |
| 数据线  | MOSI, MISO（单线） | 支持 4/8 线（IO0~IO7）    |
| 速度   | ≤ 60 MHz       | ≤ 133 MHz（DDR 模式）    |
| 典型用途 | 外设控制（LCD、传感器）  | 外部 Flash 存储、XIP 运行代码 |

---

## 2. FSP 图形化配置 SPI（主模式）

### 2.1 添加 SPI 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Connectivity` → `SPI`
3. 添加 `g_spi_master`（如 `g_spi0`）

---

### 2.2 配置主模式参数（Properties）

| 参数                        | 推荐值/说明                                     |
| ------------------------- | ------------------------------------------ |
| **Channel**               | 0, 1, 2...（选择 SPI0）                        |
| **Mode**                  | `Master`                                   |
| **Bit Rate (Hz)**         | `1000000`（1MHz） 或更高（如 10MHz）               |
| **Data Size**             | `8-bit`, `16-bit`                          |
| **Clock Polarity (CPOL)** | `Low`（Mode 0/2） 或 `High`（Mode 1/3）         |
| **Clock Phase (CPHA)**    | `Leading`（Mode 0/1） 或 `Trailing`（Mode 2/3） |
| **SS Polarity**           | `Low`（片选低有效）                               |
| **DTC Support**           | ✅ Enable（用于大数据传输）                          |

📌 **SPI 四种模式说明**：

| Mode | CPOL | CPHA | 时钟空闲 | 数据采样时刻 |
| ---- | ---- | ---- | ---- | ------ |
| 0    | 0    | 0    | 低电平  | 第一个上升沿 |
| 1    | 0    | 1    | 低电平  | 第一个下降沿 |
| 2    | 1    | 0    | 高电平  | 第一个下降沿 |
| 3    | 1    | 1    | 高电平  | 第一个上升沿 |

> 📌 多数 Flash/LCD 使用 **Mode 0**。

---

### 2.3 配置引脚（Pins 标签页）

- `SPI0 MOSI` → 分配到物理引脚（如 P300）
- `SPI0 MISO` → 分配到物理引脚（如 P301）
- `SPI0 SCLK` → 分配到物理引脚（如 P302）
- `SPI0 SS` → 可分配硬件 SS（如 P303），或使用 **GPIO 软件控制**

📌 **推荐使用 GPIO 软件片选**，更灵活控制多设备。

---

### 2.4 配置 DTC（DMA 级传输）

1. 添加 `DTC` 模块
2. 在 SPI 配置中启用：
   - `Use DTC for Transmit`
   - `Use DTC for Receive`
3. FSP 自动生成 DTC 链接

> ✅ 启用 DTC 后，`R_SPI_Write` 等函数自动使用 DMA 传输！

---

## 3. SPI 相关 FSP 函数详解

> 📌 所有函数声明在 `r_spi_master.h` 中（主模式）。

---

### 3.1 R_SPI_Open

#### 3.1.1 函数原型及功能

初始化 SPI 模块。

```c
fsp_err_t R_SPI_Open(spi_ctrl_t * const p_ctrl, spi_cfg_t const * const p_cfg)
```

#### 3.1.2 参数

- `spi_ctrl_t * const p_ctrl`：控制句柄（如 `g_spi0_ctrl`）
- `spi_cfg_t const * const p_cfg`：配置结构体（如 `g_spi0_cfg`）

#### 3.1.3 返回值

- `FSP_SUCCESS`：成功
- `FSP_ERR_ASSERTION`
- `FSP_ERR_ALREADY_OPEN`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **典型用法**：

```c
fsp_err_t err = R_SPI_Open(&g_spi0_ctrl, &g_spi0_cfg);
if (FSP_SUCCESS != err) while(1);
```

---

### 3.2 R_SPI_Write

#### 3.2.1 函数原型及功能

发送数据（全双工，发送同时接收）。

```c
fsp_err_t R_SPI_Write(spi_ctrl_t * const p_ctrl, uint8_t const * const p_src, uint32_t const bytes, spi_bit_width_t const bit_width)
```

#### 3.2.2 参数

- `p_src`：发送缓冲区
- `bytes`：字节数
- `bit_width`：`SPI_BIT_WIDTH_8_BITS` 或 `16` 等

📌 **注意**：SPI 是全双工，发送时也会收到数据，需通过 `R_SPI_Read` 获取（或启用 DTC 自动搬运）。

---

### 3.3 R_SPI_Read

#### 3.3.1 函数原型及功能

接收数据（发送 dummy 数据以产生时钟）。

```c
fsp_err_t R_SPI_Read(spi_ctrl_t * const p_ctrl, uint8_t * const p_dest, uint32_t const bytes, spi_bit_width_t const bit_width)
```

📌 **典型用法**：读取传感器或 Flash 回应。

---

### 3.4 R_SPI_WriteRead

#### 3.4.1 函数原型及功能

同步发送和接收（全双工）。

```c
fsp_err_t R_SPI_WriteRead(spi_ctrl_t * const p_ctrl, uint8_t const * const p_tx_data, uint8_t * const p_rx_data, uint32_t const length, spi_bit_width_t const bit_width)
```

📌 **最适合用于 Flash 读写、LCD 命令传输**。

---

### 3.5 R_SPI_Close

#### 3.5.1 函数原型及功能

关闭 SPI 模块。

```c
fsp_err_t R_SPI_Close(spi_ctrl_t * const p_ctrl)
```

---

### 3.6 R_SPI_Abort

#### 3.6.1 函数原型及功能

中止当前传输。

```c
fsp_err_t R_SPI_Abort(spi_ctrl_t * const p_ctrl)
```

---

## 4. SPI 中断回调函数

```c
void spi_callback(spi_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case SPI_EVENT_TRANSFER_COMPLETE:
            // 传输完成
            g_spi_done = true;
            break;

        case SPI_EVENT_TRANSFER_ABORTED:
            // 传输中止
            break;

        case SPI_EVENT_ERR_MODE_FAULT:
            // 模式错误（如从机模式冲突）
            break;

        case SPI_EVENT_ERR_READ_OVERFLOW:
            // 接收缓冲区溢出
            break;

        default:
            break;
    }
}
```

---

## 5. 实战1：SPI 控制 W25Q128 Flash 读写

### 5.1 W25Q128 常用命令

| 命令     | 说明                  |
| ------ | ------------------- |
| `0x06` | Write Enable        |
| `0x20` | Sector Erase (4KB)  |
| `0x02` | Page Program (256B) |
| `0x03` | Read Data           |
| `0x9F` | Read JEDEC ID       |

### 5.2 代码实现（软件片选）

```c
#include "hal_data.h"

#define FLASH_CS_PORT   BSP_IO_PORT_03_PIN_04  // P304
#define CMD_READ_ID     0x9F
#define CMD_READ_DATA   0x03

void flash_cs_low(void)  { R_IOPORT_PinWrite(&g_ioport_ctrl, FLASH_CS_PORT, BSP_IO_LEVEL_LOW); }
void flash_cs_high(void) { R_IOPORT_PinWrite(&g_ioport_ctrl, FLASH_CS_PORT, BSP_IO_LEVEL_HIGH); }

uint32_t flash_read_jedec_id(void)
{
    uint8_t cmd = CMD_READ_ID;
    uint8_t rx_data[3];

    flash_cs_low();
    R_SPI_WriteRead(&g_spi0_ctrl, &cmd, NULL, 1, SPI_BIT_WIDTH_8_BITS);        // 发送命令
    R_SPI_Read(&g_spi0_ctrl, rx_data, 3, SPI_BIT_WIDTH_8_BITS);                // 读取3字节
    flash_cs_high();

    return (rx_data[0] << 16) | (rx_data[1] << 8) | rx_data[2];
}

void hal_entry(void)
{
    R_SPI_Open(&g_spi0_ctrl, &g_spi0_cfg);

    uint32_t id = flash_read_jedec_id();
    char msg[64];
    snprintf(msg, sizeof(msg), "Flash JEDEC ID: 0x%06lX\r\n", id);
    R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*)msg, strlen(msg));

    while(1)
    {
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

---

## 6. 实战2：SPI 控制 ST7789 LCD 显示

### 6.1 接线说明

| LCD 引脚 | RA6E2 引脚    | 功能       |
| ------ | ----------- | -------- |
| SCK    | P302        | SPI SCLK |
| MOSI   | P300        | SPI MOSI |
| CS     | P303 (GPIO) | 片选       |
| DC     | P304 (GPIO) | 数据/命令    |
| RST    | P305 (GPIO) | 复位       |
| BLK    | P405 (PWM)  | 背光       |

### 6.2 发送命令/数据

```c
void lcd_write_cmd(uint8_t cmd)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_DC_PIN, BSP_IO_LEVEL_LOW);   // 命令模式
    flash_cs_low();
    R_SPI_Write(&g_spi0_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);
    flash_cs_high();
}

void lcd_write_data(uint8_t *data, uint32_t len)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, LCD_DC_PIN, BSP_IO_LEVEL_HIGH);  // 数据模式
    flash_cs_low();
    R_SPI_Write(&g_spi0_ctrl, data, len, SPI_BIT_WIDTH_8_BITS);
    flash_cs_high();
}
```

> ✅ 后续可结合 DMA 实现快速刷屏。

---

## 7. 多设备 SPI 管理策略

### 7.1 软件片选（推荐）

- 所有设备共用 MOSI/MISO/SCLK
- 每个设备使用独立 GPIO 作为 CS
- 通信前拉低对应 CS，结束后拉高

```c

```

---

### 7.2 硬件片选（多 SSn）

- 使用 SPI 模块自带的多个 SS 引脚（如 SS0, SS1）
- 配置 `p_cfg->ss_polarity` 和 `ss_io` 数组

> ⚠️ 使用复杂，灵活性低，不推荐。

---

## 8. SPI 函数速查表

| 函数名               | 用途     | 关键参数                  | 返回值/错误码        | 注意事项            |
| ----------------- | ------ | --------------------- | -------------- | --------------- |
| `R_SPI_Open`      | 初始化SPI | `p_ctrl`, `p_cfg`     | `ALREADY_OPEN` | 必须最先调用          |
| `R_SPI_Write`     | 发送数据   | `p_src`, `bytes`      | `ERR_INVALID`  | 全双工，接收数据需单独读取   |
| `R_SPI_Read`      | 接收数据   | `p_dest`, `bytes`     | `ERR_INVALID`  | 发送 dummy 数据产生时钟 |
| `R_SPI_WriteRead` | 同步收发   | `p_tx`, `p_rx`, `len` | `ERR_INVALID`  | 最常用             |
| `R_SPI_Abort`     | 中止传输   | -                     | -              | 紧急停止            |
| `R_SPI_Close`     | 关闭SPI  | -                     | -              | 释放资源            |
| `spi_callback`    | 事件回调   | `p_args->event`       | 无返回值           | 处理完成/错误事件       |

---

## 9. 开发建议

✅ **推荐做法**：

1. **始终使用软件片选（GPIO）**，便于多设备管理。
2. **高速传输启用 DTC**，降低 CPU 负担。
3. **Flash/LCD 使用 Mode 0 (CPOL=0, CPHA=0)**。
4. **通信前后使用 CS 控制设备选择**。
5. **长距离布线时降低波特率**（< 10MHz）。

⛔ **避免做法**：

1. **忽略片选时序**（CS 必须在传输前后正确拉高/低）。
2. **在中断中执行 SPI 通信**（除非你完全控制）。
3. **使用过高的波特率导致信号失真**。

---


