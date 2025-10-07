# 第四章 RA6E2 SCI 串口通信介绍及使用

> 📌 **本章目标**：深入掌握 RA6E2 的 **SCI（Serial Communication Interface）模块架构、FSP图形化配置、中断/DMA收发、多串口管理、波特率计算、错误处理机制**，实现稳定高效的串口通信系统。

---

## 1. RA6E2 SCI 模块简介

RA6E2 集成多个 **SCI（Serial Communication Interface）** 通道（如 SCI0~SCI9），每个通道支持：

- **异步模式（UART）**：最常用，支持 7/8/9 数据位、1/2 停止位、奇偶校验
- **时钟同步模式（SPI-like）**
- **智能卡模式**
- **简单 I2C 模式**

📌 **本章重点讲解异步 UART 模式**，因其在调试、通信、Bootloader 中应用最广。

---

### 1.1 SCI 通道与引脚复用

RA6E2 的 SCI 引脚可通过 **FSP Pin Configurator** 灵活映射到不同物理引脚。

例如 SCI0：

- **TXD0**：可映射到 P107、P300、P504 等
- **RXD0**：可映射到 P106、P301、P505 等

> ✅ 优势：PCB 布线灵活，无需拘泥于默认引脚。

---

### 1.2 SCI 功能特性

| 特性     | 说明                                                |
| ------ | ------------------------------------------------- |
| 波特率    | 支持 300 bps ~ 20 Mbps（取决于外设时钟）                     |
| 数据位    | 7, 8, 9 位                                         |
| 停止位    | 1, 2 位                                            |
| 校验位    | 无、奇校验、偶校验                                         |
| FIFO   | 部分通道支持发送/接收 FIFO（如 SCI9）                          |
| 中断     | 支持接收完成、发送完成、错误中断                                  |
| DMA    | 支持 DTC（Data Transfer Controller）实现无 CPU 负担的大数据量传输 |
| 硬件流控   | RTS/CTS（部分通道支持）                                   |
| 多处理器通信 | 支持地址帧检测（9位模式）                                     |

---

## 2. FSP 图形化配置 SCI

### 2.1 添加 SCI 模块

1. 打开 `FSP Configuration` → `Threads` → `hal_entry` → 点击 `+` 添加新模块。
2. 选择 `Driver` → `Connectivity` → `SCI UART`。
3. 选择通道（如 `g_uart0` 对应 SCI0）。

---

### 2.2 配置基本参数

在 `Properties` 面板中配置：

- **Channel**：选择 SCI 通道（如 0）
- **Baud Rate**：波特率（如 115200）
- **Data Bits**：8
- **Parity**：None
- **Stop Bits**：1
- **Flow Control**：None（若需硬件流控，选 RTS/CTS 并配置对应引脚）

---

### 2.3 配置引脚映射

切换到 `Pins` 标签页：

- 找到 `SCI0 TXD` → 分配到物理引脚（如 P504）→ Mode: `Peripheral` → Peripheral: `SCI0 TXD`
- 找到 `SCI0 RXD` → 分配到物理引脚（如 P505）→ Mode: `Peripheral` → Peripheral: `SCI0 RXD`

> ✅ FSP 会自动生成引脚初始化代码。

---

### 2.4 配置中断（可选）

在 `Interrupts` 标签页：

- 勾选 `Receive Interrupt`（接收中断）
- 勾选 `Transmit Interrupt`（发送中断）
- 设置中断优先级（如 8）

> ✅ FSP 会自动生成中断服务函数框架。

---

### 2.5 配置 DMA（可选，高性能场景）

1. 添加 `DTC` 模块（Driver → Transfer → DTC）。
2. 在 SCI 配置中勾选 `Use DTC for Receive/Transmit`。
3. 配置 DTC 通道和触发源。

📌 本章先讲解中断方式，DMA 方式在后续章节详解。

---

## 3. SCI 相关 FSP 函数详解

> 📌 所有函数声明在 `r_sci_uart.h` 中。

---

### 3.1 R_SCI_UART_Open

#### 3.1.1 函数原型及功能

初始化并打开 SCI UART 通道。

```c
fsp_err_t R_SCI_UART_Open(sci_uart_ctrl_t * const p_ctrl, sci_uart_cfg_t const * const p_cfg)
```

#### 3.1.2 参数

- `sci_uart_ctrl_t * const p_ctrl`：  
  UART 控制句柄，**用户定义**（如 `g_uart0_ctrl`）。  
  ⚠️ 若为 NULL，返回 `FSP_ERR_ASSERTION`。

- `sci_uart_cfg_t const * const p_cfg`：  
  指向配置结构体，由 FSP 自动生成（如 `g_uart0_cfg`）。  
  包含波特率、数据位、停止位、回调函数指针等。

#### 3.1.3 返回值

- `FSP_SUCCESS`：初始化成功。
- `FSP_ERR_ASSERTION`：`p_ctrl` 或 `p_cfg` 为 NULL。
- `FSP_ERR_ALREADY_OPEN`：通道已打开。
- `FSP_ERR_INVALID_ARGUMENT`：配置参数非法（如波特率无法实现）。

📌 **典型用法（FSP 自动生成）：**

```c
fsp_err_t err = R_SCI_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
if (FSP_SUCCESS != err) while(1);
```

---

### 3.2 R_SCI_UART_Write

#### 3.2.1 函数原型及功能

发送数据（支持单字节或多字节）。

```c
fsp_err_t R_SCI_UART_Write(sci_uart_ctrl_t * const p_ctrl, uint8_t const * const p_src, uint32_t const bytes)
```

#### 3.2.2 参数

- `sci_uart_ctrl_t * const p_ctrl`：已打开的 UART 句柄。
- `uint8_t const * const p_src`：指向发送数据缓冲区的指针。
- `uint32_t const bytes`：要发送的字节数（1 ~ 65535）。

#### 3.2.3 返回值

- `FSP_SUCCESS`：发送启动成功（数据进入发送缓冲区或 FIFO）。
- `FSP_ERR_NOT_OPEN`：通道未打开。
- `FSP_ERR_ASSERTION`：`p_ctrl` 或 `p_src` 为 NULL。
- `FSP_ERR_IN_USE`：上一次发送尚未完成（若未使用中断/DMA）。

📌 **注意**：  
此函数为 **非阻塞**，立即返回。实际发送在后台进行（通过中断或轮询）。

📌 **用法示例：**

```c
char msg[] = "Hello RA6E2!\r\n";
R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*)msg, strlen(msg));
```

---

### 3.3 R_SCI_UART_Read

#### 3.3.1 函数原型及功能

启动接收（指定接收字节数）。

```c
fsp_err_t R_SCI_UART_Read(sci_uart_ctrl_t * const p_ctrl, uint8_t * const p_dest, uint32_t const bytes)
```

#### 3.3.2 参数

- `sci_uart_ctrl_t * const p_ctrl`
- `uint8_t * const p_dest`：接收缓冲区指针。
- `uint32_t const bytes`：期望接收的字节数。

#### 3.3.3 返回值

- `FSP_SUCCESS`：接收启动成功。
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`
- `FSP_ERR_IN_USE`：上一次接收未完成。

📌 **注意**：  
此函数启动接收后，数据到达时会触发中断，最终在回调函数中通知用户。

---

### 3.4 R_SCI_UART_InfoGet

#### 3.4.1 函数原型及功能

获取 UART 当前状态信息。

```c
fsp_err_t R_SCI_UART_InfoGet(sci_uart_ctrl_t * const p_ctrl, sci_uart_info_t * const p_info)
```

#### 3.4.2 参数

- `sci_uart_ctrl_t * const p_ctrl`
- `sci_uart_info_t * const p_info`：返回状态结构体，包含：
  - `loaded`：已加载的字节数（发送/接收）
  - `remaining`：剩余字节数
  - `status`：状态标志（如 `SCI_UART_STATUS_TX_IDLE`）

#### 3.4.3 返回值

- `FSP_SUCCESS`
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`

📌 **用法示例（查询发送是否完成）：**

```c
sci_uart_info_t info;
R_SCI_UART_InfoGet(&g_uart0_ctrl, &info);
if (info.status & SCI_UART_STATUS_TX_IDLE) {
    // 发送空闲，可发送下一帧
}
```

---

### 3.5 R_SCI_UART_Abort

#### 3.5.1 函数原型及功能

中止当前发送或接收操作。

```c
fsp_err_t R_SCI_UART_Abort(sci_uart_ctrl_t * const p_ctrl, sci_uart_dir_t dir)
```

#### 3.5.2 参数

- `sci_uart_ctrl_t * const p_ctrl`
- `sci_uart_dir_t dir`：
  - `SCI_UART_DIR_TX`：中止发送
  - `SCI_UART_DIR_RX`：中止接收
  - `SCI_UART_DIR_TX_RX`：中止收发

#### 3.5.3 返回值

- `FSP_SUCCESS`
- `FSP_ERR_NOT_OPEN`
- `FSP_ERR_ASSERTION`

---

### 3.6 R_SCI_UART_Close

#### 3.6.1 函数原型及功能

关闭 UART 通道，释放资源。

```c
fsp_err_t R_SCI_UART_Close(sci_uart_ctrl_t * const p_ctrl)
```

#### 3.6.2 参数 & 返回值

同 `R_SCI_UART_Open`。

---

## 4. SCI 中断回调函数详解

当配置了中断后，FSP 会生成如下回调函数框架：

```c
void user_uart_callback(uart_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case UART_EVENT_RX_COMPLETE:
            // 接收完成
            break;

        case UART_EVENT_TX_COMPLETE:
            // 发送完成
            break;

        case UART_EVENT_ERR_PARITY:
        case UART_EVENT_ERR_FRAMING:
        case UART_EVENT_ERR_OVERFLOW:
            // 错误处理
            break;

        default:
            break;
    }
}
```

### 4.1 常见事件类型

| 事件类型                      | 说明          |
| ------------------------- | ----------- |
| `UART_EVENT_RX_COMPLETE`  | 接收指定字节数完成   |
| `UART_EVENT_TX_COMPLETE`  | 发送指定字节数完成   |
| `UART_EVENT_ERR_PARITY`   | 奇偶校验错误      |
| `UART_EVENT_ERR_FRAMING`  | 帧错误（停止位不正确） |
| `UART_EVENT_ERR_OVERFLOW` | 接收缓冲区溢出     |

📌 **重要**：在回调函数中 **避免执行耗时操作**，建议设置标志位，主循环中处理。

---

## 5. 实战：SCI 中断收发示例

### 5.1 配置回顾

- SCI0，波特率 115200，8N1
- TX: P504, RX: P505
- 使能接收中断和发送中断

---

### 5.2 用户代码

```c

```

---

## 6. SCI 函数速查表

| 函数名                  | 用途      | 关键参数              | 返回值/错误码        | 注意事项         |
| -------------------- | ------- | ----------------- | -------------- | ------------ |
| `R_SCI_UART_Open`    | 初始化UART | `p_ctrl`, `p_cfg` | `ALREADY_OPEN` | 必须最先调用       |
| `R_SCI_UART_Write`   | 发送数据    | `p_src`, `bytes`  | `IN_USE`       | 非阻塞，后台发送     |
| `R_SCI_UART_Read`    | 启动接收    | `p_dest`, `bytes` | `IN_USE`       | 数据到达触发回调     |
| `R_SCI_UART_InfoGet` | 获取状态    | `p_info`          | -              | 查询收发进度       |
| `R_SCI_UART_Abort`   | 中止收发    | `dir`             | -              | 紧急停止用        |
| `R_SCI_UART_Close`   | 关闭UART  | -                 | -              | 释放资源         |
| `user_uart_callback` | 中断回调    | `p_args->event`   | 无返回值           | 中断上下文，避免耗时操作 |

---

## 7. 常见问题与调试技巧

### 7.1 波特率误差过大

- **现象**：数据乱码。
- **原因**：外设时钟（PCLK）与波特率计算不匹配。
- **解决**：
  1. 在 FSP `Clocks` 标签页确认 SCI 通道的时钟源（如 PCLKA）。
  2. 在 `SCI UART` 配置中，FSP 会自动计算分频系数，若误差 > 1.5%，会标红警告。
  3. 可尝试更换主时钟源（如改用 Main OSC 提高精度）。

---

### 7.2 接收中断不触发

- **检查**：
  1. 引脚是否配置为 `Peripheral` 模式？
  2. 是否调用了 `R_SCI_UART_Read` 启动接收？
  3. 中断是否在 `Interrupts` 标签页使能？
  4. NVIC 优先级是否被其他中断屏蔽？

---

### 7.3 发送数据丢失

- **原因**：连续调用 `R_SCI_UART_Write` 而未等待前一次完成。
- **解决**：
  - 使用回调函数 `UART_EVENT_TX_COMPLETE` 确认发送完成。
  - 或使用 `R_SCI_UART_InfoGet` 查询状态。

---

## 8. 开发建议

✅ **推荐做法**：

1. **始终使用中断或DMA方式**，避免阻塞式轮询。
2. **接收缓冲区大小合理**（如 64/128/256 字节），避免频繁中断。
3. **在回调函数中仅设置标志位**，复杂处理放主循环。
4. **使能溢出错误中断**，及时恢复通信。
5. **多串口系统，为每个通道定义独立的回调函数**。

⛔ **避免做法**：

1. **在中断回调中调用 `R_BSP_SoftwareDelay` 等阻塞函数**。
2. **未检查返回值直接连续发送**。
3. **忽略错误事件，导致通信死锁**。

---


