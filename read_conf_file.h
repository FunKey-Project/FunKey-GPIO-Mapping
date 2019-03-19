#ifndef _READ_CONF_FILE_H_
#define _READ_CONF_FILE_H_


/****************************************************************
* Includes
****************************************************************/
#include <stdbool.h>

/****************************************************************
* Defines
****************************************************************/
#define MAX_NUM_GPIO 					32

#define MAX_SIMULTANEOUS_GPIO			12

#define MAX_LENGTH_STR_PINS				48
#define MAX_LENGTH_STR_TYPE				16
#define MAX_LENGTH_STR_VALUE			256
#define MAX_LENGTH_STR_HELP_PIN_NAME	48
#define MAX_LENGTH_STR_HELP_PIN_FCT		256
#define MAX_LN_LENGTH 					(MAX_LENGTH_STR_PINS+\
										MAX_LENGTH_STR_TYPE+\
										MAX_LENGTH_STR_VALUE+\
										MAX_LENGTH_STR_HELP_PIN_NAME+\
										MAX_LENGTH_STR_HELP_PIN_FCT)

typedef enum{
	TYPE_MAPPING_KEYBOARD,
	TYPE_MAPPING_SHELL_COMMAND,
	NB_TYPE_MAPPING,
} ENUM_TYPE_MAPPING;


typedef enum{
	MAPPING_ARG_PINS = 0,
	MAPPING_ARG_TYPE,
	MAPPING_ARG_VALUE,
	MAPPING_ARG_STR_HELP_PIN_NAME,
	MAPPING_ARG_STR_HELP_PIN_FCT,
	NB_MAPPING_ARG_NAMES,
} ENUM_MAPPING_ARGS_NAMES;


typedef struct{
	char name[32];
	int code;
}key_names_s;


typedef struct gpio_mapping_s{
	int pins_idx[MAX_SIMULTANEOUS_GPIO];
	int nb_simultaneous_pins;
	ENUM_TYPE_MAPPING type_mapping;
	int key_value; 
	char *shell_command;
	char *pin_help_str;
	char *fct_help_str;
	struct gpio_mapping_s * next_mapped_gpio;
	bool activated;
}STRUCT_MAPPED_GPIO;



/****************************************************************
* Public functions
****************************************************************/
void get_mapping_from_conf_file(STRUCT_MAPPED_GPIO ** chained_list_mapping, 
	int* nb_valid_gpios, int ** valid_gpio_pins);
void print_all_chained_list(STRUCT_MAPPED_GPIO * head);
void print_chained_list_node(STRUCT_MAPPED_GPIO * node);
void print_gpios(int * valid_gpio_pins, int nb_valid_gpios);

#endif	//_READ_CONF_FILE_H_
