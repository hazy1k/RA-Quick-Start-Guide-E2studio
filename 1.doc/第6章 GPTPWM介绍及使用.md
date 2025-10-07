# 第六章 GPTPWM介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **GPT（General PWM Timer）模块架构、高速 PWM 输出、多通道控制、互补输出、死区插入、编码器模式、FSP配置与代码实现**，为电机控制、电源管理、LED调光等高性能应用打下坚实基础。

---

## 1. RA6E2 GPT 模块简介

RA6E2 集成多个 **GPT（General PWM Timer）** 通道（如 GPT0~GPT15），是功能最强大的定时器之一，专为 **高精度、高速 PWM 控制** 设计。

### 1.1 GPT 核心特性

| 特性             | 说明                                   |
| -------------- | ------------------------------------ |
| 时钟源            | PCLKA / PCLKB / 外部时钟（最高支持 240MHz）    |
| 计数器宽度          | 16 位或 32 位（可配置）                      |
| PWM 频率范围       | **1Hz ~ 数 MHz**（理论可达 120MHz，受限于外设总线） |
| 支持模式           | 边沿对齐、中心对齐 PWM、单脉冲、捕获、编码器接口           |
| 输出引脚           | GTIOA / GTIOB（可映射到多个GPIO）            |
| 互补输出           | ✅ 支持 GTIOA/GTIOB 互补输出（用于 H 桥、电机驱动）   |
| 死区插入（Deadtime） | ✅ 硬件自动插入死区，防止桥臂直通                    |
| 中断支持           | 比较匹配、周期结束、捕获事件、错误中断                  |
| DMA/DTC 支持     | ✅ 支持 DTC 触发 ADC、DMA 传输，实现无 CPU 干预控制  |

📌 **典型应用场景**：

- 电机控制（BLDC、步进）
- 开关电源（SMPS）
- LED 调光（RGB、背光）
- 编码器测速
- 数字电源、逆变器

---

### 1.2 GPT vs AGT 对比（回顾）

| 功能      | GPT（本章）         | AGT（上一章）                 |
| ------- | --------------- | ------------------------ |
| 时钟源     | PCLK（最高 240MHz） | LOCO/Subclock（32.768kHz） |
| PWM 频率  | **Hz ~ MHz**    | **0.5Hz ~ 8kHz**         |
| 占空比分辨率  | 高（16/32位）       | 低（受限于低频）                 |
| 低功耗模式运行 | ❌ 停止            | ✅ 支持                     |
| 功耗      | 正常（mA级）         | 极低（μA级）                  |
| 互补输出+死区 | ✅ 完整支持          | ❌ 不支持                    |
| 应用场景    | 高性能控制、高速调光      | 低功耗唤醒、呼吸灯                |

> ✅ **选择原则**：
> 
> - 要 **高速、高精度 PWM** → 用 **GPT**
> - 要 **低功耗、低频 PWM** → 用 **AGT**

---

## 2. FSP 图形化配置 GPT PWM

### 2.1 添加 GPT 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Timers` → `GPT`
3. 添加 `g_gpt0`（对应 GPT0）

---

### 2.2 配置基本参数（Properties）

| 参数                       | 推荐值/说明                                              |
| ------------------------ | --------------------------------------------------- |
| **Channel**              | 0 ~ 15（选择 GPT0）                                     |
| **Mode**                 | `PWM`                                               |
| **Clock Source**         | `PCLKA` / `PCLKB`（建议启用 PLL 后使用 200MHz 或 240MHz）     |
| **Count Direction**      | `Up`（边沿对齐）或 `Up/Down`（中心对齐）                         |
| **Period (counts)**      | 决定 PWM 周期（如 1000 → 1kHz @ 1MHz）                     |
| **Duty Cycle (%)**       | 初始占空比（如 50）                                         |
| **Start Mode**           | `Software`（软件启动）或 `Hardware`（外部触发）                  |
| **Output Pin**           | ✅ Enable GTIOA Output（也可启用 GTIOB）                   |
| **Complementary Output** | ✅ Enable Complementary Output（生成 GTIOA_N / GTIOB_N） |
| **Deadtime**             | 设置死区时间（如 100ns），防止 H 桥直通                            |

---

### 2.3 配置引脚映射（Pins 标签页）

- 找到 `GPT0 GTIOA` → 分配到物理引脚（如 P400）
- 找到 `GPT0 GTIOA_N`（若启用互补）→ 分配到另一引脚（如 P401）
- Mode: `Peripheral` → Peripheral: `GPT0 GTIOA`

> ✅ FSP 会自动生成初始化代码和引脚别名。

---

### 2.4 配置中断（可选）

在 `Interrupts` 标签页：

- 勾选 `GPT0 Interrupt`
- 设置优先级（如 8）

> ✅ 中断可用于：
> 
> - 周期同步
> - 动态更新占空比
> - 错误处理（如直通检测）

---

## 3. GPT PWM 核心参数计算

### 3.1 PWM 频率计算公式

```c
PWM 频率 (Hz) = 时钟频率 (Hz) / (Period + 1)
```

📌 **示例**：

- 时钟源 = PCLKA = 200 MHz
- Period = 1999
- → PWM 频率 = 200,000,000 / 2000 = **100 kHz**

### 3.2 占空比计算

```c
占空比 (%) = (Compare Match Value / Period) × 100%
```

📌 **示例**：

- Period = 1999
- Compare = 500
- → 占空比 = (500 / 2000) × 100% = **25%**

---

### 3.3 常用频率与周期对照表（PCLKA = 200MHz）

| 目标频率    | Period 值 | 说明          |
| ------- | -------- | ----------- |
| 1 kHz   | 199999   | 低速电机控制      |
| 20 kHz  | 9999     | 超声波、无噪声电机驱动 |
| 100 kHz | 1999     | 开关电源（SMPS）  |
| 1 MHz   | 199      | 高速 LED 调光   |
| 10 MHz  | 19       | 极高速应用（注意布线） |

> ⚠️ **注意**：频率越高，占空比调节分辨率越低。

---

## 4. GPT 相关 FSP 函数详解

> 📌 所有函数声明在 `r_gpt.h` 中。

---

### 4.1 R_GPT_Open

#### 4.1.1 函数原型及功能

初始化 GPT 定时器。

```c
fsp_err_t R_GPT_Open(gpt_ctrl_t * const p_ctrl, gpt_cfg_t const * const p_cfg)
```

#### 4.1.2 参数

- `gpt_ctrl_t * const p_ctrl`：控制句柄（如 `g_gpt0_ctrl`）
- `gpt_cfg_t const * const p_cfg`：配置结构体（如 `g_gpt0_cfg`）

#### 4.1.3 返回值

- `FSP_SUCCESS`：成功
- `FSP_ERR_ASSERTION`
- `FSP_ERR_ALREADY_OPEN`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **典型用法**：

```c
fsp_err_t err = R_GPT_Open(&g_gpt0_ctrl, &g_gpt0_cfg);
if (FSP_SUCCESS != err) while(1);
```

---

### 4.2 R_GPT_Start

#### 4.2.1 函数原型及功能

启动 PWM 输出。

```c
fsp_err_t R_GPT_Start(gpt_ctrl_t * const p_ctrl)
```

📌 **注意**：Open 后 PWM **不会自动启动**，必须调用 `Start`！

---

### 4.3 R_GPT_Stop

#### 4.3.1 函数原型及功能

停止 PWM 输出。

```c
fsp_err_t R_GPT_Stop(gpt_ctrl_t * const p_ctrl)
```

📌 **用途**：紧急停止、进入低功耗前关闭

---

### 4.4 R_GPT_DutyCycleSet

#### 4.4.1 函数原型及功能

动态修改 PWM 占空比。

```c
fsp_err_t R_GPT_DutyCycleSet(gpt_ctrl_t * const p_ctrl, uint32_t const duty_cycle_counts, gpt_io_pin_t const pin)
```

#### 4.4.2 参数

- `duty_cycle_counts`：比较寄存器值（0 ~ Period）
- `pin`：指定输出引脚（`GPT_IO_PIN_GTIOA` 或 `GPT_IO_PIN_GTIOB`）

📌 **用法示例（设置 75% 占空比）**：

```c
R_GPT_DutyCycleSet(&g_gpt0_ctrl, 1500, GPT_IO_PIN_GTIOA); // Period=1999
```

---

### 4.5 R_GPT_PeriodSet

#### 4.5.1 函数原型及功能

动态修改 PWM 周期（频率）。

```c
fsp_err_t R_GPT_PeriodSet(gpt_ctrl_t * const p_ctrl, uint32_t const period_counts)
```

📌 **注意**：修改周期后，原占空比比例可能失效，建议重新设置。

---

### 4.6 R_GPT_InfoGet

#### 4.6.1 函数原型及功能

获取当前计数值、周期值、运行状态。

```c
fsp_err_t R_GPT_InfoGet(gpt_ctrl_t * const p_ctrl, gpt_info_t * const p_info)
```

---

### 4.7 R_GPT_CallbackSet

#### 4.7.1 函数原型及功能

运行时更换中断回调函数。

```c
fsp_err_t R_GPT_CallbackSet(gpt_ctrl_t * const p_ctrl, void (* p_callback)(timer_callback_args_t *), void const * const p_context, void ** const pp_context)
```

---

## 5. GPT 中断回调函数

FSP 自动生成回调框架：

```c
void gpt_callback(timer_callback_args_t *p_args)
{
    if (p_args->event == TIMER_EVENT_PWM_COMPARE_A)
    {
        // GTIOA 比较匹配（可在此更新占空比）
    }
    else if (p_args->event == TIMER_EVENT_PWM_PERIOD)
    {
        // 周期结束（可用于同步其他外设）
    }
    else if (p_args->event == TIMER_EVENT_ERROR)
    {
        // 错误发生（如直通检测）
        R_GPT_Stop(&g_gpt0_ctrl);
    }
}
```

---

## 6. 实战1：双通道 PWM 输出（LED调光 + 电机控制）

```c
#include "hal_data.h"

void hal_entry(void)
{
    R_GPT_Open(&g_gpt0_ctrl, &g_gpt0_cfg); // LED PWM
    R_GPT_Open(&g_gpt1_ctrl, &g_gpt1_cfg); // Motor PWM

    R_GPT_Start(&g_gpt0_ctrl);
    R_GPT_Start(&g_gpt1_ctrl);

    uint8_t led_level = 0;
    int8_t  dir = 1;

    while(1)
    {
        // 呼吸灯效果
        R_GPT_DutyCycleSet(&g_gpt0_ctrl, (2000 * led_level) / 100, GPT_IO_PIN_GTIOA);

        led_level += dir;
        if (led_level >= 100) { led_level = 100; dir = -1; }
        if (led_level == 0)   { led_level = 0;   dir = 1; }

        // 按键切换电机速度
        bsp_io_level_t key;
        R_IOPORT_PinRead(&g_ioport_ctrl, KEY0_PIN, &key);
        if (key == BSP_IO_LEVEL_LOW)
        {
            R_GPT_DutyCycleSet(&g_gpt1_ctrl, 1600, GPT_IO_PIN_GTIOA); // 80%
        }
        else
        {
            R_GPT_DutyCycleSet(&g_gpt1_ctrl, 800, GPT_IO_PIN_GTIOA);  // 40%
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

---

## 7. 实战2：互补 PWM + 死区（H桥电机驱动）

### 7.1 配置要点（FSP）

- **Mode**: `PWM`
- **Complementary Output**: ✅ Enable
- **Deadtime**: 100ns（防止上下桥臂直通）
- **Polarity**: GTIOA 正相，GTIOA_N 反相

### 7.2 代码实现

```c
void motor_forward(void)
{
    R_GPT_DutyCycleSet(&g_gpt0_ctrl, 1500, GPT_IO_PIN_GTIOA);  // 高电平驱动上桥
    // GTIOA_N 自动为低，下桥关闭
}

void motor_brake(void)
{
    R_GPT_DutyCycleSet(&g_gpt0_ctrl, 1500, GPT_IO_PIN_GTIOA);  // 上桥开
    R_GPT_DutyCycleSet(&g_gpt0_ctrl, 1500, GPT_IO_PIN_GTIOA_N); // 下桥也开 → 短路制动
}

void motor_stop(void)
{
    R_GPT_DutyCycleSet(&g_gpt0_ctrl, 0, GPT_IO_PIN_GTIOA);      // 全关
    R_GPT_DutyCycleSet(&g_gpt0_ctrl, 0, GPT_IO_PIN_GTIOA_N);
}
```

📌 **安全机制**：硬件死区确保不会同时导通上下桥臂！

---

## 8. GPT 函数速查表

| 函数名                  | 用途        | 关键参数                       | 返回值/错误码        | 注意事项         |
| -------------------- | --------- | -------------------------- | -------------- | ------------ |
| `R_GPT_Open`         | 初始化GPT    | `p_ctrl`, `p_cfg`          | `ALREADY_OPEN` | 必须最先调用       |
| `R_GPT_Start`        | 启动PWM     | -                          | -              | Open后必须调用    |
| `R_GPT_Stop`         | 停止PWM     | -                          | -              | 紧急停止用        |
| `R_GPT_DutyCycleSet` | 设置占空比     | `duty_cycle_counts`, `pin` | `INVALID_ARG`  | 0 ~ period   |
| `R_GPT_PeriodSet`    | 设置周期（改频率） | `period_counts`            | `INVALID_ARG`  | 修改后建议重设占空比   |
| `R_GPT_InfoGet`      | 获取运行状态    | `*p_info`                  | -              | 查询计数值等       |
| `gpt_callback`       | 中断回调      | `p_args->event`            | 无返回值           | 可用于动态调节、错误处理 |

---

## 9. 开发建议与注意事项

✅ **推荐做法**：

1. **高速 PWM（>1kHz）优先使用 GPT**。
2. **电机/H桥驱动务必启用互补输出 + 死区**。
3. **在中断中更新占空比，避免相位抖动**。
4. **使用 DTC 触发 ADC，实现数字电源闭环控制**。
5. **中心对齐 PWM 用于电机控制，减少 EMI**。

⛔ **避免做法**：

1. **在 `while(1)` 中频繁调用 `DutyCycleSet`**（应使用中断或定时器）。
2. **死区时间设置过短**（< 50ns，可能导致直通）。
3. **忽略错误中断**（如直通检测）。
4. **在低功耗场景使用 GPT**（应改用 AGT）。

---




