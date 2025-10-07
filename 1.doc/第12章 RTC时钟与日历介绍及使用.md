# 第十二章 RTC时钟与日历介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **RTC（Real-Time Clock）模块配置、日历时钟（年/月/日/时/分/秒）管理、闹钟中断、周期性中断、低功耗唤醒、与外部32.768kHz晶振联动**，构建高精度、低功耗的时间管理系统。

---

## 1. RA6E2 RTC 模块简介

RA6E2 集成 **硬件实时时钟（RTC）模块**，可在 **低功耗待机模式（Software Standby、Deep Standby）下持续运行**，是电池供电设备、数据记录仪、智能家居等应用的核心时间基准。

### 1.1 RTC 核心特性

| 特性    | 说明                                              |
| ----- | ----------------------------------------------- |
| 时钟源   | 外部 **32.768kHz 晶振（Subclock）** 或内部 LOCO（不推荐，精度低） |
| 计时精度  | 依赖外部晶振，典型 ±20ppm（约 ±1分钟/月）                      |
| 支持模式  | 32-bit Binary Counter（自由计数）、Calendar Mode（日历模式） |
| 日历范围  | 2000 ~ 2099 年（BCD编码）                            |
| 中断支持  | ✅ 闹钟中断（Alarm）、周期性中断（PRD）、秒中断（CUP）               |
| 低功耗唤醒 | ✅ 在 Deep Standby 模式下通过闹钟唤醒 MCU                  |
| 寄存器备份 | RTC 寄存器由 `VBACKUP` 供电，掉电不丢失                     |
| 校准功能  | ✅ 支持数字校准（±90ppm），补偿晶振偏差                         |

📌 **典型应用场景**：

- 电子时钟、万年历
- 数据记录仪（带时间戳）
- 定时开关（如智能插座）
- 低功耗传感器周期唤醒
- 事件日志记录

---

### 1.2 RTC 模式说明

| 模式                 | 说明                              |
| ------------------ | ------------------------------- |
| **Calendar Mode**  | ✅ 推荐。自动管理年、月、日、时、分、秒，支持闰年，可设置闹钟 |
| **Binary Counter** | 32位计数器，每秒加1，适合长时间运行计数，但需软件解析时间  |

> 📌 **本章重点讲解 `Calendar Mode`**。

---

## 2. 硬件准备：外部32.768kHz晶振

### 2.1 接线说明

| 引脚       | 连接说明   |
| -------- | ------ |
| `SUBOUT` | 接晶振一端  |
| `SUBIN`  | 接晶振另一端 |
| `VSS`    | 接地     |

📌 **建议**：

- 使用 **12.5pF 负载电容**（接在 SUBIN/SUBOUT 到地）
- 晶振走线尽量短，远离高频信号
- 电源加 0.1μF 陶瓷电容去耦

---

## 3. FSP 图形化配置 RTC

### 3.1 添加 RTC 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+`
2. 选择 `Driver` → `Timers` → `RTC`

---

### 3.2 配置基本参数（Properties）

| 参数                     | 推荐值/说明                               |
| ---------------------- | ------------------------------------ |
| **Mode**               | `Calendar`                           |
| **Clock Source**       | `Subclock`（必须使用外部晶振）                 |
| **Output Signal**      | `None` 或 `PRD`（周期性中断输出）              |
| **Alarm Mode**         | `Daily`（每天触发） 或 `Specific`（指定年月日时分秒） |
| **Enable Calibration** | ✅ Enable（启用数字校准）                     |
| **Calibration Value**  | -30 ~ +90 ppm（根据实测误差调整，如 +20ppm）     |

---

### 3.3 配置闹钟（Alarm）

- `Alarm Day of Month`: `Any`（每天）
- `Alarm Hour`: `10`
- `Alarm Minute`: `30`
- `Alarm Second`: `0`

📌 **对应每天 10:30:00 触发闹钟中断**。

---

### 3.4 配置中断（Interrupts 标签页）

- 勾选 `RTC Alarm Interrupt`
- 勾选 `RTC Periodic Interrupt`（可选）
- 设置中断优先级（如 12）

> ✅ 闹钟中断可唤醒 Deep Standby 模式！

---

## 4. RTC 相关 FSP 函数详解

> 📌 所有函数声明在 `r_rtc.h` 中。

---

### 4.1 R_RTC_Open

#### 4.1.1 函数原型及功能

初始化 RTC 模块。

```c
fsp_err_t R_RTC_Open(rtc_ctrl_t * const p_ctrl, rtc_cfg_t const * const p_cfg)
```

#### 4.1.2 参数

- `rtc_ctrl_t * const p_ctrl`：控制句柄（如 `g_rtc_ctrl`）
- `rtc_cfg_t const * const p_cfg`：配置结构体（如 `g_rtc_cfg`）

#### 4.1.3 返回值

- `FSP_SUCCESS`：成功
- `FSP_ERR_ASSERTION`
- `FSP_ERR_ALREADY_OPEN`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **典型用法**：

```c
fsp_err_t err = R_RTC_Open(&g_rtc_ctrl, &g_rtc_cfg);
if (FSP_SUCCESS != err) while(1);
```

---

### 4.2 R_RTC_CalendarTimeSet

#### 4.2.1 函数原型及功能

设置当前日历时间。

```c
fsp_err_t R_RTC_CalendarTimeSet(rtc_ctrl_t * const p_ctrl, rtc_time_t * const p_time, rtc_unit_t unit)
```

#### 4.2.2 参数

- `p_time`：指向时间结构体
- `unit`：`RTC_UNIT_SECONDS`（设置秒精度）

#### 4.2.3 时间结构体

```c
rtc_time_t current_time = {
    .tm_sec  = 0,
    .tm_min  = 30,
    .tm_hour = 10,
    .tm_mday = 1,
    .tm_mon  = 1,      // 1~12
    .tm_year = 2024,   // 2000~2099
    .tm_wday = 3,      // 0=Sun, 1=Mon, ... 6=Sat
};
```

📌 **用法示例**：

```c
R_RTC_CalendarTimeSet(&g_rtc_ctrl, &current_time, RTC_UNIT_SECONDS);
```

---

### 4.3 R_RTC_CalendarTimeGet

#### 4.3.1 函数原型及功能

获取当前日历时间。

```c
fsp_err_t R_RTC_CalendarTimeGet(rtc_ctrl_t * const p_ctrl, rtc_time_t * const p_time)
```

📌 **用法示例**：

```c
rtc_time_t now;
R_RTC_CalendarTimeGet(&g_rtc_ctrl, &now);
printf("Time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
       now.tm_year, now.tm_mon, now.tm_mday,
       now.tm_hour, now.tm_min, now.tm_sec);
```

---

### 4.4 R_RTC_AlarmSet

#### 4.4.1 函数原型及功能

动态修改闹钟时间。

```c
fsp_err_t R_RTC_AlarmSet(rtc_ctrl_t * const p_ctrl, rtc_alarm_time_t * const p_alarm_time)
```

📌 **用法示例（设置每天 8:00 闹钟）**：

```c
rtc_alarm_time_t alarm = {
    .time.tm_hour = 8,
    .time.tm_min  = 0,
    .time.tm_sec  = 0,
    .mask         = RTC_ALARM_MASK_HOUR | RTC_ALARM_MASK_MINUTE | RTC_ALARM_MASK_SECOND,
};
R_RTC_AlarmSet(&g_rtc_ctrl, &alarm);
```

---

### 4.5 R_RTC_AlarmEnable / Disable

#### 4.5.1 函数原型及功能

启用或禁用闹钟中断。

```c
fsp_err_t R_RTC_AlarmEnable(rtc_ctrl_t * const p_ctrl);
fsp_err_t R_RTC_AlarmDisable(rtc_ctrl_t * const p_ctrl);
```

---

### 4.6 R_RTC_PeriodicIrqRateSet

#### 4.6.1 函数原型及功能

设置周期性中断频率。

```c
fsp_err_t R_RTC_PeriodicIrqRateSet(rtc_ctrl_t * const p_ctrl, rtc_periodic_irq_rate_t const rate)
```

#### 4.6.2 可选频率

- `RTC_PERIODIC_IRQ_RATE_1HZ`（每秒一次）
- `RTC_PERIODIC_IRQ_RATE_2HZ`, `4HZ`, ..., `64HZ`
- `RTC_PERIODIC_IRQ_RATE_OFF`

📌 **用途**：替代 SysTick 实现低功耗定时。

---

### 4.7 R_RTC_Close

#### 4.7.1 函数原型及功能

关闭 RTC 模块。

```c
fsp_err_t R_RTC_Close(rtc_ctrl_t * const p_ctrl)
```

---

## 5. RTC 中断回调函数

```c
void rtc_callback(rtc_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case RTC_EVENT_ALARM_IRQ:
            // 闹钟触发
            g_alarm_triggered = true;
            break;

        case RTC_EVENT_PERIODIC_IRQ:
            // 周期性中断（如1Hz）
            g_rtc_tick = true;
            break;

        case RTC_EVENT_CARRY_IRQ:
            // 秒计数溢出（CUP中断）
            break;

        default:
            break;
    }
}
```

---

## 6. 实战1：RTC 电子时钟（带串口输出）

```c
#include "hal_data.h"

void show_current_time(void)
{
    rtc_time_t now;
    R_RTC_CalendarTimeGet(&g_rtc_ctrl, &now);

    char msg[64];
    snprintf(msg, sizeof(msg), "Time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
             now.tm_year, now.tm_mon, now.tm_mday,
             now.tm_hour, now.tm_min, now.tm_sec);
    R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*)msg, strlen(msg));
}

void hal_entry(void)
{
    R_RTC_Open(&g_rtc_ctrl, &g_rtc_cfg);

    // 设置初始时间
    rtc_time_t init_time = {
        .tm_sec  = 0,
        .tm_min  = 0,
        .tm_hour = 12,
        .tm_mday = 1,
        .tm_mon  = 1,
        .tm_year = 2024,
        .tm_wday = 1,
    };
    R_RTC_CalendarTimeSet(&g_rtc_ctrl, &init_time, RTC_UNIT_SECONDS);

    while(1)
    {
        if (g_rtc_tick) // 1Hz 周期中断
        {
            g_rtc_tick = false;
            show_current_time();
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

---

## 7. 实战2：RTC 闹钟唤醒 Deep Standby

### 7.1 配置要点

- `Alarm Mode`: `Daily`
- `Alarm Time`: `07:00:00`
- `Interrupts`: ✅ Enable Alarm IRQ
- `Power Management`: 配置为 Deep Standby 模式

### 7.2 代码实现

```c
void enter_deep_standby(void)
{
    // 关闭所有外设时钟
    R_CGC_PeripheralClockDisable(&g_cgc_ctrl, CGC_PERIPHERAL_SCI0);
    R_CGC_PeripheralClockDisable(&g_cgc_ctrl, CGC_PERIPHERAL_GPT0);

    // 进入 Deep Standby，RTC 保持运行
    R_BSP_DeepSoftwareStandbyEnter();
    // MCU 休眠，直到 RTC 闹钟唤醒
}

void hal_entry(void)
{
    R_RTC_Open(&g_rtc_ctrl, &g_rtc_cfg);

    while(1)
    {
        if (g_alarm_triggered)
        {
            g_alarm_triggered = false;
            // 执行晨间任务：打开灯光、播放音乐等
            R_SCI_UART_Write(&g_uart0_ctrl, "Good Morning!\r\n", 15);
        }

        enter_deep_standby(); // 每次任务后重新进入休眠
    }
}
```

📌 **功耗**：Deep Standby 模式下电流 **< 1μA**，仅 RTC 和备份寄存器供电！

---

## 8. RTC 校准与精度优化

### 8.1 数字校准原理

通过调整内部计数器步进值，实现 ±90ppm 调整：

```c
校准值 > 0：加快时钟（正偏差）
校准值 < 0：减慢时钟（负偏差）
```

### 8.2 校准步骤

1. 运行 RTC 一周，对比标准时间
2. 计算日偏差（秒/天）
3. 计算校准值：

```c

```

> ✅ 在 FSP 中设置 `Calibration Value = 76`

---

## 9. RTC 函数速查表

| 函数名                        | 用途       | 关键参数              | 返回值/错误码        | 注意事项             |
| -------------------------- | -------- | ----------------- | -------------- | ---------------- |
| `R_RTC_Open`               | 初始化RTC   | `p_ctrl`, `p_cfg` | `ALREADY_OPEN` | 必须最先调用           |
| `R_RTC_CalendarTimeSet`    | 设置当前时间   | `*p_time`         | `INVALID_ARG`  | 年范围 2000~2099    |
| `R_RTC_CalendarTimeGet`    | 获取当前时间   | `*p_time`         | -              | 实时读取             |
| `R_RTC_AlarmSet`           | 设置闹钟     | `*p_alarm_time`   | `INVALID_ARG`  | 可设每日/特定时间        |
| `R_RTC_AlarmEnable`        | 启用闹钟中断   | -                 | -              | 可唤醒 Deep Standby |
| `R_RTC_PeriodicIrqRateSet` | 设置周期中断频率 | `rate` (1Hz~64Hz) | `INVALID_ARG`  | 替代 SysTick       |
| `rtc_callback`             | 事件回调     | `p_args->event`   | 无返回值           | 处理闹钟、周期中断        |

---

## 10. 开发建议

✅ **推荐做法**：

1. **必须使用外部 32.768kHz 晶振**，LOCO 精度太差。
2. **启用数字校准**，补偿晶振个体差异。
3. **闹钟唤醒 Deep Standby**，实现超低功耗定时任务。
4. **周期性中断替代 SysTick**，降低运行功耗。
5. **时间显示使用 `R_RTC_CalendarTimeGet`**，避免全局变量同步问题。

⛔ **避免做法**：

1. **频繁调用 `CalendarTimeSet`**（可能引起计时混乱）
2. **在中断中执行长时间操作**（阻塞唤醒）
3. **忽略闰年与月份天数**（RTC 自动处理，无需软件干预）

---


