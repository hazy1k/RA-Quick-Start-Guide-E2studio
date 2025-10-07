# 第十一章 QSPI外部Flash与XIP介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **QSPI（Quad Serial Peripheral Interface）模块配置、外部 Flash 接口、XIP（eXecute In Place）模式、高速读写、DMA/DTC 传输、Flash 分区管理**，实现 **大容量存储、快速启动、代码直接执行** 的高性能嵌入式系统。

---

## 1. RA6E2 QSPI 模块简介

RA6E2 集成 **QSPI 控制器**，支持连接 **Quad/Octal SPI Flash**，具备以下特性：

### 1.1 核心特性

| 特性      | 说明                                                         |
| ------- | ---------------------------------------------------------- |
| 数据线     | 支持 **1/2/4/8 线模式（Single/Quad/Octal）**                      |
| 最高频率    | 支持 **133 MHz**（DDR 模式下等效 266 Mbps）                         |
| 支持协议    | JEDEC **JESD216B** 标准，兼容 Winbond、Micron、Macronix 等主流 Flash |
| XIP 模式  | ✅ **代码直接在 Flash 中执行**，无需复制到 RAM                            |
| 高速读写    | 支持 **DTR（Double Transfer Rate）** 和 **Continuous Read** 模式  |
| DTC 支持  | ✅ 支持 DTC 自动读写，实现 DMA 级数据搬运                                 |
| 内存映射    | QSPI Flash 可映射到 CPU 地址空间（如 `0x60000000`）                   |
| Boot 功能 | ✅ 可从 QSPI Flash 启动系统（External Boot Mode）                   |

📌 **典型应用场景**：

- 存储大容量程序代码（Bootloader、App2、OTA）
- 存储图片、音频、字体等资源
- 实现 XIP 运行代码，节省 RAM
- 快速启动系统（Cold Start < 100ms）
- 数据日志存储（配合文件系统）

---

### 1.2 QSPI vs SPI 对比

| 项目       | SPI（上一章）          | QSPI（本章）                       |
| -------- | ----------------- | ------------------------------ |
| 数据线      | MOSI/MISO（1线）     | IO0~IO7（最多8线）                  |
| 速度       | ≤ 60 Mbps         | ≤ 266 Mbps（DDR 133MHz）         |
| 地址空间     | 无内存映射             | ✅ 映射到 `0x60000000`             |
| XIP      | ❌                 | ✅ 可直接执行代码                      |
| 典型 Flash | W25Q128JV（SPI 模式） | W25Q128JV, IS25WP064C（QPI/OPI） |
| 接线复杂度    | 4~5 线             | 6~~8 线（含 CLK, CS, IO0~~3）      |

> ✅ **QSPI 是 SPI 的“超集”**，专为高性能存储设计。

---

## 2. FSP 图形化配置 QSPI

### 2.1 添加 QSPI 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Memory` → `QSPI`
3. 添加 `g_qspi` 实例

---

### 2.2 配置基本参数（Properties）

| 参数                     | 推荐值/说明                         |
| ---------------------- | ------------------------------ |
| **Clock Source**       | QSPCLK（建议 ≥ 133MHz，通过 PLL 生成）  |
| **Bit Rate (Hz)**      | `133000000`（133MHz）            |
| **Data Lines**         | `4 (Quad)` 或 `8 (Octal)`       |
| **Clock Polarity**     | `Low`（多数 Flash 使用 Mode 0）      |
| **Transfer Mode**      | `Continuous Read`（开启连续读取，提高性能） |
| **Memory Mapped Mode** | ✅ Enable（启用内存映射，实现 XIP）        |
| **DTC Support**        | ✅ Enable（用于大数据读写）              |

---

### 2.3 配置 Flash 参数（Flash Options）

FSP 支持自动识别 Flash（通过 SFDP），也可手动设置：

| 参数                  | 示例值（W25Q128JV）                   |
| ------------------- | -------------------------------- |
| **Flash Size**      | `16777216` bytes（16MB / 128Mbit） |
| **Page Size**       | `256`                            |
| **Sector Size**     | `4096`（4KB）                      |
| **Block Size**      | `65536`（64KB）                    |
| **Manufacturer ID** | `0xEF`（Winbond）                  |
| **Memory Type ID**  | `0x40`                           |
| **Capacity ID**     | `0x18`                           |

> ✅ 勾选 `Use SFDP` 可自动读取 Flash 参数。

---

### 2.4 配置引脚（Pins 标签页）

| QSPI 信号  | 推荐引脚（RA6E2） | 功能说明       |
| -------- | ----------- | ---------- |
| QSPCLK   | P400        | 时钟         |
| QSPCS    | P401        | 片选（低有效）    |
| QSPIO0   | P402        | 数据0        |
| QSPIO1   | P403        | 数据1        |
| QSPIO2   | P404        | 数据2        |
| QSPIO3   | P405        | 数据3        |
| QSPIO4~7 | P406~P409   | Octal 模式使用 |

📌 **注意**：

- 所有 QSPI 引脚必须配置为 **Peripheral Function** → `QSPI_xxx`
- Output Type: `Push-pull`
- Pull-up: 不启用（高速信号避免干扰）

---

## 3. QSPI 相关 FSP 函数详解

> 📌 所有函数声明在 `r_qspi.h` 中。

---

### 3.1 R_QSPI_Open

#### 3.1.1 函数原型及功能

初始化 QSPI 模块。

```c
fsp_err_t R_QSPI_Open(qspi_ctrl_t * const p_ctrl, qspi_cfg_t const * const p_cfg)
```

#### 3.1.2 参数

- `qspi_ctrl_t * const p_ctrl`：控制句柄（如 `g_qspi_ctrl`）
- `qspi_cfg_t const * const p_cfg`：配置结构体（如 `g_qspi_cfg`）

#### 3.1.3 返回值

- `FSP_SUCCESS`：成功
- `FSP_ERR_ASSERTION`
- `FSP_ERR_ALREADY_OPEN`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **典型用法**：

```c
fsp_err_t err = R_QSPI_Open(&g_qspi_ctrl, &g_qspi_cfg);
if (FSP_SUCCESS != err) while(1);
```

---

### 3.2 R_QSPI_Command

#### 3.2.1 函数原型及功能

发送 Flash 控制命令（如写使能、读ID）。

```c
fsp_err_t R_QSPI_Command(qspi_ctrl_t * const p_ctrl, uint8_t const command, uint8_t const * const p_address, uint32_t const address_size, uint8_t const * const p_tx_data, uint32_t const tx_data_size, uint8_t * const p_rx_data, uint32_t const rx_data_size)
```

#### 3.2.2 参数

- `command`：Flash 命令（如 `0x9F` 读 JEDEC ID）
- `p_address`：地址指针（可为 NULL）
- `address_size`：地址字节数（0, 3, 4）
- `p_tx_data`：发送数据
- `tx_data_size`：发送长度
- `p_rx_data`：接收缓冲区
- `rx_data_size`：期望接收长度

📌 **用法示例（读 JEDEC ID）**：

```c
uint8_t rx_id[3];
R_QSPI_Command(&g_qspi_ctrl, 0x9F, NULL, 0, NULL, 0, rx_id, 3);
```

---

### 3.3 R_QSPI_Read

#### 3.3.1 函数原型及功能

从 Flash 读取数据（推荐用于非 XIP 访问）。

```c
fsp_err_t R_QSPI_Read(qspi_ctrl_t * const p_ctrl, uint8_t * const p_dest, uint32_t const bytes, uint32_t const address)
```

📌 **注意**：XIP 模式下可直接用指针访问，无需调用此函数。

---

### 3.4 R_QSPI_Write

#### 3.4.1 函数原型及功能

写入数据到 Flash（页写入）。

```c
fsp_err_t R_QSPI_Write(qspi_ctrl_t * const p_ctrl, uint8_t const * const p_src, uint32_t const bytes, uint32_t const address)
```

📌 **注意**：

- 写入前必须 **擦除**（Sector/Block Erase）
- 每页最多写入 **256 字节**

---

### 3.5 R_QSPI_Erase

#### 3.5.1 函数原型及功能

擦除 Flash 扇区或块。

```c
fsp_err_t R_QSPI_Erase(qspi_ctrl_t * const p_ctrl, uint32_t const address, qspi_erase_t const erase_type)
```

#### 3.5.2 参数

- `address`：要擦除的地址（必须对齐到扇区/块）
- `erase_type`：
  - `QSPI_ERASE_SECTOR_4KB`
  - `QSPI_ERASE_BLOCK_64KB`

---

### 3.6 R_QSPI_StatusGet

#### 3.6.1 函数原型及功能

读取 Flash 状态寄存器（判断忙状态）。

```c
fsp_err_t R_QSPI_StatusGet(qspi_ctrl_t * const p_ctrl, uint8_t * const p_status)
```

📌 **写/擦除操作后必须轮询状态**：

```c
uint8_t status;
do {
    R_QSPI_StatusGet(&g_qspi_ctrl, &status);
} while (status & 0x01); // WIP bit
```

---

### 3.7 R_QSPI_Close

#### 3.7.1 函数原型及功能

关闭 QSPI 模块。

```c
fsp_err_t R_QSPI_Close(qspi_ctrl_t * const p_ctrl)
```

---

## 4. QSPI 内存映射与 XIP

### 4.1 地址映射

QSPI Flash 默认映射到 CPU 地址空间：

```c
0x60000000 ~ 0x60FFFFFF  ←→  QSPI Flash 物理地址 0x000000 ~ 0xFFFFFF
```

### 4.2 XIP：直接执行代码

```c
// 定义一个函数存放到 Flash 中（非 RAM）
__attribute__((section(".qspi_section"))) void flash_function(void)
{
    // 这个函数直接在 QSPI Flash 中执行
    R_IOPORT_PinToggle(&g_ioport_ctrl, LED_RED_PIN);
}

// 调用它，就像普通函数
flash_function(); // ✅ 直接从 Flash 执行
```

### 4.3 链接脚本配置（在 FSP 中自动生成）

FSP 会在 `src\pin\ra6e2\linker\gcc\` 下生成带 QSPI 的链接脚本：

- `RA6E2_QSPI_144.icf`（IAR）
- `RA6E2_QSPI_144.ld`（GCC）

确保：

- `.text` 或自定义 section 放入 QSPI 区域
- 堆栈仍在 RAM 中

---

## 5. 实战1：读写 QSPI Flash 存储数据

```c
#include "hal_data.h"

#define FLASH_ADDR_DATA     0x100000    // 1MB 处
#define DATA_SIZE           1024

static uint8_t g_write_data[DATA_SIZE];
static uint8_t g_read_data[DATA_SIZE];

void flash_program_test(void)
{
    // 1. 擦除扇区
    R_QSPI_Erase(&g_qspi_ctrl, FLASH_ADDR_DATA, QSPI_ERASE_SECTOR_4KB);

    // 2. 等待擦除完成
    uint8_t status;
    do {
        R_QSPI_StatusGet(&g_qspi_ctrl, &status);
    } while (status & 0x01);

    // 3. 写入数据
    for (int i = 0; i < DATA_SIZE; i++) {
        g_write_data[i] = i % 256;
    }
    R_QSPI_Write(&g_qspi_ctrl, g_write_data, DATA_SIZE, FLASH_ADDR_DATA);

    // 4. 读取验证
    R_QSPI_Read(&g_qspi_ctrl, g_read_data, DATA_SIZE, FLASH_ADDR_DATA);

    // 5. 比较
    bool match = (memcmp(g_write_data, g_read_data, DATA_SIZE) == 0);
    R_IOPORT_PinWrite(&g_ioport_ctrl, LED_RED_PIN, match ? BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
}
```

---

## 6. 实战2：从 QSPI Flash 启动（External Boot）

### 6.1 硬件设置

- 设置 RA6E2 的 `MODE[1:0]` 引脚为 `10`（External Boot）
- 确保 QSPI Flash 中已烧录有效程序

### 6.2 烧录流程

1. 使用 e2 studio 调试器将程序烧录到 QSPI Flash
2. 或使用 Flash Programmer 工具
3. 复位 MCU，从 Flash 自动启动

📌 **启动速度**：Cold Start < 100ms，远快于 SD 卡启动！

---

## 7. QSPI 函数速查表

| 函数名                | 用途         | 关键参数                      | 返回值/错误码         | 注意事项     |
| ------------------ | ---------- | ------------------------- | --------------- | -------- |
| `R_QSPI_Open`      | 初始化QSPI    | `p_ctrl`, `p_cfg`         | `ALREADY_OPEN`  | 必须最先调用   |
| `R_QSPI_Command`   | 发送Flash命令  | `command`, `*p_rx_data`   | `INVALID_ARG`   | 读ID、状态等  |
| `R_QSPI_Read`      | 读取Flash数据  | `p_dest`, `bytes`, `addr` | `BUSY`          | 非XIP访问   |
| `R_QSPI_Write`     | 写入Flash（页） | `p_src`, `bytes`, `addr`  | `BUSY`, `ALIGN` | 需先擦除     |
| `R_QSPI_Erase`     | 擦除扇区/块     | `address`, `erase_type`   | `ALIGN`         | 地址必须对齐   |
| `R_QSPI_StatusGet` | 读状态寄存器     | `*p_status`               | -               | 判断 WIP 位 |
| `R_QSPI_Close`     | 关闭QSPI     | -                         | -               | 释放资源     |

---

## 8. 开发建议

✅ **推荐做法**：

1. **启用 Continuous Read + DDR 模式**，最大化读取速度。
2. **代码或资源存入 QSPI Flash，使用 XIP 执行**，节省 RAM。
3. **写/擦除操作前后轮询状态寄存器**。
4. **使用 DTC 加速大块数据读写**。
5. **分区管理 Flash**（如 0~~1MB Boot, 1~~2MB App, 2~16MB Assets）。

⛔ **避免做法**：

1. **频繁写入 Flash**（寿命有限，典型 10万次）
2. **写入未擦除的区域**（导致数据错误）
3. **忽略地址对齐要求**
4. **在中断中执行擦除/写入**（耗时操作，建议后台任务）

---


