# 第二章 GPIO介绍及使用

## 1. RA6E2 GPIO简介

RA6E2 是瑞萨电子（Renesas）推出的高性能32位MCU，属于RA6系列，基于Arm® Cortex®-M33内核。其GPIO系统灵活、功能丰富，支持多种工作模式、中断触发、复用功能切换，并可通过FSP（Flexible Software Package）图形化配置工具快速初始化。

RA6E2芯片引脚数量依封装不同而异，例如LQFP144封装拥有最多118个GPIO引脚，分布在多个端口组（Port A ~ Port J等），每个端口最多16个引脚（P0~P15）。

与STM32的“组”概念类似，RA6E2使用“端口（Port）”管理引脚，如P0A0表示Port A的第0脚，P1B5表示Port B的第5脚。

---

### 1.1 RA6E2 GPIO的八种工作模式（FSP配置视角）

在FSP中，GPIO模式主要通过 **Pin Configuration** 工具设置，底层由寄存器 `PmnPFS`（Port n Pin m Pin Function Select）控制。以下是常见的八种工作模式：

#### 1.1.1 输入模式（无上下拉） - 类似“浮空输入”

- **FSP配置项**：
  - Mode: `Input`
  - Pull-up/Pull-down: `None`
- **寄存器对应**：
  - PMR = 0（设为GPIO模式）
  - PDR = 0（输入）
  - PCR = 0（无上下拉）  
    🔹 **特点**：
- 引脚电平完全由外部电路决定。
- 功耗最低，但易受干扰。  
  🔹 **应用场景**：
- 外部数字信号源（如传感器、其他MCU输出）。
- 高速信号采集（如编码器脉冲）。  
  ⚠️ **注意**：悬空时电平不确定，建议外接上下拉电阻。

---

#### 1.1.2 上拉输入

- **FSP配置项**：
  - Mode: `Input`
  - Pull-up/Pull-down: `Pull-up`
- **寄存器对应**：
  - PMR = 0
  - PDR = 0
  - PCR = 1（启用上拉）  
    🔹 **特点**：
- 内部弱上拉（约几十kΩ），默认高电平。
- 按键按下接地时读取低电平。  
  🔹 **应用场景**：
- 按键输入（低有效）。
- 总线空闲保持高电平（如UART RX默认高）。  
  ✅ 无需外部上拉，简化设计。

---

#### 1.1.3 下拉输入

- **FSP配置项**：
  - Mode: `Input`
  - Pull-up/Pull-down: `Pull-down`
- **寄存器对应**：
  - PMR = 0
  - PDR = 0
  - PCR = 2（启用下拉）  
    🔹 **特点**：
- 默认低电平，外部拉高时读高。  
  🔹 **应用场景**：
- 按键接VCC（高有效）。
- 使能信号检测（默认无效为低）。

---

#### 1.1.4 模拟输入

- **FSP配置项**：
  - Mode: `Analog`
- **寄存器对应**：
  - PMR = 0
  - ASEL = 1（模拟输入使能）  
    🔹 **特点**：
- 关闭数字缓冲器，直接连接ADC。
- 高阻抗，低功耗。  
  🔹 **应用场景**：
- ADC采集（温度、电压、电位器等）。
- 低功耗模式引脚配置。  
  ✅ **重要**：ADC通道必须配置为模拟模式，否则影响精度或损坏ADC。

---

#### 1.1.5 推挽输出

- **FSP配置项**：
  - Mode: `Output`
  - Output Type: `Push-pull`
- **寄存器对应**：
  - PMR = 0
  - PDR = 1（输出）
  - PODR = 0 或 1（控制电平）  
    🔹 **特点**：
- 可输出高/低电平，驱动能力强。
- 不需外部上拉。  
  🔹 **应用场景**：
- LED控制（加限流电阻）。
- 继电器、MOSFET驱动。
- 数字使能信号输出。

---

#### 1.1.6 开漏输出

- **FSP配置项**：
  - Mode: `Output`
  - Output Type: `Open-drain`
- **寄存器对应**：
  - PMR = 0
  - PDR = 1
  - PODR = 0 → 引脚拉低；PODR = 1 → 引脚高阻  
    🔹 **特点**：
- 输出高电平时为高阻态，需外接上拉。
- 支持“线与”逻辑。  
  🔹 **应用场景**：
- I2C总线（必须开漏）。
- 多设备共享信号线。
- 电平转换（如3.3V ↔ 5V）。  
  ✅ **必须外接上拉电阻**（如4.7kΩ）。

---

#### 1.1.7 复用功能推挽输出（Alternate Function Push-Pull）

- **FSP配置项**：
  - Mode: `Peripheral Function`
  - Peripheral: 选择外设（如SCI、SPI、PWM）
  - Output Type: `Push-pull`（部分外设支持）
- **寄存器对应**：
  - PMR = 1（设为外设模式）
  - 由外设控制输出电平  
    🔹 **特点**：
- 由外设（如UART、SPI、PWM）控制引脚。
- 推挽输出，驱动能力强。  
  🔹 **应用场景**：
- SCI Tx（串口发送）。
- SPI SCK/MOSI。
- GPT PWM输出。

---

#### 1.1.8 复用功能开漏输出（Alternate Function Open-Drain）

- **FSP配置项**：
  - Mode: `Peripheral Function`
  - Peripheral: 选择外设
  - Output Type: `Open-drain`（如I2C）
- **寄存器对应**：
  - PMR = 1
  - 外设控制，开漏输出  
    🔹 **特点**：
- 外设控制 + 开漏输出。
- 需外接上拉。  
  🔹 **应用场景**：
- I2C SDA/SCL。
- 共享中断线。
- 电平兼容接口。

---

### 1.2 模式对比表（RA6E2 FSP视角）

| 模式   | FSP Mode                | 输入/输出 | 上下拉  | 输出类型 | 典型用途           |
| ---- | ----------------------- | ----- | ---- | ---- | -------------- |
| 浮空输入 | Input → None            | 输入    | 无    | -    | 传感器、高速信号       |
| 上拉输入 | Input → Pull-up         | 输入    | 上拉   | -    | 按键（低有效）        |
| 下拉输入 | Input → Pull-down       | 输入    | 下拉   | -    | 按键（高有效）        |
| 模拟输入 | Analog                  | 输入    | 无    | -    | ADC采集、低功耗      |
| 推挽输出 | Output → Push-pull      | 输出    | 可选   | 推挽   | LED、继电器、数字输出   |
| 开漏输出 | Output → Open-drain     | 输出    | 外加上拉 | 开漏   | I2C、线与、电平转换    |
| 复用推挽 | Peripheral → Push-pull  | 外设控制  | 可选   | 推挽   | SCI Tx、SPI、PWM |
| 复用开漏 | Peripheral → Open-drain | 外设控制  | 外加上拉 | 开漏   | I2C、共享总线       |

---

## 2. GPIO使用示例 - 基于e2 studio + FSP

我们以RA6E2开发板为例，配置LED、Beep、按键，并编写测试代码。

> 📌 温馨提示：FSP图形化配置是RA系列开发的核心优势，建议配合e2 studio使用。

---

### 2.1 FSP图形化配置步骤

#### 2.1.1 创建工程 & 时钟配置

- 打开 e2 studio → New RA Project → 选择 RA6E2 → LQFP144。
- 在 `FSP Configuration` 中配置主时钟（如HOCO 20MHz → PLL → 200MHz系统时钟）。

#### 2.1.2 LED GPIO配置（推挽输出）

假设LED连接在 **P405**（低电平点亮）：

- 打开 `Pins` 标签页 → 找到 `P405`。
- 配置：
  - Mode: `Output`
  - Output Type: `Push-pull`
  - Initial Output: `High`（默认熄灭）
  - Label: `LED_RED`

#### 2.1.3 Beep GPIO配置（推挽输出）

假设Beep连接在 **P504**（高电平鸣响）：

- 配置 P504：
  - Mode: `Output`
  - Output Type: `Push-pull`
  - Initial Output: `Low`
  - Label: `BEEP`

#### 2.1.4 按键 GPIO配置（上拉输入）

假设三个按键：

- WK_UP → P100（按下为高）→ 配置为 `Input + Pull-up`
- KEY0 → P101（按下为低）→ 配置为 `Input + Pull-up`
- KEY1 → P102（按下为低）→ 配置为 `Input + Pull-up`

> 📌 注意：RA6E2内部上拉足够强，一般无需外接电阻。

---

### 2.2 用户代码

#### 2.2.1 LED控制宏（使用FSP生成的引脚别名）

FSP会自动生成引脚别名，如：

```c
// 在hal_data.h中自动生成
#define LED_RED_PORT    (R_PORT4)
#define LED_RED_PIN     (0x0020U)  // P405 = BIT5

#define BEEP_PORT       (R_PORT5)
#define BEEP_PIN        (0x0010U)  // P504 = BIT4
```

我们可封装控制宏：

```c
// LED低电平点亮
#define LED_ON(port, pin)      R_PORT->PODR[port] &= ~(pin)
#define LED_OFF(port, pin)     R_PORT->PODR[port] |= (pin)
#define LED_TOGGLE(port, pin)  R_PORT->PODR[port] ^= (pin)

// 或者使用FSP推荐的R_IOPORT API（更安全）
#define LED_ON_FSP(pin)        R_IOPORT_PinWrite(&g_ioport_ctrl, pin, BSP_IO_LEVEL_LOW)
#define LED_OFF_FSP(pin)       R_IOPORT_PinWrite(&g_ioport_ctrl, pin, BSP_IO_LEVEL_HIGH)
#define LED_TOGGLE_FSP(pin)    do { \
    bsp_io_level_t level; \
    R_IOPORT_PinRead(&g_ioport_ctrl, pin, &level); \
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin, (level == BSP_IO_LEVEL_LOW) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW); \
} while(0)
```

> ✅ 推荐使用 `R_IOPORT_PinWrite/Read`，线程安全且支持运行时重配置。

---

#### 2.2.2 按键扫描函数

```c
typedef enum {
    KEY_NONE = 0,
    KEY_WK_UP,
    KEY0,
    KEY1
} KEY_TypeDef;

// FSP自动生成引脚别名（在hal_data.h）
// #define WK_UP_PIN    (IOPORT_PORT_01_PIN_00)
// #define KEY0_PIN     (IOPORT_PORT_01_PIN_01)
// #define KEY1_PIN     (IOPORT_PORT_01_PIN_02)

KEY_TypeDef Key_Scan(void)
{
    static uint8_t key_up_wkup = 1;
    static uint8_t key_up_key0 = 1;
    static uint8_t key_up_key1 = 1;

    bsp_io_level_t level;

    // WK_UP: 按下为高
    R_IOPORT_PinRead(&g_ioport_ctrl, WK_UP_PIN, &level);
    if (level == BSP_IO_LEVEL_HIGH)
    {
        if (key_up_wkup)
        {
            key_up_wkup = 0;
            return KEY_WK_UP;
        }
    }
    else key_up_wkup = 1;

    // KEY0: 按下为低
    R_IOPORT_PinRead(&g_ioport_ctrl, KEY0_PIN, &level);
    if (level == BSP_IO_LEVEL_LOW)
    {
        if (key_up_key0)
        {
            key_up_key0 = 0;
            return KEY0;
        }
    }
    else key_up_key0 = 1;

    // KEY1: 按下为低
    R_IOPORT_PinRead(&g_ioport_ctrl, KEY1_PIN, &level);
    if (level == BSP_IO_LEVEL_LOW)
    {
        if (key_up_key1)
        {
            key_up_key1 = 0;
            return KEY1;
        }
    }
    else key_up_key1 = 1;

    return KEY_NONE;
}
```

---

#### 2.2.3 主函数测试

```c
#include "hal_data.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

volatile bool g_main_loop = true;

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* 初始化IO（FSP已自动生成在hal_entry之前）*/
    fsp_err_t err = R_IOPORT_Open(&g_ioport_ctrl, &g_ioport_cfg);
    if (FSP_SUCCESS != err) while(1);

    bool beep_status = false;

    while (g_main_loop)
    {
        KEY_TypeDef key = Key_Scan();
        switch(key)
        {
            case KEY_WK_UP:
                R_IOPORT_PinWrite(&g_ioport_ctrl, BEEP_PIN, BSP_IO_LEVEL_HIGH);
                beep_status = true;
                break;
            case KEY0:
                R_IOPORT_PinWrite(&g_ioport_ctrl, BEEP_PIN, BSP_IO_LEVEL_LOW);
                beep_status = false;
                break;
            case KEY1:
                beep_status = !beep_status;
                R_IOPORT_PinWrite(&g_ioport_ctrl, BEEP_PIN, beep_status ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
                break;
            default:
                break;
        }

        /* 流水灯效果 */
        LED_OFF_FSP(LED_RED_PIN);
        LED_ON_FSP(LED_GREEN_PIN);  // 假设已配置LED_GREEN
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

        LED_OFF_FSP(LED_GREEN_PIN);
        LED_ON_FSP(LED_BLUE_PIN);   // 假设已配置LED_BLUE
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

        LED_OFF_FSP(LED_BLUE_PIN);
        LED_ON_FSP(LED_RED_PIN);
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
```

> 📌 `R_BSP_SoftwareDelay` 是FSP提供的阻塞延时函数，单位可选毫秒、微秒等。

---

## 3. RA6E2 GPIO相关函数总结（基于FSP库）

> 📌 本章目标：深入掌握 RA6E2 FSP 库中 GPIO 模块所有常用函数的 **参数含义、返回值、错误码、典型用法、注意事项**，为后续外设开发打下坚实基础。

### 3.1 R_IOPORT_Open

#### 3.1.1 函数原型及功能

初始化 IO 端口模块，加载引脚配置并使能对应端口时钟。

```c
fsp_err_t R_IOPORT_Open(ioport_ctrl_t *const p_ctrl, const ioport_cfg_t * p_cfg)
```

#### 3.1.2 参数

- `ioport_ctrl_t *const p_ctrl`：  
  IO 端口模块的控制句柄，**必须由用户定义并传入地址**。该结构体用于保存驱动运行时状态。  
  ⚠️ **若传入 NULL，函数将返回 `FSP_ERR_ASSERTION`。**

- `const ioport_cfg_t * p_cfg`：  
  指向 IO 端口配置结构体的指针，通常由 FSP 图形化工具自动生成（如 `g_ioport_cfg`）。  
  结构体包含：
  
  - `p_pin_cfg_data`：指向引脚配置数组（每个元素对应一个引脚的初始模式、上下拉、输出电平等）
  - `number_of_pins`：配置的引脚总数  
    ⚠️ **若传入 NULL，函数将使用默认配置（不推荐，可能导致引脚状态异常）**

#### 3.1.3 返回

- `FSP_SUCCESS`：初始化成功，所有引脚已按配置生效。
- `FSP_ERR_ASSERTION`：`p_ctrl` 为 NULL。
- `FSP_ERR_ALREADY_OPEN`：模块已被打开，重复调用会触发此错误（需先调用 `R_IOPORT_Close`）。
- `FSP_ERR_INVALID_ARGUMENT`：配置数据中存在无效引脚或模式（极少见，通常 FSP 生成代码已校验）

📌 **典型用法（FSP 自动生成）：**

```c
fsp_err_t err = R_IOPORT_Open(&g_ioport_ctrl, &g_ioport_cfg);
if (FSP_SUCCESS != err) {
    while(1); // 错误处理
}
```

---

### 3.2 R_IOPORT_Close

#### 3.2.1 函数原型及功能

关闭 IO 端口模块，释放资源（一般在低功耗或模块卸载时调用）。

```c
fsp_err_t R_IOPORT_Close(ioport_ctrl_t *const p_ctrl)
```

#### 3.2.2 参数

- `ioport_ctrl_t *const p_ctrl`：  
  已打开的 IO 端口控制句柄。  
  ⚠️ **若传入 NULL，返回 `FSP_ERR_ASSERTION`。**

#### 3.2.3 返回

- `FSP_SUCCESS`：关闭成功。
- `FSP_ERR_ASSERTION`：`p_ctrl` 为 NULL。
- `FSP_ERR_NOT_OPEN`：模块未打开，无法关闭。

📌 **注意**：在 RA6E2 开发中，除非进入深度休眠或动态重配置，否则一般不调用此函数。

---

### 3.3 R_IOPORT_PinWrite

#### 3.3.1 函数原型及功能

设置指定引脚的输出电平为高或低。

```c
fsp_err_t R_IOPORT_PinWrite(ioport_ctrl_t *const p_ctrl, bsp_io_port_pin_t pin, bsp_io_level_t level)
```

#### 3.3.2 参数

- `ioport_ctrl_t *const p_ctrl`：  
  已通过 `R_IOPORT_Open` 初始化的控制句柄。  
  ⚠️ **若传入 NULL，返回 `FSP_ERR_ASSERTION`。**

- `bsp_io_port_pin_t pin`：  
  要操作的目标引脚，使用 FSP 生成的枚举值，如：
  
  - `BSP_IO_PORT_04_PIN_05` → 表示 P405
  - `BSP_IO_PORT_01_PIN_00` → 表示 P100  
    ⚠️ **若传入无效引脚（超出范围或未配置为输出），返回 `FSP_ERR_INVALID_ARGUMENT`。**

- `bsp_io_level_t level`：  
  要设置的电平值：
  
  - `BSP_IO_LEVEL_LOW` → 输出低电平（0V）
  - `BSP_IO_LEVEL_HIGH` → 输出高电平（VCC）  
    ⚠️ **若传入其他值（如 2, 3），返回 `FSP_ERR_INVALID_ARGUMENT`。**

#### 3.3.3 返回

- `FSP_SUCCESS`：设置成功，引脚电平已更新。
- `FSP_ERR_INVALID_ARGUMENT`：`pin` 或 `level` 参数无效。
- `FSP_ERR_NOT_OPEN`：IO 端口模块未初始化。
- `FSP_ERR_ASSERTION`：`p_ctrl` 为 NULL。

📌 **典型用法：**

```c
// 点亮 LED（假设低电平有效）
R_IOPORT_PinWrite(&g_ioport_ctrl, LED_RED_PIN, BSP_IO_LEVEL_LOW);

// 关闭蜂鸣器
R_IOPORT_PinWrite(&g_ioport_ctrl, BEEP_PIN, BSP_IO_LEVEL_LOW);
```

---

### 3.4 R_IOPORT_PinRead

#### 3.4.1 函数原型及功能

读取指定引脚的当前输入电平状态。

```c
fsp_err_t R_IOPORT_PinRead(ioport_ctrl_t *const p_ctrl, bsp_io_port_pin_t pin, bsp_io_level_t * p_pin_value)
```

#### 3.4.2 参数

- `ioport_ctrl_t *const p_ctrl`：  
  已初始化的控制句柄。  
  ⚠️ **若传入 NULL，返回 `FSP_ERR_ASSERTION`。**

- `bsp_io_port_pin_t pin`：  
  要读取的目标引脚（如 `BSP_IO_PORT_01_PIN_01`）。  
  ⚠️ **若引脚无效或未配置为输入，返回 `FSP_ERR_INVALID_ARGUMENT`。**

- `bsp_io_level_t * p_pin_value`：  
  指向存储读取结果的变量指针。函数执行后，该变量将被赋值为：
  
  - `BSP_IO_LEVEL_LOW`
  - `BSP_IO_LEVEL_HIGH`  
    ⚠️ **若传入 NULL，返回 `FSP_ERR_ASSERTION`。**

#### 3.4.3 返回

- `FSP_SUCCESS`：读取成功，`*p_pin_value` 已更新。
- `FSP_ERR_INVALID_ARGUMENT`：`pin` 无效。
- `FSP_ERR_NOT_OPEN`：模块未打开。
- `FSP_ERR_ASSERTION`：`p_ctrl` 或 `p_pin_value` 为 NULL。

📌 **典型用法（按键检测）：**

```c
bsp_io_level_t key_state;
R_IOPORT_PinRead(&g_ioport_ctrl, KEY0_PIN, &key_state);
if (key_state == BSP_IO_LEVEL_LOW) {
    // 按键按下
}
```

---

### 3.5 R_IOPORT_PortWrite

#### 3.5.1 函数原型及功能

批量设置指定端口的部分或全部引脚输出电平（按位掩码控制）。

```c
fsp_err_t R_IOPORT_PortWrite(ioport_ctrl_t *const p_ctrl, ioport_port_t port, uint32_t value, uint32_t mask)
```

#### 3.5.2 参数

- `ioport_ctrl_t *const p_ctrl`：控制句柄。
- `ioport_port_t port`：目标端口号，如 `IOPORT_PORT_04`（表示 Port 4）。
- `uint32_t value`：要写入的电平值（bit0~~bit15 对应 P0~~P15）。
- `uint32_t mask`：写入掩码，只有 mask 中为 1 的位才会被更新。  
  例如：`mask = 0x00FF` 表示只更新 P0~P7。

#### 3.5.3 返回

- `FSP_SUCCESS`：写入成功。
- `FSP_ERR_INVALID_ARGUMENT`：`port` 无效或超出范围。
- `FSP_ERR_NOT_OPEN`：模块未打开。
- `FSP_ERR_ASSERTION`：`p_ctrl` 为 NULL。

📌 **典型用法（8位并行输出）：**

```c
// 设置 P40~P47 输出 0xAA（10101010）
R_IOPORT_PortWrite(&g_ioport_ctrl, IOPORT_PORT_04, 0xAA, 0xFF);
```

---

### 3.6 R_IOPORT_PortRead

#### 3.6.1 函数功能及原型

读取指定端口的所有引脚输入状态（16位）。

```c
fsp_err_t R_IOPORT_PortRead(ioport_ctrl_t *const p_ctrl, ioport_port_t port, uint32_t * p_value)
```

#### 3.6.2 参数

- `ioport_ctrl_t *const p_ctrl`：控制句柄。
- `ioport_port_t port`：目标端口（如 `IOPORT_PORT_01`）。
- `uint32_t * p_value`：指向存储读取结果的变量（bit0~~bit15 对应 P0~~P15）。

#### 3.6.3 返回

- `FSP_SUCCESS`：读取成功。
- `FSP_ERR_INVALID_ARGUMENT`：`port` 无效。
- `FSP_ERR_NOT_OPEN`：模块未打开。
- `FSP_ERR_ASSERTION`：`p_ctrl` 或 `p_value` 为 NULL。

📌 **典型用法（读取8位拨码开关）：**

```c
uint32_t switch_val;
R_IOPORT_PortRead(&g_ioport_ctrl, IOPORT_PORT_02, &switch_val);
uint8_t sw8 = (uint8_t)(switch_val & 0xFF); // 取低8位
```

---

### 3.7 R_BSP_SoftwareDelay

#### 3.7.1 函数原型及功能

软件延时函数，基于系统时钟循环计数实现。

```c
void R_BSP_SoftwareDelay(uint32_t delay, bsp_delay_units_t units)
```

#### 3.7.2 参数：

- `uint32_t delay`：延时数量（如 100）。
- `bsp_delay_units_t units`：延时单位：
  - `BSP_DELAY_UNITS_SECONDS`
  - `BSP_DELAY_UNITS_MILLISECONDS`
  - `BSP_DELAY_UNITS_MICROSECONDS`

📌 **实现原理：**  
函数根据 `SystemCoreClock`（通过 `bsp_cpu_clock_get()` 获取）和预设循环周期 `BSP_DELAY_LOOP_CYCLES`（通常为 3~6 cycles/loop）计算所需循环次数，调用底层汇编循环函数 `bsp_prv_software_delay_loop()` 实现延时。

⚠️ **注意：**

- 是**阻塞式延时**，期间 CPU 无法执行其他任务。
- 精度受编译器优化、中断、Cache 等影响，**仅适用于对时间精度要求不高的场景**（如 LED 闪烁、按键消抖）。
- 不建议在中断服务函数中使用长延时。

📌 **典型用法：**

```c
R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // 延时100ms
R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);  // 延时10us（近似）
```

---

### 3.8 R_IOPORT_PinToggle（推荐封装函数）

#### 3.8.1 函数原型及功能

FSP 库未直接提供 Toggle 函数，但可通过 Read + Write 封装实现。

```c
fsp_err_t R_IOPORT_PinToggle(ioport_ctrl_t *const p_ctrl, bsp_io_port_pin_t pin)
{
    fsp_err_t err;
    bsp_io_level_t current_level;

    // 读取当前电平
    err = R_IOPORT_PinRead(p_ctrl, pin, &current_level);
    if (FSP_SUCCESS != err) {
        return err;
    }

    // 写入相反电平
    bsp_io_level_t new_level = (current_level == BSP_IO_LEVEL_LOW) ? 
                               BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW;
    return R_IOPORT_PinWrite(p_ctrl, pin, new_level);
}
```

📌 **用法示例：**

```c
R_IOPORT_PinToggle(&g_ioport_ctrl, LED_RED_PIN); // LED 翻转
```

---

### 3.9 外部中断回调函数（FSP 自动生成框架）

当在 FSP Pin Configurator 中配置引脚为 “Interrupt” 模式时，会自动生成如下回调函数：

```c
void external_irq_callback(external_irq_callback_args_t *p_args)
{
    // p_args->channel: 触发中断的 IRQ 通道号
    // p_args->pin:     触发中断的引脚（bsp_io_port_pin_t）
    // p_args->event:   触发事件（上升沿/下降沿等）

    if (p_args->pin == WK_UP_PIN) {
        LED_TOGGLE_FSP(LED_RED_PIN);
    }
}
```

📌 **注意：**

- 此函数在中断上下文中执行，应**尽量简短**，避免调用阻塞函数（如 `R_BSP_SoftwareDelay`）。
- 如需复杂处理，建议设置标志位，在主循环中处理。

---

## 4. 总结：RA6E2 GPIO FSP 函数速查表

| 函数名                     | 用途       | 关键参数                       | 返回值/错误码                        | 注意事项         |
| ----------------------- | -------- | -------------------------- | ------------------------------ | ------------ |
| `R_IOPORT_Open`         | 初始化GPIO  | `p_ctrl`, `p_cfg`          | `FSP_SUCCESS`, `ALREADY_OPEN`  | 必须最先调用       |
| `R_IOPORT_PinWrite`     | 单引脚输出    | `pin`, `level`             | `INVALID_ARGUMENT`, `NOT_OPEN` | 确保引脚配置为输出    |
| `R_IOPORT_PinRead`      | 单引脚输入    | `pin`, `*p_value`          | 同上                             | 确保引脚配置为输入    |
| `R_IOPORT_PortWrite`    | 端口批量输出   | `port`, `value`, `mask`    | `INVALID_ARGUMENT`             | mask 控制哪些位生效 |
| `R_IOPORT_PortRead`     | 端口批量输入   | `port`, `*p_value`         | 同上                             | 读取16位值       |
| `R_BSP_SoftwareDelay`   | 软件延时     | `delay`, `units`           | 无返回值                           | 阻塞式，精度不高     |
| `R_IOPORT_PinToggle`    | 引脚翻转（封装） | `pin`                      | 同 Write/Read                   | 非官方函数，需自行实现  |
| `external_irq_callback` | 外部中断回调   | `p_args->pin`, `->channel` | 无返回值                           | 中断上下文，避免复杂操作 |

---

📌 **开发建议：**

1. **始终使用 FSP 图形化配置引脚**，避免手动查寄存器。
2. **所有 IO 操作前确保 `R_IOPORT_Open` 已成功调用**。
3. **未使用引脚配置为 `Analog` 或 `Input + Pull-up/down` 以降低功耗**。
4. **I2C、1-Wire 等总线必须配置为 `Open-drain` + 外部上拉电阻**。
5. **延时函数仅用于调试或非实时场景，实时控制请使用定时器**。

## 5. STM32 HAL vs RA6E2 FSP 对比速查

| 功能      | STM32 HAL                  | RA6E2 FSP                                 |
| ------- | -------------------------- | ----------------------------------------- |
| 初始化     | `HAL_GPIO_Init()` + 时钟使能   | `R_IOPORT_Open()`（FSP自动生成）                |
| 写引脚     | `HAL_GPIO_WritePin()`      | `R_IOPORT_PinWrite()`                     |
| 读引脚     | `HAL_GPIO_ReadPin()`       | `R_IOPORT_PinRead()`                      |
| 翻转引脚    | `HAL_GPIO_TogglePin()`     | 需封装或调用两次Write                             |
| 中断回调    | `HAL_GPIO_EXTI_Callback()` | `user_irq_callback()`（FSP生成）              |
| 延时      | `HAL_Delay()`              | `R_BSP_SoftwareDelay()`                   |
| 图形化配置   | STM32CubeMX                | e2 studio FSP Pin Configurator            |
| 未使用引脚建议 | `GPIO_MODE_ANALOG`         | `Analog` 或 `Input + Pull`                 |
| I2C引脚要求 | `GPIO_MODE_AF_OD` + 外部上拉   | `Peripheral Function + Open-drain` + 外部上拉 |

---

✅ **RA6E2 GPIO开发要点总结**：

1. 优先使用FSP图形化配置，自动生成初始化代码和引脚别名。
2. 使用 `R_IOPORT_PinWrite/Read` 进行安全IO操作。
3. 中断通过FSP配置，自动生成回调框架。
4. 未使用引脚配置为模拟输入或带上拉/下拉输入，降低功耗。
5. I2C等开漏总线必须配置为开漏模式 + 外部上拉电阻。

---


