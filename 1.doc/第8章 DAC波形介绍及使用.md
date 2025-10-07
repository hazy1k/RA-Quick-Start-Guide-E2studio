# 第八章 DAC波形介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **DAC（Digital-to-Analog Converter）模块架构、双通道独立输出、GPT定时触发、DTC自动波形生成、正弦波/三角波输出、精度优化与典型应用场景**，实现高保真模拟信号输出系统。

---

## 1. RA6E2 DAC 模块简介

RA6E2 集成 **12 位高精度 DAC**，支持双通道独立输出，是 RA6E2 中高性能模拟输出的核心模块。

### 1.1 核心特性

| 特性     | 说明                                              |
| ------ | ----------------------------------------------- |
| 分辨率    | 12 位（0 ~ 4095）                                  |
| 输出通道   | **2 通道（DAC0, DAC1）**，可独立配置                      |
| 输出电压范围 | 0 ~ VREF（典型为 3.3V，可外接高精度基准）                     |
| 建立时间   | 典型 5 μs（从数字输入到模拟稳定）                             |
| 触发方式   | 软件触发、GPT定时触发、外部触发                               |
| 数据传输   | 支持 DTC（Data Transfer Controller）自动加载数据，实现连续波形输出 |
| 缓冲输出   | ✅ 内置电压缓冲器，可直接驱动负载（如运放输入）                        |
| 低功耗模式  | 支持待机模式，关闭时功耗极低                                  |

📌 **典型应用场景**：

- 波形发生器（正弦、三角、锯齿波）
- 音频信号输出（低采样率）
- 模拟电压设定（电源控制）
- 电机控制参考电压
- 传感器模拟信号注入

---

### 1.2 DAC vs ADC 对比

| 项目     | ADC（上一章）            | DAC（本章）             |
| ------ | ------------------- | ------------------- |
| 功能     | 模拟 → 数字（采集）         | 数字 → 模拟（输出）         |
| 分辨率    | 12 位                | 12 位                |
| 触发方式   | 软件、GPT、DTC（输入）      | 软件、GPT、DTC（输出）      |
| 数据方向   | 外设 → 内存             | 内存 → 外设             |
| 典型外设联动 | GPT（定时采样） + DTC（搬运） | GPT（定时更新） + DTC（送数） |

> ✅ **DAC 是 ADC 的“镜像”**，但数据流向相反。

---

## 2. FSP 图形化配置 DAC

### 2.1 添加 DAC 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Analog` → `DAC`
3. 添加 `g_dac0`（对应 DAC0）

---

### 2.2 配置基本参数（Properties）

| 参数                    | 推荐值/说明                                  |
| --------------------- | --------------------------------------- |
| **Unit**              | 0（DAC0） 或 1（DAC1）                       |
| **Resolution**        | `12-bit`                                |
| **Reference Voltage** | `VREF`（外部） 或 `Internal`（内部 1.2V）        |
| **Trigger Mode**      | `Software`（软件触发） 或 `Hardware`（GPT 定时触发） |
| **Output Buffer**     | ✅ Enable（启用输出缓冲，提高驱动能力）                 |
| **DTC Support**       | ✅ Enable（允许 DTC 自动写入数据）                 |

---

### 2.3 配置 DTC 数据传输（关键！）

1. 添加 `DTC` 模块（Driver → Transfer → DTC）
2. 在 DAC 配置中：
   - `Transfer Info` → `Enable`
   - `Source Address`: 选择波形数据数组（如 `g_sine_wave`）
   - `Number of Transfers`: 波形点数（如 256）
   - `Mode`: `Repeat`（循环发送，实现连续波形）

> ✅ DTC + GPT 实现 **无 CPU 干预的连续波形输出**！

---

### 2.4 配置引脚（Pins 标签页）

- 找到 `DA0`（DAC0 输出）→ 设置为 `Analog Output`
- 确保未启用上下拉或数字功能

> 📌 注意：RA6E2 的 DAC 输出引脚是固定的（如 DA0 = P409, DA1 = P410），**不支持复用到其他引脚**。

---

## 3. DAC 相关 FSP 函数详解

> 📌 所有函数声明在 `r_dac.h` 中。

---

### 3.1 R_DAC_Open

#### 3.1.1 函数原型及功能

初始化 DAC 模块。

```c
fsp_err_t R_DAC_Open(dac_ctrl_t * const p_ctrl, dac_cfg_t const * const p_cfg)
```

#### 3.1.2 参数

- `dac_ctrl_t * const p_ctrl`：控制句柄（如 `g_dac0_ctrl`）
- `dac_cfg_t const * const p_cfg`：配置结构体（如 `g_dac0_cfg`）

#### 3.1.3 返回值

- `FSP_SUCCESS`：成功
- `FSP_ERR_ASSERTION`
- `FSP_ERR_ALREADY_OPEN`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **典型用法**：

```c
fsp_err_t err = R_DAC_Open(&g_dac0_ctrl, &g_dac0_cfg);

if (FSP_SUCCESS != err) while(1);
```

---

### 3.2 R_DAC_Write

#### 3.2.1 函数原型及功能

写入 DAC 数字值（12位），立即或在触发后更新模拟输出。

```c
fsp_err_t R_DAC_Write(dac_ctrl_t * const p_ctrl, uint16_t const data)
```

#### 3.2.2 参数

- `data`：要输出的数字值（0 ~ 4095）

📌 **注意**：

- 若 `Trigger Mode = Software`，调用 `Write` 后立即更新输出。
- 若 `Trigger Mode = Hardware`，`Write` 仅加载数据，由 GPT 触发更新。

---

### 3.3 R_DAC_Start

#### 3.3.1 函数原型及功能

启动 DAC 输出（使能 DAC 模块）。

```c
fsp_err_t R_DAC_Start(dac_ctrl_t * const p_ctrl)
```

📌 **重要**：`R_DAC_Open` 后 DAC **默认不工作**，必须调用 `Start`！

---

### 3.4 R_DAC_Stop

#### 3.4.1 函数原型及功能

停止 DAC 输出，进入低功耗状态。

```c
fsp_err_t R_DAC_Stop(dac_ctrl_t * const p_ctrl)
```

---

### 3.5 R_DAC_Close

#### 3.5.1 函数原型及功能

关闭 DAC 模块，释放资源。

```c
fsp_err_t R_DAC_Close(dac_ctrl_t * const p_ctrl)
```

---

## 4. DAC 中断回调函数

FSP 自动生成回调框架：

```c
void dac_callback(dac_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case DAC_EVENT_CYCLE_END:
            // DTC 一个周期传输完成（可用于切换波形）
            g_dac_cycle_complete = true;
            break;

        case DAC_EVENT_TRANSFER_ABORTED:
            // 传输被中止
            break;

        default:
            break;
    }
}
```

📌 **在 DTC 模式下，`DAC_EVENT_CYCLE_END` 标志一个波形周期结束**。

---

## 5. 实战1：软件触发 DAC 输出可变电压（电位器模拟）

```c
#include "hal_data.h"

void hal_entry(void)
{
    R_DAC_Open(&g_dac0_ctrl, &g_dac0_cfg);
    R_DAC_Start(&g_dac0_ctrl);

    uint16_t dac_value = 0;
    int8_t   direction = 1;

    while(1)
    {
        // 模拟电位器旋转
        dac_value += direction * 64;
        if (dac_value >= 4090) {
            dac_value = 4090;
            direction = -1;
        } else if (dac_value == 0) {
            direction = 1;
        }

        R_DAC_Write(&g_dac0_ctrl, dac_value);

        // 计算输出电压
        float voltage = (dac_value / 4095.0f) * 3.3f;

        char msg[32];
        snprintf(msg, sizeof(msg), "DAC: %u (%.2fV)\r\n", dac_value, voltage);
        R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*)msg, strlen(msg));

        R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

---

## 6. 实战2：DTC + GPT 生成连续正弦波

### 6.1 生成正弦波数据表

```c
#define SINE_TABLE_SIZE 256
#define DAC_MAX         4095
#define DAC_OFFSET      2048  // 0V 偏移（若需要双极性）
#define AMPLITUDE       2000  // 振幅（避免削顶）

static uint16_t g_sine_wave[SINE_TABLE_SIZE];

void init_sine_wave(void)
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++)
    {
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        float value = sinf(angle);
        g_sine_wave[i] = (uint16_t)(DAC_OFFSET + AMPLITUDE * value);
    }
}
```

### 6.2 FSP 配置要点

- **DAC Trigger Mode**: `Hardware`
- **Hardware Trigger Source**: `GPT0`（周期触发）
- **DTC**:
  - `Source`: `g_sine_wave`
  - `Destination`: `DAC0 DADR`（数据寄存器）
  - `Mode`: `Repeat`
- **GPT0**: 配置为 256kHz PWM（用于 1kHz 正弦波）

> 📌 计算：256kHz / 256点 = 1000Hz 正弦波

### 6.3 代码实现

```c
#include "hal_data.h"

void hal_entry(void)
{
    init_sine_wave();

    R_DAC_Open(&g_dac0_ctrl, &g_dac0_cfg);
    R_GPT_Open(&g_gpt0_ctrl, &g_gpt0_cfg);  // 启动 GPT 触发
    R_GPT_Start(&g_gpt0_ctrl);
    R_DAC_Start(&g_dac0_ctrl);              // 启动 DAC，DTC 自动送数

    while(1)
    {
        // 主循环可执行其他任务
        if (g_dac_cycle_complete)
        {
            g_dac_cycle_complete = false;
            // 可在此切换波形或调整频率
        }

        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

📌 **效果**：P409 引脚输出 **1kHz 正弦波**，CPU 占用率接近 0%！

---

## 7. 常见波形数据生成函数

### 7.1 三角波

```c
for (int i = 0; i < size; i++)
{
    if (i < size/2) {
        wave[i] = (4095 * 2 * i) / size;
    } else {
        wave[i] = 4095 - ((4095 * 2 * (i - size/2)) / size);
    }
}
```

### 7.2 锯齿波

```c
for (int i = 0; i < size; i++)
{
    wave[i] = (4095 * i) / size;
}
```

### 7.3 方波（占空比可调）

```c
int high_count = (duty_ratio * size) / 100;
for (int i = 0; i < size; i++)
{
    wave[i] = (i < high_count) ? 4095 : 0;
}
```

---

## 8. DAC 精度优化与注意事项

### 8.1 提高输出精度

| 方法           | 说明                        |
| ------------ | ------------------------- |
| 使用外部高精度 VREF | 如 REF3033（3.3V，0.5% 精度）   |
| 增加输出滤波       | 加 RC 低通滤波器（如 1kΩ + 100nF） |
| 避免电源噪声       | DAC 电源加 0.1μF 陶瓷电容去耦      |
| 降低更新频率       | 高频更新可能引起建立不完全             |
| 使用缓冲输出       | ✅ 启用 Output Buffer，提高驱动能力 |

---

### 8.2 建立时间与最大更新频率

```c
最大更新频率 < 1 / 建立时间
→ 典型 5μs 建立时间 → 最大更新频率 ~ 200kHz
```

📌 **注意**：超过此频率，输出波形将失真！

---

## 9. DAC 函数速查表

| 函数名            | 用途      | 关键参数              | 返回值/错误码        | 注意事项      |
| -------------- | ------- | ----------------- | -------------- | --------- |
| `R_DAC_Open`   | 初始化DAC  | `p_ctrl`, `p_cfg` | `ALREADY_OPEN` | 必须最先调用    |
| `R_DAC_Write`  | 写入数字值   | `data` (0~4095)   | `INVALID_ARG`  | 触发后更新模拟输出 |
| `R_DAC_Start`  | 启动DAC输出 | -                 | -              | Open后必须调用 |
| `R_DAC_Stop`   | 停止DAC   | -                 | -              | 进入低功耗     |
| `R_DAC_Close`  | 关闭DAC   | -                 | -              | 释放资源      |
| `dac_callback` | DTC完成回调 | `p_args->event`   | 无返回值           | 波形周期结束标志  |

---

## 10. 开发建议

✅ **推荐做法**：

1. **连续波形输出 → 使用 GPT + DTC 模式**
2. **高保真输出 → 加 RC 滤波，使用外部 VREF**
3. **双通道同步 → 使用同一 GPT 触发两个 DAC**
4. **低功耗应用 → 用完调用 `R_DAC_Stop`**
5. **音频输出 → 采样率 ≤ 20kHz，避免高频失真**

⛔ **避免做法**：

1. **在 `while(1)` 中频繁调用 `R_DAC_Write`**（应使用 DTC）
2. **更新频率超过 200kHz**（建立时间不足）
3. **忽略电源去耦**（导致输出噪声大）

---


