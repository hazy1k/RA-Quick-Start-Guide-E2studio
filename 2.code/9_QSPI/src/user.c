/*
 * user.c
 *
 *  Created on: 2025年9月27日
 *      Author: qiu
 */
#include "user.h"
#include "qspi.h"

// 定义Flash芯片型号的ID标识
#define  FLASH_ID_W25Q32JV      0xEF6016  // W25Q32JV型号Flash的ID
#define  FLASH_ID_W25Q64JV      0xEF6017  // W25Q64JV型号Flash的ID
#define  TEST_SIZE  20          // 测试数据数量

/* 存储要写入Flash的整数数组 */
int wbuffer[TEST_SIZE] = {0};

/* 存放从Flash读取的数据的数组 */
int rbuffer[TEST_SIZE] = {0};

/**
 * @brief  主运行函数，实现Flash的检测、数据写入和读取验证
 * @note   流程包括：初始化硬件、检测Flash、生成测试数据、写入Flash、读取Flash、验证数据
 * @retval 无
 */
void bsp_run(void)
{
    unsigned int FlashID = 0;  // 用于存储读取到的Flash ID
    uint16_t i;                // 循环计数器

    // 初始化UART9，用于调试信息输出
    //UART9_Init();

    /* 初始化QSPI接口和Flash设备 */
    W25Q_Init();
    // 读取Flash的ID信息，用于识别Flash型号
    FlashID = Flash_ReadID();
    // 输出Flash的制造商ID和设备ID（高位为制造商ID，低位为设备ID）
    //printf("FlashID：%x \r\n", (FlashID >> 8));          // 输出高8位的制造商ID
    //printf("FlashDeviceID：%x \r\n", FlashID & 0xff);   // 输出低8位的设备ID

    // 判断是否成功检测到Flash（0xFFFFFFFF表示未检测到有效设备）
    if (FlashID != 0xFFFFFFFF)
    {
        // 根据读取到的ID判断Flash型号并输出
        //printf("检测到FLASH：%s \r\n",
        //     (FlashID >> 8) == FLASH_ID_W25Q32JV ? "W25Q32JV" : "其他型号");

        /* 生成要写入Flash的测试数据 */
        for (i = 0; i < TEST_SIZE; i++)
        {
            // 生成0-255之间的随机数并存储到发送缓冲区
            wbuffer[i] = (uint8_t)(rand() % 256);
        }

        /* 将发送缓冲区的数据写入到Flash中 */
        // 第一个参数为写入地址，第二个参数为数据源，第三个参数为数据长度
        W25Q_Write(TEST_SIZE, (void*)wbuffer, sizeof(wbuffer));

        /* 从Flash中读取数据到接收缓冲区 */
        // 第一个参数为读取地址，第二个参数为数据存储目标，第三个参数为读取长度
        W25Q_Read(TEST_SIZE, (void*)rbuffer, sizeof(wbuffer));

        // 输出写入和读取的数据，用于验证是否一致
        //printf("\r\n数据测试:\n\t");
        for (i = 0; i < TEST_SIZE; i++)
        {
            //printf("wbuffer[%d] = %d \t rbuffer[%d] = %d\r\n\t ",
            //       i, wbuffer[i], i, rbuffer[i]);
        }
        //printf("\r\n");
    }
    else
    {
        // 未检测到Flash设备时输出提示信息
        //printf("未检测到FLASH设备！！\r\n");
    }
}
