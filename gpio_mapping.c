#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/input.h>
#include "gpio_mapping.h"
#include "driver_pcal6416a.h"
#include "driver_axp209.h"

/****************************************************************
 * Defines
 ****************************************************************/
#define die(str, args...) do { \
        perror(str); \
        return(EXIT_FAILURE); \
    } while(0)

#define DEBUG_GPIO_PRINTF 			(1)
#define DEBUG_PERIODIC_CHECK_PRINTF (0)
#define ERROR_GPIO_PRINTF 			(1)

#if (DEBUG_GPIO_PRINTF)
#define GPIO_PRINTF(...) printf(__VA_ARGS__);
#else
#define GPIO_PRINTF(...)
#endif

#if (DEBUG_PERIODIC_CHECK_PRINTF)
#define PERIODIC_CHECK_PRINTF(...) printf(__VA_ARGS__);
#else
#define PERIODIC_CHECK_PRINTF(...)
#endif

#if (ERROR_GPIO_PRINTF)
#define GPIO_ERROR_PRINTF(...) printf(__VA_ARGS__);
#else
#define GPIO_ERROR_PRINTF(...)
#endif

// This define forces to perform a gpio sanity check after a timeout. 
// If not declared, there will be no timeout and no periodical sanity check of GPIO expander values
//#define TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP	1
#define TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP	(30*1000)

// Comment this for debug purposes on cards or eval boards that do not have the AXP209
#define ENABLE_AXP209_INTERRUPTS

#define KEY_IDX_MAPPED_FOR_SHORT_PEK_PRESS	16 	//KEY_Q
#define KEY_IDX_MAPPED_FOR_LONG_PEK_PRESS	28 	//KEY_ENTER


/****************************************************************
 * Static variables
 ****************************************************************/
static int nb_mapped_gpios;
static int * gpio_pins_idx_declared;
static bool * gpios_pins_active_high;
STRUCT_MAPPED_GPIO * chained_list_mapping;
static int max_fd = 0;
static int gpio_fd_interrupt_expander_gpio;
static int gpio_fd_interrupt_axp209;
static fd_set fds;
static bool * mask_gpio_value;
static bool interrupt_i2c_expander_found = false;
static bool interrupt_axp209_found = false;


/****************************************************************
 * Static functions
 ****************************************************************/
 /*****  Add a fd to fd_set, and update max_fd_nb  ******/
static int safe_fd_set(int fd, fd_set* list_fds, int* max_fd_nb) {
    assert(max_fd_nb != NULL);

    FD_SET(fd, list_fds);
    if (fd > *max_fd_nb) {
        *max_fd_nb = fd;
    }
    return 0;
}

/*****  Clear fd from fds, update max fd if needed  *****/
static int safe_fd_clr(int fd, fd_set* list_fds, int* max_fd_nb) {
    assert(max_fd_nb != NULL);

    FD_CLR(fd, list_fds);
    if (fd == *max_fd_nb) {
        (*max_fd_nb)--;
    }
    return 0;
}

/*****  Apply mapping activation fuction  *****/
static void apply_mapping_activation(STRUCT_MAPPED_GPIO * node_mapping){
	node_mapping->activated = true;
	if(node_mapping->type_mapping == TYPE_MAPPING_KEYBOARD){
		GPIO_PRINTF("Apply mapping activation fct: Key event: %d\n", node_mapping->key_value);
		sendKey(node_mapping->key_value, 1);
		//sendKeyAndStopKey(node_mapping->key_value);
	}
	else if(node_mapping->type_mapping == TYPE_MAPPING_SHELL_COMMAND){
		GPIO_PRINTF("Apply mapping activation fct: shell command \"%s\"\n", node_mapping->shell_command);
		system(node_mapping->shell_command);
	}
}

/*****  Apply mapping desactivation function  *****/
static void apply_mapping_desactivation(STRUCT_MAPPED_GPIO * node_mapping){
	node_mapping->activated = false;
	if(node_mapping->type_mapping == TYPE_MAPPING_KEYBOARD){
		GPIO_PRINTF("Apply mapping desactivation fct: Key event: %d\n", node_mapping->key_value);
		sendKey(node_mapping->key_value, 0);
	}
	else if(node_mapping->type_mapping == TYPE_MAPPING_SHELL_COMMAND){
		//GPIO_PRINTF("Apply mapping desactivation fct: shell command \"%s\"\n", node_mapping->shell_command);
	}
}


static void find_and_call_mapping_function(int idx_gpio_interrupted,
									STRUCT_MAPPED_GPIO * cl_mapping, 
									bool* mask_gpio_current_interrupts,
									bool * mask_gpio_values,
									bool activation){
	// Init variables
	STRUCT_MAPPED_GPIO * current = cl_mapping;
	bool mapping_found = false;
	
	// Search if there are mappings to deactivate
	while (current != NULL) {
		int i;
		bool gpio_found_pin_in_mapping = false;
		bool all_gpio_activated_in_mapping = true;


		//GPIO_PRINTF("	Mapping searching for idx_gpio_interrupted: %d, and %s: \n", idx_gpio_interrupted, activation?"activation":"deactivation")
		for (i=0; i < current->nb_simultaneous_pins; i++){
			//GPIO_PRINTF("	Pin in mapping: %d, pin_idx=%d\n", gpio_pins_idx_declared[current->pins_idx[i]], current->pins_idx[i]);
			// Find if current mapping contains interrupted pin
			if (current->pins_idx[i] == idx_gpio_interrupted){
				gpio_found_pin_in_mapping = true;
			}

			// Check if all other pins of current mapping are activated or were already activated in previous mask
			if( (!mask_gpio_values[current->pins_idx[i]] && gpios_pins_active_high[current->pins_idx[i]]) ||
				(mask_gpio_values[current->pins_idx[i]] && !gpios_pins_active_high[current->pins_idx[i]]) ){
				all_gpio_activated_in_mapping = false;
			}
		}


		// If this mapping contains the interrupted pin and all other pins activated:
		if(gpio_found_pin_in_mapping && all_gpio_activated_in_mapping){
			// Treating activation cases
			if(activation){
				// if real mapping already found => need to deactivate previously activated ones
				if(mapping_found && current->activated){
					GPIO_PRINTF("	Mapping Deactivation because real one already found: GPIO %d found in following activated mapping: \n", 
						gpio_pins_idx_declared[idx_gpio_interrupted]);
					print_chained_list_node(current);
					apply_mapping_desactivation(current);
				}
				else if(!mapping_found){ // If this is the real mapping 
					if(current->activated){
						GPIO_ERROR_PRINTF("WARNING in find_and_call_mapping_function - Activating already activated mapping %s\n", 
							current->fct_help_str);
					}
					// Print information and activate mapping
					GPIO_PRINTF("	Mapping Activation: GPIO %d found in following deactivated mapping: \n", 
						gpio_pins_idx_declared[idx_gpio_interrupted]);
					print_chained_list_node(current);
					apply_mapping_activation(current);
				}
			}
			else{ // Treating deactivation cases
				if(current->activated){
					GPIO_PRINTF("	Mapping Desactivation: GPIO %d found in following activated mapping: \n", 
							gpio_pins_idx_declared[idx_gpio_interrupted]);
					print_chained_list_node(current);
					apply_mapping_desactivation(current);
				}
			}

			// Set that mapping has been found
			mapping_found = true;
		}

		// Next node in chained list
		current = current->next_mapped_gpio;
	}
}

/*****  Init GPIO Interrupt i2c expander fd  *****/
static int init_gpio_interrupt(int pin_nb, int *fd_saved, const char *edge)
{
  	// Variables
  	GPIO_PRINTF("Initializing Interrupt on GPIO pin: %d\n", pin_nb);
	  
	//Initializing I2C interrupt GPIO
	gpio_export(pin_nb); 
	gpio_set_edge(pin_nb, edge);  // Can be rising, falling or both
	//gpio_set_edge(pin_nb, "both");  // Can be rising, falling or both
	//gpio_set_edge(pin_nb, "falling");  // Can be rising, falling or both
	*fd_saved = gpio_fd_open(pin_nb, O_RDONLY);
  	GPIO_PRINTF("fd is: %d\n", *fd_saved);
		
	// add stdin and the sock fd to fds fd_set 
	safe_fd_set(*fd_saved, &fds, &max_fd);

	return 0;
}

/*****  DeInit GPIO Interrupt i2c expander fd  *****/
static int deinit_gpio_interrupt(int fd_saved)
{ 
  	GPIO_PRINTF("DeInitializing Interrupt on GPIO pin fd: %d\n", fd_saved);

	// Remove stdin and  sock fd from fds fd_set 
	safe_fd_clr(fd_saved, &fds, &max_fd);
	
	// Unexport GPIO
	gpio_fd_close(fd_saved);

	return 0;
}


/****************************************************************
 * Public functions
 ****************************************************************/
/*****  Init I2C expander pin mappings *****/
int init_mapping_gpios(int * gpio_pins_to_declare, 
	bool * gpios_pins_active_high_declared, 
	int nb_gpios_to_declare, 
	STRUCT_MAPPED_GPIO * chained_list_mapping_gpios)
{
  	// Variables
  	int idx_gpio;
  	unsigned int cur_pin_nb;

	// Store arguments
	nb_mapped_gpios = nb_gpios_to_declare;
	gpio_pins_idx_declared = gpio_pins_to_declare;
	gpios_pins_active_high = gpios_pins_active_high_declared;
	chained_list_mapping = chained_list_mapping_gpios;
  
	// Init values
	mask_gpio_value = malloc(nb_mapped_gpios*sizeof(bool));
	memset(mask_gpio_value, false, nb_mapped_gpios*sizeof(bool));
	STRUCT_MAPPED_GPIO * current = chained_list_mapping;	
	do{
		//set mapping deactivated
		current->activated = false;
	
		// Next node in chained list
		current = current->next_mapped_gpio;
	} while(current != NULL);
	
	// Init fds fd_set 
	FD_ZERO(&fds);

	// Init GPIO interrupt from I2C GPIO expander
	GPIO_PRINTF("	Initiating interrupt for GPIO_PIN_I2C_EXPANDER_INTERRUPT\n");
	init_gpio_interrupt(GPIO_PIN_I2C_EXPANDER_INTERRUPT, &gpio_fd_interrupt_expander_gpio, "both"); // Can be rising, falling or both

	// Init I2C expander
	pcal6416a_init();

#ifdef ENABLE_AXP209_INTERRUPTS
	// Init GPIO interrupt from AXP209
	GPIO_PRINTF("	Initiating interrupt for GPIO_PIN_AXP209_INTERRUPT\n");
	init_gpio_interrupt(GPIO_PIN_AXP209_INTERRUPT, &gpio_fd_interrupt_axp209, "falling");

	// Init AXP209
	axp209_init();
#endif //ENABLE_AXP209_INTERRUPTS

	return 0;
}

/*****  DeInit GPIO fds  *****/
int deinit_mapping_gpios(void)
{
	// DeInit GPIO interrupt from I2C expander
	GPIO_PRINTF("	DeInitiating interrupt for GPIO_PIN_I2C_EXPANDER_INTERRUPT\n");
	deinit_gpio_interrupt(gpio_fd_interrupt_expander_gpio);

	// DeInit I2C expander
	pcal6416a_deinit();

#ifdef ENABLE_AXP209_INTERRUPTS
	// DeInit GPIO interrupt from AXP209
	GPIO_PRINTF("	DeInitiating interrupt for GPIO_PIN_AXP209_INTERRUPT\n");
	deinit_gpio_interrupt(gpio_fd_interrupt_axp209);

	// DeInit AXP209
	axp209_deinit();
#endif //ENABLE_AXP209_INTERRUPTS

	return 0;
}


/*****  Listen GPIOs interrupts *****/
int listen_gpios_interrupts(void)
{
	// Variables
	char buffer[2];
	int value;
	int idx_gpio;
	int nb_interrupts = 0;
	bool previous_mask_gpio_value[nb_mapped_gpios];
	bool mask_gpio_current_interrupts[nb_mapped_gpios];
	bool forced_interrupt = false;

	// Back up master 
	fd_set dup = fds;

	// Init masks
	memcpy(previous_mask_gpio_value, mask_gpio_value, nb_mapped_gpios*sizeof(bool));
	memset(mask_gpio_value, false, nb_mapped_gpios*sizeof(bool));
	memset(mask_gpio_current_interrupts, false, nb_mapped_gpios*sizeof(bool));

	// If interrupt not already found, waiting for interrupt or timeout, Note the max_fd+1
	if(!interrupt_i2c_expander_found && !interrupt_axp209_found){
		// Listen to interrupts
#ifdef TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP
		struct timeval timeout = {0, TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP};
		nb_interrupts = select(max_fd+1, NULL, NULL, &dup, &timeout);
#elif TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP
		struct timeval timeout = {TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP, 0};
		nb_interrupts = select(max_fd+1, NULL, NULL, &dup, &timeout);
#else
		nb_interrupts = select(max_fd+1, NULL, NULL, &dup, NULL);
#endif //TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP
		if(!nb_interrupts){
			// Timeout case 
			PERIODIC_CHECK_PRINTF("	Timeout, forcing sanity check\n");
			
			// Timeout forcing a "Found interrupt" event for sanity check
			interrupt_i2c_expander_found = true;
			interrupt_axp209_found = true;
			forced_interrupt = true;
		}
		else if ( nb_interrupts < 0) {
			perror("select");
			return -1;
		}
	}

	if(nb_interrupts){
		// Check if interrupt from I2C expander or AXP209
		// Check which cur_fd is available for read 
		for (int cur_fd = 0; cur_fd <= max_fd; cur_fd++) {
			if (FD_ISSET(cur_fd, &dup)) {
				// Rewind file
				lseek(cur_fd, 0, SEEK_SET);
			
				// Read current gpio value
				if (read(cur_fd, & buffer, 2) != 2) {
					perror("read");
					break;
				}
	
				// remove end of line char
				buffer[1] = '\0';
				value = atoi(buffer);
				//value = 1-atoi(buffer);

				// Found interrupt
				if(cur_fd == gpio_fd_interrupt_expander_gpio){
					GPIO_PRINTF("Found interrupt generated by PCAL6416AHF \r\n");
					interrupt_i2c_expander_found = true;
				}
				else if(cur_fd == gpio_fd_interrupt_axp209){
					GPIO_PRINTF("Found interrupt generated by AXP209\r\n");
					interrupt_axp209_found = true;
				}
			}	
		}
	}


#ifdef ENABLE_AXP209_INTERRUPTS
	if(interrupt_axp209_found){
		if(forced_interrupt){
			PERIODIC_CHECK_PRINTF("	Processing forced interrupt AXP209\n");
		}
		else{
			GPIO_PRINTF("	Processing real interrupt AXP209\n");
		}
		int val_int_bank_3 = axp209_read_interrupt_bank_3();
		if(val_int_bank_3 < 0){
			GPIO_PRINTF("	Could not read AXP209 with I2C\n");
			return 0;
		}
		interrupt_axp209_found = false;

		if(val_int_bank_3 & AXP209_INTERRUPT_PEK_SHORT_PRESS){
			GPIO_PRINTF("	AXP209 short PEK key press detected\n");
			sendKeyAndStopKey(KEY_IDX_MAPPED_FOR_SHORT_PEK_PRESS);

		}
		if(val_int_bank_3 & AXP209_INTERRUPT_PEK_LONG_PRESS){
			GPIO_PRINTF("	AXP209 long PEK key press detected\n");
			sendKeyAndStopKey(KEY_IDX_MAPPED_FOR_LONG_PEK_PRESS);
		}
	}
#endif //ENABLE_AXP209_INTERRUPTS

	if(interrupt_i2c_expander_found){
		if(forced_interrupt){
			PERIODIC_CHECK_PRINTF("	Processing forced interrupt PCAL6416AHF\n");
		}
		else{
			GPIO_PRINTF("	Processing real interrupt PCAL6416AHF\n");
		}

		// Read I2C GPIO masks: 
		int val_i2c_mask_interrupted = pcal6416a_read_mask_interrupts();
		if(val_i2c_mask_interrupted < 0){
			GPIO_PRINTF("	Could not read pcal6416a_read_mask_interrupts\n");
			return 0;
		}
		int val_i2c_mask_active = pcal6416a_read_mask_active_GPIOs();
		if(val_i2c_mask_active < 0){
			GPIO_PRINTF("	Could not read pcal6416a_read_mask_active_GPIOs\n");
			return 0;
		}
		interrupt_i2c_expander_found = false;


		// Find GPIO idx correspondance
		for (idx_gpio=0; idx_gpio<nb_mapped_gpios; idx_gpio++){
			uint16_t tmp_mask_gpio = (1 << gpio_pins_idx_declared[idx_gpio]);

			// Found GPIO idx in active GPIOs
			if(val_i2c_mask_active & tmp_mask_gpio){
				mask_gpio_value[idx_gpio] = true;
			}
 
			// Found GPIO idx in interrupt mask
			if(val_i2c_mask_interrupted & tmp_mask_gpio){
				// Print information
				GPIO_PRINTF("	--> Interrupt GPIO: %d, idx_pin: %d\n", gpio_pins_idx_declared[idx_gpio], idx_gpio);
				mask_gpio_current_interrupts[idx_gpio] = true;
			}

			// Sanity check: if we missed an interrupt for some reason, check if the new values are the same
			if( !mask_gpio_current_interrupts[idx_gpio] && 
				(mask_gpio_value[idx_gpio] != previous_mask_gpio_value[idx_gpio]) ){
				// Print information
				GPIO_PRINTF("	--> No Interrupt (missed) but value has changed on GPIO: %d, idx_pin: %d\n", 
					gpio_pins_idx_declared[idx_gpio], idx_gpio);
				mask_gpio_current_interrupts[idx_gpio] = true;
			}
		}


		// Find and call mapping functions (after all active gpios have been assigned):
		for (idx_gpio=0; idx_gpio<nb_mapped_gpios; idx_gpio++){
			if(!mask_gpio_current_interrupts[idx_gpio]) continue;

			if ( (mask_gpio_value[idx_gpio] && gpios_pins_active_high[idx_gpio]) ||
				(!mask_gpio_value[idx_gpio] && !gpios_pins_active_high[idx_gpio]) ){
				find_and_call_mapping_function(idx_gpio,
										chained_list_mapping, 
										mask_gpio_current_interrupts,
										mask_gpio_value,
										true);
			}
			else{
				find_and_call_mapping_function(idx_gpio,
										chained_list_mapping, 
										mask_gpio_current_interrupts,
										previous_mask_gpio_value,
										false);
			}
		}
	}

	return 0;
}

