#ifndef _GPIO_MAPPING_H_
#define _GPIO_MAPPING_H_

/****************************************************************
* Includes
****************************************************************/
#include "gpio-utils.h"
#include "uinput.h" 
#include "read_conf_file.h"

/****************************************************************
* Defines
****************************************************************/
#define GPIO_PIN_I2C_EXPANDER_INTERRUPT		35			//PB3
#define GPIO_PIN_AXP209_INTERRUPT			37			//PB5

/****************************************************************
* Public functions
****************************************************************/
int init_mapping_gpios(int * gpio_pins_to_declare, int nb_gpios_to_declare, 
	STRUCT_MAPPED_GPIO * chained_list_mapping_gpios);
int deinit_mapping_gpios(void);
int listen_gpios_interrupts(void);

#endif	//_GPIO_MAPPING_H_
