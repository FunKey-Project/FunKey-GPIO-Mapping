#ifndef _DRIVER_AXP209_H_
#define _DRIVER_AXP209_H_


 /****************************************************************
 * Includes
 ****************************************************************/
#include <stdbool.h>

 /****************************************************************
 * Defines
 ****************************************************************/
// Chip physical address
#define AXP209_I2C_ADDR			0x34

// Chip registers adresses
#define AXP209_REG_32H						0x32 
#define AXP209_INTERRUPT_BANK_1_ENABLE		0x40 
#define AXP209_INTERRUPT_BANK_1_STATUS		0x48 
#define AXP209_INTERRUPT_BANK_2_ENABLE		0x41
#define AXP209_INTERRUPT_BANK_2_STATUS		0x49 
#define AXP209_INTERRUPT_BANK_3_ENABLE		0x42
#define AXP209_INTERRUPT_BANK_3_STATUS		0x4A
#define AXP209_INTERRUPT_BANK_4_ENABLE		0x43
#define AXP209_INTERRUPT_BANK_4_STATUS		0x4B 
#define AXP209_INTERRUPT_BANK_5_ENABLE		0x44
#define AXP209_INTERRUPT_BANK_5_STATUS		0x4C

// Masks
#define AXP209_INTERRUPT_PEK_SHORT_PRESS	0x02 
#define AXP209_INTERRUPT_PEK_LONG_PRESS		0x01 

/****************************************************************
 * Public functions
 ****************************************************************/
bool axp209_init(void);
bool axp209_deinit(void);
int axp209_read_interrupt_bank_3(void);


#endif	//_DRIVER_AXP209_H_
