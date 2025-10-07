# 第七章 ADC介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **ADC（Analog-to-Digital Converter）模块架构、单通道/多通道采样、软件/硬件触发、DTC自动传输、温度传感器集成、精度优化与校准**，实现高精度、高效率的模拟信号采集系统。

---

## 1. RA6E2 ADC 模块简介

RA6E2 集成 **12 位高精度 ADC**，支持单端输入，具备以下特性：

### 1.1 核心特性

| 特性           | 说明                                               |
| ------------ | ------------------------------------------------ |
| 分辨率          | 12 位（0 ~ 4095）                                   |
| 转换速度         | 最快 1.2 μs 转换时间（@ PCLK 120MHz）                    |
| 输入电压范围       | 0 ~ VREFH（通常 3.3V），VREFL 接地                      |
| 采样通道数        | 最多 21 通道（含外部引脚 + 内部信号）                           |
| 支持模式         | 单次转换、连续扫描、间隔扫描                                   |
| 触发方式         | 软件触发、GPT/PWM 定时触发、外部中断触发                         |
| 数据传输         | 支持 DTC（Data Transfer Controller）自动搬运结果，无需 CPU 干预 |
| 内部信号采集       | ✅ 温度传感器、内部 VREF、电源电压监控                           |
| 双 ADC 模式（可选） | 提高吞吐量（部分型号支持）                                    |

📌 **典型应用场景**：

- 传感器采集（温度、湿度、光照、压力）
- 电池电量检测
- 电机电流采样
- 音频信号采集（低速）
- 电源监控与保护

---

### 1.2 ADC 通道类型

| 类型   | 说明                 | 示例通道              |
| ---- | ------------------ | ----------------- |
| 外部引脚 | 连接外部模拟信号           | AD000, AD001, ... |
| 内部信号 | MCU 内部集成信号，无需引脚    | 温度传感器、VREF        |
| 差分输入 | 支持部分通道差分采集（提高抗噪能力） | AD000/AD001 差分对   |

> ✅ 本章重点讲解 **外部单端输入** 与 **温度传感器采集**。

---

## 2. FSP 图形化配置 ADC

### 2.1 添加 ADC 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Analog` → `ADC`
3. 添加 `g_adc0`（对应 ADC0）

---

### 2.2 配置基本参数（Properties）

| 参数                     | 推荐值/说明                                  |
| ---------------------- | --------------------------------------- |
| **Unit**               | 0 或 1（选择 ADC0）                          |
| **Resolution**         | `12-bit`                                |
| **Data Alignment**     | `Right`（右对齐，低位补0） 或 `Left`（左对齐）         |
| **Trigger Mode**       | `Software`（软件触发） 或 `Hardware`（如 GPT 触发） |
| **Sample State Count** | 采样时间周期数（建议 4~16，提高精度）                   |
| **Reference Voltage**  | `VREFH/VREFL`（外部基准） 或 `Internal`（内部基准）  |
| **Addition/Average**   | ✅ Enable 可对多次采样求和或平均（用于降噪）              |

---

### 2.3 配置通道（Channels）

点击 `Channels` 标签页，添加要采集的通道：

#### 2.3.1 外部通道示例（如 AD002 接电位器）

- `Channel`: `ADC_CHANNEL_2`
- `Channel Type`: `Single Ended`
- `Addition/Average`: `No Addition`
- `Resolution`: `12-bit`（可单独设置）

#### 2.3.2 内部通道示例（温度传感器）

- `Channel`: `ADC_CHANNEL_TEMP`
- `Channel Type`: `Internal`
- `Addition/Average`: `4-sample addition`（提高稳定性）

---

### 2.4 配置 DTC 数据传输（推荐）

1. 添加 `DTC` 模块（Driver → Transfer → DTC）
2. 在 ADC 配置中：
   - `Transfer Info` → `Enable`
   - `Transfer Destination`: 选择目标缓冲区（如 `g_adc_buffer`）
   - `Number of Transfers`: 2（若采集两个通道）
   - `Mode`: `Normal` 或 `Repeat`

> ✅ DTC 自动将 ADC 结果搬运到内存，**极大降低 CPU 负担**！

---

### 2.5 配置引脚（Pins 标签页）

- 找到 `AD002` → 设置为 `Analog Input`
- 确保未启用上下拉或输出功能

---

## 3. ADC 相关 FSP 函数详解

> 📌 所有函数声明在 `r_adc.h` 中。

---

### 3.1 R_ADC_Open

#### 3.1.1 函数原型及功能

初始化 ADC 模块。

```c
fsp_err_t R_ADC_Open(adc_ctrl_t * const p_ctrl, adc_cfg_t const * const p_cfg)
```

#### 3.1.2 参数

- `adc_ctrl_t * const p_ctrl`：控制句柄（如 `g_adc0_ctrl`）
- `adc_cfg_t const * const p_cfg`：配置结构体（如 `g_adc0_cfg`）

#### 3.1.3 返回值

- `FSP_SUCCESS`：成功
- `FSP_ERR_ASSERTION`
- `FSP_ERR_ALREADY_OPEN`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **典型用法**：

```c
fsp_err_t err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);

if (FSP_SUCCESS != err) while(1);
```

---

### 3.2 R_ADC_ScanStart

#### 3.2.1 函数原型及功能

启动一次 ADC 转换（软件触发模式）。

```c
fsp_err_t R_ADC_ScanStart(adc_ctrl_t * const p_ctrl)
```

📌 **注意**：仅在 `Trigger Mode = Software` 时有效。

---

### 3.3 R_ADC_Read

#### 3.3.1 函数原型及功能

读取指定通道的最近一次转换结果。

```c
fsp_err_t R_ADC_Read(adc_ctrl_t * const p_ctrl, adc_channel_t const channel_id, uint16_t * const p_data)
```

#### 3.3.2 参数

- `channel_id`：通道编号（如 `ADC_CHANNEL_2`）
- `p_data`：用于接收结果的变量指针

📌 **用法示例**：

```c
uint16_t adc_value;

R_ADC_Read(&g_adc0_ctrl, ADC_CHANNEL_2, &adc_value);
```

---

### 3.4 R_ADC_ReadAll

#### 3.4.1 函数原型及功能

读取所有已配置通道的转换结果（适合 DTC 模式）。

```c
fsp_err_t R_ADC_ReadAll(adc_ctrl_t * const p_ctrl, adc_data_t * const p_data)
```

#### 3.4.2 参数

- `p_data`：指向结构体数组，每个元素对应一个通道的结果

📌 **DTC 模式下推荐使用 `R_ADC_ReadAll`**。

---

### 3.5 R_ADC_ScanStop

#### 3.5.1 函数原型及功能

停止 ADC 转换（连续模式下使用）。

```c
fsp_err_t R_ADC_ScanStop(adc_ctrl_t * const p_ctrl)
```

---

### 3.6 R_ADC_InfoGet

#### 3.6.1 函数原型及功能

获取 ADC 当前状态、时钟、采样时间等信息。

```c
fsp_err_t R_ADC_InfoGet(adc_ctrl_t * const p_ctrl, adc_info_t * const p_info)
```

---

### 3.7 R_ADC_StatusGet

#### 3.7.1 函数原型及功能

查询 ADC 是否正在转换。

```c
fsp_err_t R_ADC_StatusGet(adc_ctrl_t * const p_ctrl, adc_status_t * const p_status)
```

📌 **用法示例**：

```c
adc_status_t status;
R_ADC_StatusGet(&g_adc0_ctrl, &status);
if (!status.state) {
    // ADC 空闲，可启动下一次转换
}
```

---

### 3.8 R_ADC_Close

#### 3.8.1 函数原型及功能

关闭 ADC 模块，关闭电源以省电。

```c
fsp_err_t R_ADC_Close(adc_ctrl_t * const p_ctrl)
```

---

## 4. ADC 中断回调函数

FSP 自动生成回调框架：

```c
void adc_callback(adc_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case ADC_EVENT_SCAN_COMPLETE:
            // 扫描完成，DTC 已搬运数据
            // 可在此处理数据或启动下一次采集
            g_adc_complete = true;
            break;

        case ADC_EVENT_SCAN_STOP:
            // 扫描被停止
            break;

        case ADC_EVENT_ERROR:
            // 采样错误
            break;

        default:
            break;
    }
}
```

📌 **DTC 模式下，`SCAN_COMPLETE` 是数据就绪的标志！**

---

## 5. 实战1：软件触发单通道采样（电位器）

```c
#include "hal_data.h"

#define POT_CHANNEL ADC_CHANNEL_2
static uint16_t g_adc_value = 0;

void hal_entry(void)
{
    R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);

    while(1)
    {
        R_ADC_ScanStart(&g_adc0_ctrl); // 启动采样

        // 等待完成（或使用中断）
        adc_status_t status;
        do {
            R_ADC_StatusGet(&g_adc0_ctrl, &status);
        } while(status.state);

        R_ADC_Read(&g_adc0_ctrl, POT_CHANNEL, &g_adc_value);

        // 计算电压（假设 VREF = 3.3V）
        float voltage = (g_adc_value / 4095.0f) * 3.3f;

        // 通过串口发送
        char msg[32];
        snprintf(msg, sizeof(msg), "ADC: %u (%.2fV)\r\n", g_adc_value, voltage);
        R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*)msg, strlen(msg));

        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

---

## 6. 实战2：DTC + GPT 定时采样双通道

### 6.1 配置要点

- **ADC Trigger**: `GPT0`（周期触发）
- **GPT0**: 配置为 1kHz PWM（不输出，仅触发）
- **DTC**: 启用，搬运 ADC 结果到 `g_adc_buffer[2]`
- **Channels**: 添加 `ADC_CHANNEL_2` 和 `ADC_CHANNEL_TEMP`

### 6.2 代码实现

```c
#include "hal_data.h"

#define SAMPLE_FREQ_HZ 1000
#define ADC_BUF_LEN  2
static uint16_t g_adc_buffer[ADC_BUF_LEN] = {0};
static volatile bool g_adc_complete = false;

void adc_callback(adc_callback_args_t *p_args)
{
    if (p_args->event == ADC_EVENT_SCAN_COMPLETE)
    {
        g_adc_complete = true;
    }
}

void hal_entry(void)
{
    R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    R_GPT_Open(&g_gpt0_ctrl, &g_gpt0_cfg); // 用作触发源
    R_GPT_Start(&g_gpt0_ctrl);             // 启动 GPT 触发

    while(1)
    {
        if (g_adc_complete)
        {
            g_adc_complete = false;

            uint16_t pot_val = g_adc_buffer[0];
            uint16_t temp_val = g_adc_buffer[1];

            float voltage = (pot_val / 4095.0f) * 3.3f;

            // 温度计算（参考手册公式）
            float temp_deg = ((float)temp_val - 1786) * (0.25f); // 近似

            char msg[64];
            snprintf(msg, sizeof(msg), "Pot: %.2fV, Temp: %.1f°C\r\n", voltage, temp_deg);
            R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*)msg, strlen(msg));
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

---

## 7. ADC 精度优化与校准建议

### 7.1 提高精度的方法

| 方法       | 说明                     |
| -------- | ---------------------- |
| 增加采样时间   | 提高采样电容充电时间，减少误差        |
| 使用内部基准电压 | 避免外部 VREF 波动           |
| 多次采样取平均  | 用 DTC + Addition 功能实现  |
| 禁用未使用引脚  | 配置为 Analog Input 降低干扰  |
| 使用差分输入   | 抗共模干扰                  |
| 电源去耦     | 在 VREFH 引脚加 0.1μF 陶瓷电容 |

---

### 7.2 温度传感器校准

RA6E2 内置温度传感器，但存在个体差异，建议：

1. 在已知温度下（如 25°C）读取 ADC 值
2. 计算偏移量，用于补偿
3. 或使用两点校准法

```c
// 示例：单点校准
#define ROOM_TEMP_ADC   1786   // 25°C 时实测值
#define ROOM_TEMP_REAL  25.0f

float calc_temperature(uint16_t adc_val)
{
    float temp = ((float)adc_val - ROOM_TEMP_ADC) * 0.25f + ROOM_TEMP_REAL;
    return temp;
}
```

---

## 8. ADC 函数速查表

| 函数名               | 用途       | 关键参数                    | 返回值/错误码        | 注意事项    |
| ----------------- | -------- | ----------------------- | -------------- | ------- |
| `R_ADC_Open`      | 初始化ADC   | `p_ctrl`, `p_cfg`       | `ALREADY_OPEN` | 必须最先调用  |
| `R_ADC_ScanStart` | 启动转换（软件） | -                       | -              | 软件触发模式  |
| `R_ADC_Read`      | 读取单通道    | `channel_id`, `*p_data` | `INVALID_ARG`  | 确保转换已完成 |
| `R_ADC_ReadAll`   | 读取所有通道   | `*p_data`               | -              | DTC模式推荐 |
| `R_ADC_ScanStop`  | 停止扫描     | -                       | -              | 连续模式下使用 |
| `R_ADC_StatusGet` | 查询是否忙    | `*p_status`             | -              | 轮询时使用   |
| `adc_callback`    | 中断回调     | `p_args->event`         | 无返回值           | 数据就绪标志  |

---

## 9. 开发建议

✅ **推荐做法**：

1. **高频率采样 → 使用 GPT + DTC 自动触发与搬运**
2. **多通道采集 → 启用 DTC + ReadAll**
3. **电池供电设备 → 采样后调用 `R_ADC_Close` 关闭省电**
4. **高精度需求 → 增加采样时间、使用平均模式**
5. **温度采集 → 实测校准，避免直接使用默认公式**

⛔ **避免做法**：

1. **频繁轮询 `StatusGet`**（应使用中断）
2. **在中断中执行复杂浮点运算**
3. **忽略通道间串扰**（高速采样切换通道时）

---
