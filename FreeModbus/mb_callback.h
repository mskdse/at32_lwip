#ifndef __MB_CALLBACK_H
#define __MB_CALLBACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ÏßÈ¦²Ù×÷ */
void     mb_set_coil(uint16_t addr, uint8_t value);
uint8_t  mb_get_coil(uint16_t addr);

/* ÀëÉ¢ÊäÈë²Ù×÷ */
void     mb_set_discrete_input(uint16_t addr, uint8_t value);

/* ±£³Ö¼Ä´æÆ÷²Ù×÷ */
void     mb_set_holding_reg(uint16_t addr, uint16_t value);
uint16_t mb_get_holding_reg(uint16_t addr);
void     mb_set_holding_regs(uint16_t start_addr, uint16_t *values, uint16_t count);
void     mb_get_holding_regs(uint16_t start_addr, uint16_t *values, uint16_t count);

/* ÊäÈë¼Ä´æÆ÷²Ù×÷ */
void     mb_set_input_reg(uint16_t addr, uint16_t value);
uint16_t mb_get_input_reg(uint16_t addr);
void     mb_set_input_regs(uint16_t start_addr, uint16_t *values, uint16_t count);
void     mb_get_input_regs(uint16_t start_addr, uint16_t *values, uint16_t count);

#ifdef __cplusplus
}
#endif

#endif /* __MB_CALLBACK_H */
