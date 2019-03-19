#ifndef _DRIVER_PCAL6416A_H_
#define _DRIVER_PCAL6416A_H_


 /****************************************************************
 * Includes
 ****************************************************************/
#include <stdbool.h>

 /****************************************************************
 * Defines
 ****************************************************************/
// Chip physical address
#define PCAL6416A_I2C_ADDR			0x20

// Chip registers adresses
#define PCAL6416A_INPUT			0x00 /* Input port [RO] */
#define PCAL6416A_DAT_OUT      		0x02 /* GPIO DATA OUT [R/W] */
#define PCAL6416A_POLARITY     		0x04 /* Polarity Inversion port [R/W] */
#define PCAL6416A_CONFIG       		0x06 /* Configuration port [R/W] */
#define PCAL6416A_DRIVE0		0x40 /* Output drive strength Port0 [R/W] */
#define PCAL6416A_DRIVE1		0x42 /* Output drive strength Port1 [R/W] */
#define PCAL6416A_INPUT_LATCH		0x44 /* Port0 Input latch [R/W] */
#define PCAL6416A_EN_PULLUPDOWN		0x46 /* Port0 Pull-up/Pull-down enbl [R/W] */
#define PCAL6416A_SEL_PULLUPDOWN	0x48 /* Port0 Pull-up/Pull-down slct [R/W] */
#define PCAL6416A_INT_MASK		0x4A /* Interrupt mask [R/W] */ 
#define PCAL6416A_INT_STATUS		0x4C /* Interrupt status [RO] */ 
#define PCAL6416A_OUTPUT_CONFIG 	0x4F /* Output port config [R/W] */ 

/****************************************************************
 * Public functions
 ****************************************************************/
bool pcal6416a_init(void);
bool pcal6416a_deinit(void);
uint16_t pcal6416a_read_mask_interrupts(void);
uint16_t pcal6416a_read_mask_active_GPIOs(void);


#endif	//_DRIVER_PCAL6416A_H_
