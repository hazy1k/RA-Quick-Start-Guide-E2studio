# 第三章 CGC时钟控制介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **时钟生成电路（CGC, Clock Generation Circuit）架构、FSP 配置方法、核心函数使用、时钟树管理、动态调频技巧及低功耗时钟策略**，为高性能与低功耗系统设计打下坚实基础。

---

## 1. RA6E2 CGC 简介

RA6E2 的 **CGC（Clock Generation Circuit）** 是整个 MCU 的“心脏”，负责生成和分发系统所需的各种时钟源，包括：

- **主系统时钟（System Clock）**
- **外设模块时钟（Peripheral Clock）**
- **低速时钟（用于 RTC、低功耗模式）**
- **PLL、HOCO、MOCO、LOCO、外部晶振等时钟源**

RA6E2 支持多种时钟源灵活切换，实现 **高性能运行** 与 **超低功耗待机** 的平衡。

---

### 1.1 RA6E2 时钟源概览

| 时钟源          | 频率范围          | 特点                        | 典型用途              |
| ------------ | ------------- | ------------------------- | ----------------- |
| **HOCO**     | 16~20 MHz（可调） | 高速片上振荡器，启动快，精度中等          | 主系统时钟（默认）         |
| **MOCO**     | 8 MHz         | 中速片上振荡器，低功耗               | 低功耗模式、备份时钟        |
| **LOCO**     | 32.768 kHz    | 低速片上振荡器，极低功耗              | Deep Standby、WDT  |
| **Main OSC** | 8~20 MHz      | 外部高速晶振，精度高                | 高精度系统时钟           |
| **Sub OSC**  | 32.768 kHz    | 外部低速晶振                    | RTC、低功耗唤醒         |
| **PLL**      | 最高 240 MHz    | 倍频器，可将 HOCO 或 Main OSC 倍频 | 高性能模式（Cortex-M33） |

📌 **默认出厂配置**：  
RA6E2 上电后默认使用 **HOCO 20MHz** → 通过 **PLL 倍频至 200MHz** 作为系统主频（可在 FSP 中修改）。

---

### 1.2 时钟树结构简图（简化版）

```c
[LOCO 32k] ───┐
[MOCO 8M] ────┤
[HOCO 20M] ───┼──→ [PLL] ──→ [System Clock (up to 240MHz)]
[Main OSC] ───┘           │
                          ├──→ [Peripheral Clock A/B/C/D...]
[Sub OSC 32k] ────────────┴──→ [RTC, IWDT, AGT]
```

> 💡 所有外设时钟均可独立使能/关闭，实现精细化功耗控制。

---

## 2. FSP 图形化配置 CGC

### 2.1 创建工程后配置主时钟

1. 打开 `FSP Configuration` → `Clocks` 标签页。
2. 在 `Main` 选项卡中配置系统主时钟源：
   - **Clock Source**：选择 `HOCO`, `Main OSC`, 或 `PLL`
   - **PLL Input Source**：若选 PLL，可选 HOCO 或 Main OSC
   - **PLL Multiplier**：倍频系数（如 10x → 20MHz × 10 = 200MHz）
   - **System Clock Divider**：可分频（如 /1, /2, /4...）

📌 示例：配置为 **Main OSC 12MHz → PLL ×20 = 240MHz → 系统时钟**

---

### 2.2 配置外设时钟（Peripheral Clock）

在 `Peripheral Clocks` 标签页中：

- 可单独使能/关闭每个外设的时钟（如 SCI0, SPI0, GPT0...）
- 可选择外设时钟源（PCLKA, PCLKB, PCLKC, PCLKD）
- 可设置分频系数（/1, /2, /4, /8...）

> ✅ **强烈建议**：未使用的外设时钟 **务必关闭**，可显著降低功耗。

---

### 2.3 低速时钟配置（用于RTC、WDT）

在 `Subclock` 选项卡：

- 使能 `Sub Oscillator`（需外接 32.768kHz 晶振）
- 配置 `LOCO` 作为备用（精度较低，但无需外部元件）

---

## 3. CGC 相关 FSP 函数详解

> 📌 所有时钟操作函数均在 `r_cgc.h` 中声明，需包含头文件。

---

### 3.1 R_CGC_Open

#### 3.1.1 函数原型及功能

初始化 CGC 模块，应用 FSP 配置的时钟设置。

```c
fsp_err_t R_CGC_Open(cgc_ctrl_t * const p_ctrl, cgc_cfg_t const * const p_cfg)
```

#### 3.1.2 参数

- `cgc_ctrl_t * const p_ctrl`：  
  CGC 控制句柄，**必须由用户定义并传入地址**。  
  ⚠️ **若传入 NULL，返回 `FSP_ERR_ASSERTION`。**

- `cgc_cfg_t const * const p_cfg`：  
  指向时钟配置结构体的指针，通常由 FSP 自动生成（如 `g_cgc_cfg`）。  
  包含主时钟源、PLL 设置、外设时钟使能等。  
  ⚠️ **若传入 NULL，使用默认配置（不推荐）。**

#### 3.1.3 返回值

- `FSP_SUCCESS`：时钟配置成功并生效。
- `FSP_ERR_ASSERTION`：`p_ctrl` 或 `p_cfg` 为 NULL。
- `FSP_ERR_ALREADY_OPEN`：CGC 模块已初始化。
- `FSP_ERR_INVALID_ARGUMENT`：配置参数非法（如 PLL 超频）。

📌 **典型用法（FSP 自动生成于 `hal_entry` 前）：**

```c
fsp_err_t err = R_CGC_Open(&g_cgc_ctrl, &g_cgc_cfg);
if (FSP_SUCCESS != err) {
    while(1); // 时钟初始化失败，死循环
}
```

---

### 3.2 R_CGC_ClockStart

#### 3.2.1 函数原型及功能

启动指定的时钟源（如 Main OSC, PLL）。

```c
fsp_err_t R_CGC_ClockStart(cgc_ctrl_t * const p_ctrl, cgc_clock_t clock_source)
```

#### 3.2.2 参数

- `cgc_ctrl_t * const p_ctrl`：已初始化的 CGC 句柄。
- `cgc_clock_t clock_source`：要启动的时钟源，枚举值包括：
  - `CGC_CLOCK_HOCO`
  - `CGC_CLOCK_MOCO`
  - `CGC_CLOCK_LOCO`
  - `CGC_CLOCK_MAIN_OSC`
  - `CGC_CLOCK_SUBCLOCK`
  - `CGC_CLOCK_PLL`

#### 3.2.3 返回值

- `FSP_SUCCESS`：时钟启动成功。
- `FSP_ERR_NOT_OPEN`：CGC 未初始化。
- `FSP_ERR_ASSERTION`：`p_ctrl` 为 NULL。
- `FSP_ERR_INVALID_ARGUMENT`：无效时钟源。
- `FSP_ERR_IN_USE`：时钟已在运行。

📌 **用法示例（手动启动 Main OSC）：**

```c
err = R_CGC_ClockStart(&g_cgc_ctrl, CGC_CLOCK_MAIN_OSC);
if (FSP_SUCCESS != err) { /* 错误处理 */ }
```

> 💡 通常在 FSP 中配置后，`R_CGC_Open` 会自动启动所需时钟，无需手动调用。

---

### 3.3 R_CGC_ClockStop

#### 3.3.1 函数原型及功能

停止指定的时钟源（节省功耗）。

```c
fsp_err_t R_CGC_ClockStop(cgc_ctrl_t * const p_ctrl, cgc_clock_t clock_source)
```

#### 3.3.2 参数

同 `R_CGC_ClockStart`。

#### 3.3.3 返回值

- `FSP_SUCCESS`：时钟停止成功。
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`
- `FSP_ERR_INVALID_ARGUMENT`
- `FSP_ERR_NOT_ENABLED`：时钟未启用，无法停止。

📌 **注意**：  
⛔ **禁止停止当前系统时钟源**（如正在使用 PLL 时停止 PLL），会导致系统崩溃！

---

### 3.4 R_CGC_SystemClockSet

#### 3.4.1 函数原型及功能

动态切换系统主时钟源（运行时调频）。

```c
fsp_err_t R_CGC_SystemClockSet(cgc_ctrl_t * const p_ctrl, cgc_system_clock_cfg_t const * const p_clock_cfg)
```

#### 3.4.2 参数

- `cgc_ctrl_t * const p_ctrl`：CGC 句柄。
- `cgc_system_clock_cfg_t const * const p_clock_cfg`：新系统时钟配置，包含：
  - `clock_source`：新主时钟源（如 `CGC_CLOCK_PLL`）
  - `divider`：系统时钟分频器（`CGC_SYS_CLOCK_DIV_1`, `_2`, `_4`...）
  - `pll_cfg`：若切换到 PLL，需提供 PLL 配置（倍频系数等）

#### 3.4.3 返回值

- `FSP_SUCCESS`：切换成功。
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`
- `FSP_ERR_INVALID_ARGUMENT`
- `FSP_ERR_IN_USE`：目标时钟未准备好（如 PLL 未稳定）

📌 **典型应用场景**：

- 高性能模式 → 切换到 PLL 240MHz
- 低功耗模式 → 切换到 MOCO 8MHz
- 睡眠唤醒后恢复高速时钟

📌 **用法示例（切换到 MOCO 8MHz）：**

```c
cgc_system_clock_cfg_t sys_clk_cfg = {
    .clock_source = CGC_CLOCK_MOCO,
    .divider = CGC_SYS_CLOCK_DIV_1,
    .pll_cfg = NULL  // 不使用PLL
};

err = R_CGC_SystemClockSet(&g_cgc_ctrl, &sys_clk_cfg);
if (FSP_SUCCESS != err) { /* 处理错误 */ }

// 更新 SystemCoreClock 全局变量（用于延时函数等）
SystemCoreClock = 8000000; // 8MHz
```

> 📌 **重要**：切换时钟后，**务必手动更新 `SystemCoreClock` 变量**，否则 `R_BSP_SoftwareDelay` 等函数会计算错误！

---

### 3.5 R_CGC_OscillatorStatusGet

#### 3.5.1 函数原型及功能

获取指定振荡器的当前状态（是否稳定）。

```c
fsp_err_t R_CGC_OscillatorStatusGet(cgc_ctrl_t * const p_ctrl, cgc_clock_t clock_source, cgc_oscillator_status_t * p_status)
```

#### 3.5.2 参数

- `cgc_ctrl_t * const p_ctrl`：CGC 句柄。
- `cgc_clock_t clock_source`：要查询的时钟源。
- `cgc_oscillator_status_t * p_status`：返回状态：
  - `CGC_OSCILLATOR_STATE_NOT_STABILIZED`
  - `CGC_OSCILLATOR_STATE_STABILIZED`

#### 3.5.3 返回值

- `FSP_SUCCESS`
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **用法示例（等待 Main OSC 稳定）：**

```c
cgc_oscillator_status_t osc_status;
do {
    R_CGC_OscillatorStatusGet(&g_cgc_ctrl, CGC_CLOCK_MAIN_OSC, &osc_status);
} while (osc_status != CGC_OSCILLATOR_STATE_STABILIZED);
```

---

### 3.6 R_CGC_ClockFrequencyGet

#### 3.6.1 函数原型及功能

获取指定时钟源的当前频率（Hz）。

```c
fsp_err_t R_CGC_ClockFrequencyGet(cgc_ctrl_t * const p_ctrl, cgc_clock_t clock_source, uint32_t * p_frequency_hz)
```

#### 3.6.2 参数

- `cgc_ctrl_t * const p_ctrl`
- `cgc_clock_t clock_source`
- `uint32_t * p_frequency_hz`：返回频率值（单位 Hz）

#### 3.6.3 返回值

- `FSP_SUCCESS`
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **用法示例：**

```c
uint32_t sys_clk_hz;
R_CGC_ClockFrequencyGet(&g_cgc_ctrl, CGC_CLOCK_MAIN_OSC, &sys_clk_hz);
printf("Main OSC: %lu Hz\n", sys_clk_hz);
```

---

### 3.7 R_CGC_PeripheralClockEnable / Disable

#### 3.7.1 函数原型及功能

使能或关闭指定外设的时钟（动态功耗管理）。

```c
fsp_err_t R_CGC_PeripheralClockEnable(cgc_ctrl_t * const p_ctrl, cgc_peripheral_t peripheral, cgc_clock_t clock_source)
fsp_err_t R_CGC_PeripheralClockDisable(cgc_ctrl_t * const p_ctrl, cgc_peripheral_t peripheral)
```

#### 3.7.2 参数

- `cgc_peripheral_t peripheral`：外设枚举，如：
  - `CGC_PERIPHERAL_SCI0`
  - `CGC_PERIPHERAL_SPI0`
  - `CGC_PERIPHERAL_GPT0`
  - `CGC_PERIPHERAL_ADC0`
- `cgc_clock_t clock_source`（仅 Enable 需要）：为此外设选择时钟源（PCLKA/B/C/D）

#### 3.7.3 返回值

- `FSP_SUCCESS`
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`
- `FSP_ERR_INVALID_ARGUMENT`

📌 **用法示例（动态开关 ADC 时钟）：**

```c
/ 使用前开启时钟
R_CGC_PeripheralClockEnable(&g_cgc_ctrl, CGC_PERIPHERAL_ADC0, CGC_CLOCK_PCLKA);

// ... 进行ADC采样 ...

// 使用后关闭时钟省电
R_CGC_PeripheralClockDisable(&g_cgc_ctrl, CGC_PERIPHERAL_ADC0);
```

---

## 4. CGC 函数速查表

| 函数名                            | 用途      | 关键参数                         | 返回值/错误码                       | 注意事项                     |
| ------------------------------ | ------- | ---------------------------- | ----------------------------- | ------------------------ |
| `R_CGC_Open`                   | 初始化CGC  | `p_ctrl`, `p_cfg`            | `ALREADY_OPEN`, `INVALID_ARG` | 必须最先调用                   |
| `R_CGC_ClockStart`             | 启动时钟源   | `clock_source`               | `IN_USE`                      | Main OSC/PLL 需等待稳定       |
| `R_CGC_ClockStop`              | 停止时钟源   | `clock_source`               | `NOT_ENABLED`                 | 禁止停止当前系统时钟               |
| `R_CGC_SystemClockSet`         | 切换系统主时钟 | `p_clock_cfg`                | `IN_USE`                      | 切换后需更新 `SystemCoreClock` |
| `R_CGC_OscillatorStatusGet`    | 查询振荡器状态 | `clock_source`, `*p_status`  | -                             | 用于等待时钟稳定                 |
| `R_CGC_ClockFrequencyGet`      | 获取时钟频率  | `clock_source`, `*p_freq`    | -                             | 调试/动态配置用                 |
| `R_CGC_PeripheralClockEnable`  | 使能外设时钟  | `peripheral`, `clock_source` | `INVALID_ARG`                 | 使用前必须使能                  |
| `R_CGC_PeripheralClockDisable` | 关闭外设时钟  | `peripheral`                 | -                             | 用完关闭，省电！                 |

---

## 5. 实战：动态调频 + 低功耗演示

```c

```

---

## 6. 开发建议与注意事项

✅ **必做事项**：

1. **所有时钟操作前确保 `R_CGC_Open` 已调用**。
2. **切换系统时钟后，立即更新 `SystemCoreClock` 全局变量**。
3. **启动 Main OSC / PLL 后，务必等待其稳定再切换为主时钟**。
4. **未使用的外设，务必调用 `R_CGC_PeripheralClockDisable` 关闭时钟**。
5. **进入低功耗模式前，切换到低速时钟源（如 MOCO/LOCO）**。

⛔ **禁止事项**：

1. **禁止在未等待稳定时切换到新时钟源**。
2. **禁止停止当前正在使用的系统时钟源**。
3. **禁止在中断服务函数中执行长时间时钟切换操作**。
