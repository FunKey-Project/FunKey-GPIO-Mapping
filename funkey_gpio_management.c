#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include <sys/select.h>
#include "gpio-utils.h"
#include <assert.h>
#include "uinput.h"
#include "gpio_mapping.h"
#include "read_conf_file.h"

 /****************************************************************
 * Defines
 ****************************************************************/
 
/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;	// Set to 0 when ctrl-c is pressed

/****************************************************************
 * signal_handler
 ****************************************************************/
void signal_handler(int sig);
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}

/****************************************************************
 * Local functions
 ****************************************************************/


/****************************************************************
 * Main
 ****************************************************************/
int main(int argc, char **argv, char **envp)
{
	// Variables
	STRUCT_MAPPED_GPIO * chained_list_mapping = NULL;
	int nb_valid_gpios = 0;
	int * gpio_pins_idx_declared = NULL;
	bool * gpios_pins_active_high = NULL;

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);
	
	// Init uinput device
	init_uinput();

	// Read Conf File: Get GPIO pins to declare and all valid pin mappings
	get_mapping_from_conf_file(&chained_list_mapping, &nb_valid_gpios, &gpio_pins_idx_declared, &gpios_pins_active_high);

	// Init GPIOs 
	init_mapping_gpios(gpio_pins_idx_declared, gpios_pins_active_high, nb_valid_gpios, chained_list_mapping);
	
	// Main Loop
	while (keepgoing) {
		listen_gpios_interrupts();
	}

	// De-Init GPIOs 
	deinit_mapping_gpios();
	
	/*
    * Give userspace some time to read the events before we destroy the
    * device with UI_DEV_DESTOY.
    */
	close_uinput();
    
	return 0;
}

