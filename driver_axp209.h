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
#define AXP209_INTERRUPT_BANK_3_STATUS		0x4A 

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
