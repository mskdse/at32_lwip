#include "mb_callback.h"
#include "mb.h"
#include <string.h>

/* ----------------------- 寄存器区域大小定义 ----------------------- */
#define MB_COILS_MAX          128     /* 线圈数量（0x） */
#define MB_DISCRETE_MAX       128     /* 离散输入数量（1x） */
#define MB_HOLDING_MAX        256     /* 保持寄存器数量（4x） */
#define MB_INPUT_MAX          256     /* 输入寄存器数量（3x） */

/* ----------------------- 寄存器存储区 ----------------------- */
static uint8_t  coils[MB_COILS_MAX / 8 + 1];           /* 线圈存储（位域） */
static uint8_t  discrete_inputs[MB_DISCRETE_MAX / 8 + 1]; /* 离散输入存储（位域） */
static uint16_t holding_regs[MB_HOLDING_MAX];          /* 保持寄存器 */
static uint16_t input_regs[MB_INPUT_MAX];              /* 输入寄存器 */

/* ----------------------- 线圈（0x）读写 ----------------------- */
eMBErrorCode
eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils,
              eMBRegisterMode eMode)
{
    /* 地址越界检查 */
    if (usAddress + usNCoils > MB_COILS_MAX) {
        return MB_ENOREG;
    }

    if (eMode == MB_REG_READ) {
        /* 读取线圈：每个 bit 代表一个线圈 */
        memset(pucRegBuffer, 0, (usNCoils + 7) / 8);
        for (USHORT i = 0; i < usNCoils; i++) {
            uint16_t coil_addr = usAddress + i;
            if (coils[coil_addr / 8] & (0x01 << (coil_addr % 8))) {
                pucRegBuffer[i / 8] |= (0x01 << (i % 8));
            }
        }
    } else {
        /* 写线圈：每个 bit 代表一个线圈 */
        for (USHORT i = 0; i < usNCoils; i++) {
            uint16_t coil_addr = usAddress + i;
            if (pucRegBuffer[i / 8] & (0x01 << (i % 8))) {
                coils[coil_addr / 8] |= (0x01 << (coil_addr % 8));
            } else {
                coils[coil_addr / 8] &= ~(0x01 << (coil_addr % 8));
            }
        }
    }
    return MB_ENOERR;
}

/* ----------------------- 离散输入（1x）读取 ----------------------- */
eMBErrorCode
eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    /* 地址越界检查 */
    if (usAddress + usNDiscrete > MB_DISCRETE_MAX) {
        return MB_ENOREG;
    }

    /* 读取离散输入 */
    memset(pucRegBuffer, 0, (usNDiscrete + 7) / 8);
    for (USHORT i = 0; i < usNDiscrete; i++) {
        uint16_t input_addr = usAddress + i;
        if (discrete_inputs[input_addr / 8] & (0x01 << (input_addr % 8))) {
            pucRegBuffer[i / 8] |= (0x01 << (i % 8));
        }
    }
    return MB_ENOERR;
}

/* ----------------------- 保持寄存器（4x）读写 ----------------------- */
eMBErrorCode
eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                eMBRegisterMode eMode)
{
    /* 地址越界检查 */
    if (usAddress + usNRegs > MB_HOLDING_MAX) {
        return MB_ENOREG;
    }

    if (eMode == MB_REG_READ) {
        /* 读取保持寄存器 */
        for (USHORT i = 0; i < usNRegs; i++) {
            pucRegBuffer[i * 2]     = (UCHAR)(holding_regs[usAddress + i] >> 8);
            pucRegBuffer[i * 2 + 1] = (UCHAR)(holding_regs[usAddress + i] & 0xFF);
        }
    } else {
        /* 写保持寄存器 */
        for (USHORT i = 0; i < usNRegs; i++) {
            holding_regs[usAddress + i] = (uint16_t)((pucRegBuffer[i * 2] << 8) |
                                                      pucRegBuffer[i * 2 + 1]);
        }
    }
    return MB_ENOERR;
}

/* ----------------------- 输入寄存器（3x）读取 ----------------------- */
eMBErrorCode
eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    /* 地址越界检查 */
    if (usAddress + usNRegs > MB_INPUT_MAX) {
        return MB_ENOREG;
    }

    /* 读取输入寄存器 */
    for (USHORT i = 0; i < usNRegs; i++) {
        pucRegBuffer[i * 2]     = (UCHAR)(input_regs[usAddress + i] >> 8);
        pucRegBuffer[i * 2 + 1] = (UCHAR)(input_regs[usAddress + i] & 0xFF);
    }
    return MB_ENOERR;
}

/* ======================= 应用层辅助函数 ======================= */
/* 这些函数供你的应用程序调用，用于设置/读取寄存器值 */

/* 设置单个线圈 */
void mb_set_coil(uint16_t addr, uint8_t value)
{
    if (addr < MB_COILS_MAX) {
        if (value) {
            coils[addr / 8] |= (0x01 << (addr % 8));
        } else {
            coils[addr / 8] &= ~(0x01 << (addr % 8));
        }
    }
}

/* 读取单个线圈 */
uint8_t mb_get_coil(uint16_t addr)
{
    if (addr < MB_COILS_MAX) {
        return (coils[addr / 8] >> (addr % 8)) & 0x01;
    }
    return 0;
}

/* 设置单个离散输入 */
void mb_set_discrete_input(uint16_t addr, uint8_t value)
{
    if (addr < MB_DISCRETE_MAX) {
        if (value) {
            discrete_inputs[addr / 8] |= (0x01 << (addr % 8));
        } else {
            discrete_inputs[addr / 8] &= ~(0x01 << (addr % 8));
        }
    }
}

/* 设置单个保持寄存器 */
void mb_set_holding_reg(uint16_t addr, uint16_t value)
{
    if (addr < MB_HOLDING_MAX) {
        holding_regs[addr] = value;
    }
}

/* 读取单个保持寄存器 */
uint16_t mb_get_holding_reg(uint16_t addr)
{
    if (addr < MB_HOLDING_MAX) {
        return holding_regs[addr];
    }
    return 0;
}

/* 设置单个输入寄存器 */
void mb_set_input_reg(uint16_t addr, uint16_t value)
{
    if (addr < MB_INPUT_MAX) {
        input_regs[addr] = value;
    }
}

/* 读取单个输入寄存器 */
uint16_t mb_get_input_reg(uint16_t addr)
{
    if (addr < MB_INPUT_MAX) {
        return input_regs[addr];
    }
    return 0;
}

/* 批量设置保持寄存器 */
void mb_set_holding_regs(uint16_t start_addr, uint16_t *values, uint16_t count)
{
    if (start_addr + count <= MB_HOLDING_MAX) {
        memcpy(&holding_regs[start_addr], values, count * sizeof(uint16_t));
    }
}

/* 批量读取保持寄存器 */
void mb_get_holding_regs(uint16_t start_addr, uint16_t *values, uint16_t count)
{
    if (start_addr + count <= MB_HOLDING_MAX) {
        memcpy(values, &holding_regs[start_addr], count * sizeof(uint16_t));
    }
}

/* 批量设置输入寄存器 */
void mb_set_input_regs(uint16_t start_addr, uint16_t *values, uint16_t count)
{
    if (start_addr + count <= MB_INPUT_MAX) {
        memcpy(&input_regs[start_addr], values, count * sizeof(uint16_t));
    }
}

/* 批量读取输入寄存器 */
void mb_get_input_regs(uint16_t start_addr, uint16_t *values, uint16_t count)
{
    if (start_addr + count <= MB_INPUT_MAX) {
        memcpy(values, &input_regs[start_addr], count * sizeof(uint16_t));
    }
}
