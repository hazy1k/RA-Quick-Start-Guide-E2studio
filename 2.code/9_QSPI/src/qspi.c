/*
 * qspi.c
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */
#include "qspi.h"

/**
 * @brief  QSPI Flash初始化函数
 * @note   初始化QSPI控制器，配置与Flash的通信参数
 * @retval 无
 */
void W25Q_Init(void)
{
    fsp_err_t err = R_QSPI_Open(&g_qspi0_flash_ctrl, g_qspi0_flash.p_cfg);
    if (err != FSP_SUCCESS)
    {
        //printf("QSPI启动错误\n");
    }
}

/**
 * @brief  读取Flash的ID信息
 * @note   读取Flash的制造商ID和设备ID，用于识别Flash型号
 * @retval 32位ID信息，包含制造商ID和设备ID；0xFFFFFFFF表示读取失败，高24位为JEDEC ID，低8位为设备ID
 */
uint32_t Flash_ReadID(void)
{
    uint8_t cmd = 0x9F;               // 读取JEDEC ID指令
    uint8_t cmd_seq[4] = {0xAB, 0xFF, 0xFF, 0xFF};  // 读取设备ID的指令序列
    uint8_t id_buf[3] = {0};          // 存储读取到的ID信息
    uint8_t dev_id = 0;               // 存储设备ID

    // 发送读取JEDEC ID指令
    fsp_err_t err = R_QSPI_DirectWrite(&g_qspi0_flash_ctrl, &cmd, 1, true);
    if (err != FSP_SUCCESS)
    {
        //printf("ID指令发送失败\n");
        return 0xFFFFFFFF;
    }

    // 读取3字节ID信息（制造商ID和设备ID高字节）
    err = R_QSPI_DirectRead(&g_qspi0_flash_ctrl, id_buf, 3);
    if (err != FSP_SUCCESS)
    {
        //printf("ID读取失败\n");
        return 0xFFFFFFFF;
    }

    // 发送读取设备ID的指令序列（0xAB为Release Power-Down/Device ID指令）
    err = R_QSPI_DirectWrite(&g_qspi0_flash_ctrl, cmd_seq, 4, true);
    if (err != FSP_SUCCESS)
    {
        //printf("设备ID指令失败\n");
        return 0xFFFFFFFF;
    }

    // 读取1字节设备ID
    err = R_QSPI_DirectRead(&g_qspi0_flash_ctrl, &dev_id, 1);
    if (err != FSP_SUCCESS)
    {
        //printf("设备ID读取失败\n");
        return 0xFFFFFFFF;
    }

    // 组合32位ID返回（高24位为JEDEC ID，低8位为设备ID）
    return (id_buf[0] << 24) | (id_buf[1] << 16) | (id_buf[2] << 8) | dev_id;
}

/**
 * @brief  等待Flash操作完成
 * @note   轮询Flash状态，直到当前操作（如擦除、写入）完成
 * @retval FSP_SUCCESS表示操作完成；其他错误码表示失败
 */
static fsp_err_t W25Q_WaitReady(void)
{
    spi_flash_status_t flash_stat;    // Flash状态结构体
    flash_stat.write_in_progress = true;  // 初始化为操作中状态

    // 循环等待，直到写入操作完成
    while (flash_stat.write_in_progress)
    {
        // 获取Flash当前状态
        fsp_err_t err = R_QSPI_StatusGet(&g_qspi0_flash_ctrl, &flash_stat);
        if (err != FSP_SUCCESS)
        {
            return err;
        }
    }

    return FSP_SUCCESS;
}

/**
 * @brief  从Flash指定地址读取数据
 * @param  addr: 读取起始地址
 * @param  buf: 接收数据的缓冲区指针
 * @param  Size: 要读取的数据长度（字节）
 * @retval 成功读取的字节数；0表示失败
 */
int W25Q_Read(uint32_t addr, uint8_t *buf, uint32_t Size)
{
    // 参数合法性检查：缓冲区为空或长度为0则返回失败
    if (buf == NULL || Size == 0)
    {
        //printf("读取参数错误\n");
        return 0;
    }

    // 计算实际地址
    uint32_t real_addr = addr + QSPI_START_ADDR;
    // 读取指令帧（0x03为Read Data指令，后3字节为地址）
    uint8_t read_cmd[4] = {0x03, 0x00, 0x00, 0x00};

    // 填充地址到指令帧
    read_cmd[1] = (real_addr >> 16) & 0xFF;
    read_cmd[2] = (real_addr >>  8) & 0xFF;
    read_cmd[3] =  real_addr        & 0xFF;

    // 等待Flash就绪
    if (W25Q_WaitReady() != FSP_SUCCESS)
        return 0;

    // 发送读取指令
    fsp_err_t err = R_QSPI_DirectWrite(&g_qspi0_flash_ctrl, read_cmd, 4, true);
    if (err != FSP_SUCCESS)
    {
        //printf("读指令发送失败\n");
        return 0;
    }

    // 读取数据到缓冲区
    err = R_QSPI_DirectRead(&g_qspi0_flash_ctrl, buf, Size);
    if (err != FSP_SUCCESS)
    {
        //printf("数据读取失败\n");
        return 0;
    }

    // 返回成功读取的字节数
    return (int)Size;
}

/**
 * @brief  擦除Flash中需要写入数据的区域
 * @note   基于Flash特性，写入前需要先擦除对应扇区（4096字节/扇区）
 * @param  addr: 起始地址
 * @param  Size: 数据长度
 * @retval FSP_SUCCESS表示成功；其他错误码表示失败
 */
static fsp_err_t W25Q_Clean_Page(uint32_t addr, uint32_t Size)
{
    // 计算需要擦除的扇区数量（向上取整）
    uint32_t sec_count = (Size + addr % 4096) / 4096 + 1;
    uint32_t curr_sec = addr;  // 当前要擦除的扇区地址

    // 逐个擦除扇区
    while (sec_count--)
    {
        // 擦除指定扇区（4096字节）
        fsp_err_t err = R_QSPI_Erase(&g_qspi0_flash_ctrl,
                              (uint8_t*)(QSPI_START_ADDR + curr_sec),
                             4096);
        if (err != FSP_SUCCESS)
        {
            //printf("扇区0x%08lX擦除失败\n", curr_sec);
            return 0;
        }

        // 等待擦除完成
        if (W25Q_WaitReady() != FSP_SUCCESS)
            return 0;

        // 移动到下一个扇区
        curr_sec += 4096;
    }

    return FSP_SUCCESS;
}

/**
 * @brief  向Flash指定地址写入数据
 * @note   支持跨页写入，自动处理页边界；写入前会先擦除对应扇区
 * @param  addr: 写入起始地址（相对于Flash内部地址）
 * @param  data: 要写入的数据缓冲区指针
 * @param  Size: 要写入的数据长度（字节）
 * @retval 成功写入的字节数；0表示失败
 */
int W25Q_Write(uint32_t addr, uint8_t *data, uint32_t Size)
{
    fsp_err_t err;

    // 计算需要写入的页数（256字节/页），向上取整
    uint32_t dwPageCount = (Size + addr % 256) / 256 + 1;
    uint32_t curr_addr = addr;    // 当前写入地址
    uint32_t P_Size = Size;       // 剩余写入长度
    uint8_t *p_data = data;       // 当前数据指针

    // 参数合法性检查：数据为空或长度为0则返回失败
    if (data == NULL || Size == 0)
    {
        //printf("写入参数错误\n");
        return 0;
    }

    // 擦除需要写入的区域
    W25Q_Clean_Page(curr_addr, P_Size);

    // 等待擦除完成
    if (W25Q_WaitReady() != FSP_SUCCESS)
        return 0;

    // 单页写入（数据不跨页）
    if (dwPageCount == 1)
    {
        err = R_QSPI_Write(&g_qspi0_flash_ctrl,
                            (uint8_t*)p_data,
                            (uint8_t*)(QSPI_START_ADDR + curr_addr),
                            P_Size);
        if (err != FSP_SUCCESS)
        {
            //printf("单页写入失败\n");
            return 0;
        }
    }
    // 多页写入（数据跨页）
    else
    {
        // 计算第一页可写入的字节数
        unsigned int first_Size = 256 - (addr % 256);

        // 写入第一页数据
        err = R_QSPI_Write(&g_qspi0_flash_ctrl,
                            (uint8_t*)p_data,
                            (uint8_t*)(QSPI_START_ADDR + curr_addr),
                            first_Size);
        if (err != FSP_SUCCESS)
        {
            //printf("首页写入失败\n");
            return 0;
        }

        // 等待第一页写入完成
        if (W25Q_WaitReady() != FSP_SUCCESS)
            return 0;

        // 更新地址和数据指针
        curr_addr += first_Size;
        p_data += first_Size;

        // 写入剩余数据（按页处理）
        for (uint32_t remain = P_Size - first_Size; remain > 0; remain -= 256) {
            // 计算当前页要写入的字节数（不超过一页）
            uint32_t write_Size = (remain > 256) ? 256 : remain;

            // 写入当前页数据
            R_QSPI_Write(&g_qspi0_flash_ctrl,
                            p_data,
                            (uint8_t*)(QSPI_START_ADDR + curr_addr),
                            write_Size);

            // 等待当前页写入完成
            if (W25Q_WaitReady() != FSP_SUCCESS)
                return 0;

            // 更新地址和数据指针
            curr_addr += write_Size;
            p_data += write_Size;
        }
    }

    // 返回成功写入的字节数
    return (int)P_Size;
}
