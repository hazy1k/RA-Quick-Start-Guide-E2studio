# 第五章 AGT低功耗定时器介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **AGT（Asynchronous General-purpose Timer）模块架构、低功耗特性、PWM输出配置、FSP图形化设置、动态占空比调节、与低功耗模式联动**，实现 μA 级功耗下的精准定时与 PWM 控制。

---

## 1. RA6E2 AGT 模块简介

AGT（Asynchronous General-purpose Timer）是 RA6E2 专为 **低功耗场景** 设计的通用定时器，最大特点是：

✅ **可在深度睡眠（Software Standby）模式下继续运行**  
✅ **使用独立低速时钟源（LOCO 32.768kHz 或 Subclock 32.768kHz）**  
✅ **支持定时中断、PWM输出、脉冲计数**  
✅ **极低功耗（典型值 < 1μA）**

📌 **典型应用场景**：

- 电池供电设备的 PWM 背光控制
- 实时时钟（RTC）辅助定时
- 低功耗状态下的周期性唤醒
- 电机低速 PWM 驱动（如振动马达）

---

### 1.1 AGT vs GPT 对比

| 特性      | AGT（本章）                     | GPT（通用PWM定时器）   |
| ------- | --------------------------- | --------------- |
| 时钟源     | LOCO / Subclock (32.768kHz) | PCLK（最高 120MHz） |
| 低功耗模式运行 | ✅ 支持（Software Standby）      | ❌ 停止            |
| PWM频率范围 | ~0.5Hz ~ 8kHz（典型）           | Hz ~ MHz        |
| 分辨率     | 16位计数器                      | 16/32位          |
| 功耗      | **极低（μA级）**                 | 正常（mA级）         |
| 应用场景    | 低功耗PWM、定时唤醒                 | 高速PWM、编码器、输入捕获  |

> 💡 **选择建议**：
> 
> - 需要 **低功耗 + PWM** → 选 **AGT**
> - 需要 **高频率 + 高精度** → 选 **GPT**

---

### 1.2 AGT 通道与引脚

RA6E2 提供 **AGT0 和 AGT1** 两个通道，每个通道支持：

- **PWM 输出引脚**：AGTO（可映射到多个GPIO）
- **外部时钟输入**：AGTEE（用于脉冲计数）
- **中断输出**：支持周期中断、比较匹配中断

📌 **引脚复用**：通过 FSP Pin Configurator 灵活分配，例如：

- AGT0 AGTO → P400, P401, P500...
- AGT1 AGTO → P402, P403, P501...

---

## 2. FSP 图形化配置 AGT PWM

### 2.1 添加 AGT 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Timers` → `AGT`
3. 选择通道（如 `g_agt0`）

---

### 2.2 配置基本参数（Properties）

| 参数                  | 推荐值/说明                                |
| ------------------- | ------------------------------------- |
| **Channel**         | 0 或 1                                 |
| **Mode**            | `Periodic`（定时）或 `PWM`                 |
| **Clock Source**    | `LOCO`（内部）或 `Subclock`（外部32.768kHz晶振） |
| **Period (counts)** | 决定 PWM 周期（见下文计算）                      |
| **Duty Cycle (%)**  | 初始占空比（如 50）                           |
| **Count Direction** | `Up`（递增）                              |
| **Output Pin**      | ✅ Enable AGTO Output                  |

---

### 2.3 配置引脚映射（Pins 标签页）

- 找到 `AGT0 AGTO` → 分配到物理引脚（如 P400）
- Mode: `Peripheral` → Peripheral: `AGT0 AGTO`

---

### 2.4 配置中断（可选）

- 在 `Interrupts` 标签页勾选 `AGT Interrupt`
- 设置优先级（如 12，低于SCI等通信中断）

> ✅ 中断可用于周期性唤醒或事件通知。

---

## 3. AGT PWM 核心参数计算

### 3.1 PWM 频率计算公式

```c
PWM 频率 (Hz) = 时钟源频率 (Hz) / Period (counts)
```

📌 **示例**：

- 时钟源 = LOCO (32768 Hz)
- Period = 4096
- → PWM 频率 = 32768 / 4096 = **8 Hz**

### 3.2 占空比计算

```c
高电平计数值 = Period × (Duty Cycle / 100)
```

📌 **示例**：

- Period = 4096
- Duty Cycle = 25%
- → 高电平计数 = 4096 × 0.25 = **1024**

---

### 3.3 常用频率配置表（LOCO 32.768kHz）

| 目标频率    | Period 值 | 说明              |
| ------- | -------- | --------------- |
| 8 Hz    | 4096     | LED 呼吸灯（慢）      |
| 64 Hz   | 512      | 电机控制（避免人耳可闻）    |
| 256 Hz  | 128      | 蜂鸣器（中频）         |
| 1024 Hz | 32       | 最高实用频率（受限于LOCO） |

> ⚠️ **注意**：LOCO 精度约 ±10%，对频率精度要求高的场景请使用 **Subclock（外部32.768kHz晶振）**。

---

## 4. AGT 相关 FSP 函数详解

> 📌 所有函数声明在 `r_agt.h` 中。

---

### 4.1 R_AGT_Open

#### 4.1.1 函数原型及功能

初始化并打开 AGT 定时器。

```c
fsp_err_t R_AGT_Open(agt_ctrl_t * const p_ctrl, agt_cfg_t const * const p_cfg)
```

#### 4.1.2 参数

- `agt_ctrl_t * const p_ctrl`：AGT 控制句柄（用户定义，如 `g_agt0_ctrl`）
- `agt_cfg_t const * const p_cfg`：配置结构体（FSP 自动生成，如 `g_agt0_cfg`）

#### 4.1.3 返回值

- `FSP_SUCCESS`：初始化成功
- `FSP_ERR_ASSERTION`：参数为 NULL
- `FSP_ERR_ALREADY_OPEN`：已打开
- `FSP_ERR_INVALID_ARGUMENT`：配置非法

📌 **典型用法**：

```c
fsp_err_t err = R_AGT_Open(&g_agt0_ctrl, &g_agt0_cfg);
if (FSP_SUCCESS != err) while(1);
```

---

### 4.2 R_AGT_Start

#### 4.2.1 函数原型及功能

启动 AGT 定时器/PWM 输出。

```c
fsp_err_t R_AGT_Start(agt_ctrl_t * const p_ctrl)
```

📌 **注意**：  
AGT 在 `R_AGT_Open` 后默认 **不启动**，必须手动调用 `R_AGT_Start`！

---

### 4.3 R_AGT_Stop

#### 4.3.1 函数原型及功能

停止 AGT 定时器/PWM 输出。

```c
fsp_err_t R_AGT_Stop(agt_ctrl_t * const p_ctrl)
```

📌 **低功耗应用**：在进入深度睡眠前可不停止，AGT 会继续运行并唤醒系统！

---

### 4.4 R_AGT_PeriodSet

#### 4.4.1 函数原型及功能

动态修改 PWM 周期（频率）。

```c
fsp_err_t R_AGT_PeriodSet(agt_ctrl_t * const p_ctrl, uint32_t const period_counts)
```

#### 4.4.2 参数

- `period_counts`：新的周期计数值（1~65535）

📌 **用法示例（改变频率）**：

```c
// 改为 64Hz (32768/512)
R_AGT_PeriodSet(&g_agt0_ctrl, 512);
```

---

### 4.5 R_AGT_DutyCycleSet

#### 4.5.1 函数原型及功能

动态修改 PWM 占空比。

```c
fsp_err_t R_AGT_DutyCycleSet(agt_ctrl_t * const p_ctrl, uint32_t const duty_cycle_counts, agt_pwm_pin_t const pin)
```

#### 4.5.2 参数

- `duty_cycle_counts`：高电平计数值（0 ~ period-1）
- `pin`：指定引脚（通常 `AGT_PWM_PIN_AGTO`）

📌 **用法示例（设置25%占空比）**：

```c
uint32_t period;
R_AGT_PeriodGet(&g_agt0_ctrl, &period); // 先获取当前周期
R_AGT_DutyCycleSet(&g_agt0_ctrl, period/4, AGT_PWM_PIN_AGTO);
```

---

### 4.6 R_AGT_PeriodGet

#### 4.6.1 函数原型及功能

获取当前周期值。

```c
fsp_err_t R_AGT_PeriodGet(agt_ctrl_t * const p_ctrl, uint32_t * const p_period_counts)
```

---

### 4.7 R_AGT_InfoGet

#### 4.7.1 函数原型及功能

获取 AGT 当前状态（计数值、运行状态等）。

```c
fsp_err_t R_AGT_InfoGet(agt_ctrl_t * const p_ctrl, agt_info_t * const p_info)
```

### 4.8 R_AGT_CallbackSet（动态设置回调）

#### 4.8.1 函数原型及功能

运行时修改中断回调函数。

```c
fsp_err_t R_AGT_CallbackSet(agt_ctrl_t * const p_ctrl, void (* p_callback)(timer_callback_args_t *), void const * const p_context, void ** const pp_context)📌 **高级用法**：实现多状态机管理。
```

---

## 5. AGT 中断回调函数

当配置中断后，FSP 生成回调框架：

```c
void agt_callback(timer_callback_args_t *p_args)
{
    if (p_args->event == TIMER_EVENT_EXPIRED)
    {
        // 周期中断触发
        // 可用于：翻转LED、采集传感器、唤醒系统等
    }
}
```

📌 **在低功耗模式下**：此中断可唤醒 MCU！

---

## 6. 实战1：AGT PWM 呼吸灯（动态占空比）

```c
#include "hal_data.h"

#define PWM_PERIOD 4096  // 8Hz @ 32768Hz

void hal_entry(void)
{
    fsp_err_t err = R_AGT_Open(&g_agt0_ctrl, &g_agt0_cfg);
    if (FSP_SUCCESS != err) while(1);

    R_AGT_Start(&g_agt0_ctrl);

    uint32_t duty = 0;
    int8_t direction = 1; // 1=increase, -1=decrease

    while(1)
    {
        R_AGT_DutyCycleSet(&g_agt0_ctrl, duty, AGT_PWM_PIN_AGTO);
        
        duty += direction * 64; // 步进64
        if (duty >= PWM_PERIOD - 1) {
            duty = PWM_PERIOD - 1;
            direction = -1;
        } else if (duty == 0) {
            direction = 1;
        }

        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS); // 调节变化速度
    }
}
```

📌 **效果**：LED 亮度缓慢渐变，实现“呼吸”效果，**整机功耗 < 50μA**！

---

## 7. 实战2：AGT + 低功耗模式定时唤醒

```c
#include "hal_data.h"

volatile bool g_wakeup_flag = false;

void agt_callback(timer_callback_args_t *p_args)
{
    if (p_args->event == TIMER_EVENT_EXPIRED)
    {
        g_wakeup_flag = true; // 设置唤醒标志
    }
}

void enter_low_power_mode(void)
{
    // 关闭无关外设时钟
    R_CGC_PeripheralClockDisable(&g_cgc_ctrl, CGC_PERIPHERAL_SCI0);
    R_CGC_PeripheralClockDisable(&g_cgc_ctrl, CGC_PERIPHERAL_ADC0);

    // 进入 Software Standby 模式
    R_BSP_SoftwareStandbyEnter();
    // MCU 在此处“休眠”，直到 AGT 中断唤醒
}

void hal_entry(void)
{
    R_AGT_Open(&g_agt0_ctrl, &g_agt0_cfg);
    R_AGT_Start(&g_agt0_ctrl); // AGT 在休眠时继续运行！

    while(1)
    {
        if (g_wakeup_flag)
        {
            g_wakeup_flag = false;
            // 执行唤醒任务：读取传感器、发送数据等
            R_SCI_UART_Write(&g_uart0_ctrl, "Wakeup!\r\n", 9);
        }

        enter_low_power_mode(); // 每次任务后重新休眠
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

📌 **功耗实测**：休眠时电流 **< 5μA**（仅 AGT + RTC 运行）！

---

## 8. AGT 函数速查表

| 函数名                  | 用途        | 关键参数                       | 返回值/错误码        | 注意事项         |
| -------------------- | --------- | -------------------------- | -------------- | ------------ |
| `R_AGT_Open`         | 初始化AGT    | `p_ctrl`, `p_cfg`          | `ALREADY_OPEN` | 必须最先调用       |
| `R_AGT_Start`        | 启动定时器/PWM | -                          | -              | Open后必须调用    |
| `R_AGT_Stop`         | 停止定时器     | -                          | -              | 休眠前可不调用      |
| `R_AGT_PeriodSet`    | 设置周期（改频率） | `period_counts`            | `INVALID_ARG`  | 范围1~65535    |
| `R_AGT_DutyCycleSet` | 设置占空比     | `duty_cycle_counts`, `pin` | `INVALID_ARG`  | 0 ~ period-1 |
| `R_AGT_PeriodGet`    | 获取当前周期    | `*p_period_counts`         | -              | 用于动态计算       |
| `R_AGT_InfoGet`      | 获取运行状态    | `*p_info`                  | -              | 查询计数值等       |
| `agt_callback`       | 中断回调      | `p_args->event`            | 无返回值           | 可唤醒休眠系统      |

---

## 9. 开发建议与注意事项

✅ **推荐做法**：

1. **低功耗PWM首选AGT**，GPT仅用于高速场景。
2. **频率 > 1kHz 时考虑GPT**，AGT受LOCO限制。
3. **动态调节使用 `PeriodSet` + `DutyCycleSet`**。
4. **休眠唤醒系统，AGT中断优先级设为最高**。
5. **使用外部32.768kHz晶振（Subclock）提高精度**。

⛔ **避免做法**：

1. **在AGT中断回调中执行耗时操作**（阻塞唤醒）。
2. **未调用 `R_AGT_Start` 直接使用**。
3. **占空比设置超出范围（0 ~ period-1）**。
4. **在非低功耗场景过度使用AGT**（功能受限）。

---


