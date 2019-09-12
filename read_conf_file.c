#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/input.h>
#include "read_conf_file.h"
#include <assert.h>

/****************************************************************
 * Defines
 ****************************************************************/
#define die(str, args...) do { \
        perror(str); \
        return(EXIT_FAILURE); \
    } while(0)

#define DEBUG_READ_CONF_FILE_PRINTF 			(0)

#if (DEBUG_READ_CONF_FILE_PRINTF)
	#define READ_CONF_FILE_PRINTF(...) printf(__VA_ARGS__);
#else
	#define READ_CONF_FILE_PRINTF(...)
#endif


/****************************************************************
 * Extern variables
 ****************************************************************/
extern key_names_s key_names[];


/****************************************************************
 * Static variables
 ****************************************************************/
static STRUCT_MAPPED_GPIO * head_chained_list_mapping_gpio = NULL;
static int count_valid_gpios = 0;
static int gpio_pins[MAX_NUM_GPIO];


/****************************************************************
 * Static functions
 ****************************************************************/
/* Return linux Key code from corresponding str*/
static int find_key(const char *name)
{
	int i=0;
	while(key_names[i].code >= 0){
		if(!strncmp(key_names[i].name, name, 32))break;
		i++;
	}
	if(key_names[i].code < 0){
		i = 0;
	}
	return i;
}

/* Return pin idx in the array of instanciated pins  */
static int get_idx_pin(int* ptr_list_gpio_pins, int size_list_gpio_pins, int pin_number_to_find){
	int idx;

	for (idx=0; idx<size_list_gpio_pins; idx++){
		if(ptr_list_gpio_pins[idx] == pin_number_to_find){
			return idx;
		}
	}

	// if we get here, then we did not find the pin:
	idx=-1;
	return idx;
}


/* function to swap data of two nodes a and b in chained list */
static void swap(STRUCT_MAPPED_GPIO *a, STRUCT_MAPPED_GPIO *b)
{
    STRUCT_MAPPED_GPIO temp;
    memcpy(&temp, a, sizeof(STRUCT_MAPPED_GPIO));
    memcpy(a, b, sizeof(STRUCT_MAPPED_GPIO));
    memcpy(b, &temp, sizeof(STRUCT_MAPPED_GPIO));
    b->next_mapped_gpio = a->next_mapped_gpio;
    a->next_mapped_gpio = b;
}


/* Bubble sort the given linked list */
static void bubbleSort(STRUCT_MAPPED_GPIO *head)
{
    int swapped, i;
    STRUCT_MAPPED_GPIO *ptr1 = head;
    STRUCT_MAPPED_GPIO *lptr = NULL;
 
    /* Checking for empty list */
    if (ptr1 == NULL)
        return;
 
    do
    {
        swapped = 0;
        ptr1 = head;
 
        while (ptr1->next_mapped_gpio != lptr)
        {
            if (ptr1->nb_simultaneous_pins < ptr1->next_mapped_gpio->nb_simultaneous_pins)
            { 
                swap(ptr1, ptr1->next_mapped_gpio);
                swapped = 1;
            }
            ptr1 = ptr1->next_mapped_gpio;
        }
        lptr = ptr1;
    }
    while (swapped);
}


/* Push new node in chained list */
static void push(STRUCT_MAPPED_GPIO ** head, STRUCT_MAPPED_GPIO * new_mapping) {
	STRUCT_MAPPED_GPIO ** current = head;
    while (*current != NULL) {
        current = &(*current)->next_mapped_gpio;
    }

    /* now we can add a new variable */
    *current = malloc(sizeof(STRUCT_MAPPED_GPIO));
    memcpy(*current, new_mapping, sizeof(STRUCT_MAPPED_GPIO));
    (*current)->next_mapped_gpio = NULL;
}



/****************************************************************
 * Public functions
 ****************************************************************/
 void get_mapping_from_conf_file(STRUCT_MAPPED_GPIO ** chained_list_mapping, 
 	int* nb_valid_gpios, int ** valid_gpio_pins){
 	/* Variables */
	READ_CONF_FILE_PRINTF("Reading the config file:\n");
	FILE *fp;
	char ln[MAX_LN_LENGTH];
	int lnno = 0;
	char conffile[80];
	bool pins_declaration_line = true;

  	/* search for the conf file in ~/.funkey_gpio_mapping.conf and /etc/funkey_gpio_mapping.conf */
	sprintf(conffile, "%s/.funkey_gpio_mapping.conf", getenv("HOME"));
	fp = fopen(conffile, "r");
	if(!fp){
		fp = fopen("/etc/funkey_gpio_mapping.conf", "r");
		if(!fp){
			fp = fopen("./funkey_gpio_mapping.conf", "r");
			if(!fp){
				perror(conffile);
				perror("/etc/funkey_gpio_mapping.conf");
				perror("./funkey_gpio_mapping.conf");
			}
			sprintf(conffile, "./funkey_gpio_mapping.conf");
		}
		else{
			sprintf(conffile, "/etc/funkey_gpio_mapping.conf");
		}
	}
	if(fp){
		READ_CONF_FILE_PRINTF("Config file found at %s\n", conffile);
	}

	/* Main loop to read conf file (purposely exploded and not in multiple sub-functions) */
	while(fp && !feof(fp)){
		if(fgets(ln, MAX_LN_LENGTH, fp)){
			lnno++;
			if(strlen(ln) > 1){
				if(ln[0]=='#')continue;
				READ_CONF_FILE_PRINTF("\nCurrent line : %s\n", ln);

				// ************ Pins declaration ************
				if(pins_declaration_line){
					char* token;
					int token_int;
					
					//count nb valid GPIOs and store them
					token = strtok(ln, ",");
					while(token != NULL){
						token_int = atoi(token);
						if(token_int || !strcmp(token, "0")){
							gpio_pins[count_valid_gpios] = token_int;
							count_valid_gpios++;
							READ_CONF_FILE_PRINTF("GPIO %d declared\n", token_int);
						}
						else{
							READ_CONF_FILE_PRINTF("Could not declare GPIO: %s\n", token);
						}
						token = strtok(NULL, ",");
					}
					pins_declaration_line = false;
				}

				// ************ Pins mapping ************
				else{
					STRUCT_MAPPED_GPIO cur_gpio_mapping;
					char* end_ln;
					char* token_comma;
					char * current_arg;
					int cur_arg_idx = MAPPING_ARG_PINS;
					int idx_pin = 0;
					int cur_pin;
					bool add_current_mapping = true;

					token_comma = strtok_r(ln, ",", &end_ln);
					while(token_comma != NULL && cur_arg_idx < NB_MAPPING_ARG_NAMES && add_current_mapping){
						current_arg= token_comma;
						//remove_initial_spaces: 
						while(*current_arg==' ' && *current_arg!=0){current_arg++;}
						READ_CONF_FILE_PRINTF("	Current arg: %s\n", current_arg);
						//Handle current mapping arg
						switch(cur_arg_idx){
							case MAPPING_ARG_PINS:
								//count nb valid GPIOs and store them
								;char* end_arg;
								char * token_plus = strtok_r(current_arg, "+", &end_arg);
								while(token_plus != NULL){
									cur_pin = atoi(token_plus);
									if(cur_pin || !strcmp(token_plus, "0")){
										int idx_cur_pin = get_idx_pin(gpio_pins, count_valid_gpios, cur_pin);
										if(idx_cur_pin == -1){
											READ_CONF_FILE_PRINTF("		Could not find GPIO: %s in previously instanciated GPIOs\n", token_plus);
											add_current_mapping = false;
											break;
										}
										cur_gpio_mapping.pins_idx[idx_pin] = idx_cur_pin;
										idx_pin++;
										READ_CONF_FILE_PRINTF("		GPIO %d in current mapping\n", cur_pin);
									}
									else{
										READ_CONF_FILE_PRINTF("		Could not find GPIO: %s\n", token_plus);
										add_current_mapping = false;
										break;
									}
									token_plus = strtok_r(NULL, "+", &end_arg);//strtok_r(NULL, ",");
								}
								cur_gpio_mapping.nb_simultaneous_pins = idx_pin;
								break;
							case MAPPING_ARG_TYPE:
								if(!strcmp(current_arg, "KEYBOARD")){
									cur_gpio_mapping.type_mapping = TYPE_MAPPING_KEYBOARD;
								}
								else if(!strcmp(current_arg, "SHELL_COMMAND")){
									cur_gpio_mapping.type_mapping = TYPE_MAPPING_SHELL_COMMAND;
								}
								else{
									READ_CONF_FILE_PRINTF("		%s is not a valid mapping type\n", current_arg);
									add_current_mapping = false;
									break;
								}
								break;
							case MAPPING_ARG_VALUE:
								switch (cur_gpio_mapping.type_mapping){
									case TYPE_MAPPING_KEYBOARD:
										cur_gpio_mapping.key_value = find_key(current_arg);
										if(!cur_gpio_mapping.key_value){
											READ_CONF_FILE_PRINTF("		Could not find Key: %s\n", current_arg);
											add_current_mapping = false;
										}
										break;
									case TYPE_MAPPING_SHELL_COMMAND:
										cur_gpio_mapping.shell_command = malloc(strlen(current_arg)*sizeof(char));
										strcpy(cur_gpio_mapping.shell_command, current_arg);
										break;
									default:
										READ_CONF_FILE_PRINTF("		%d is not a valid mapping type\n", cur_gpio_mapping.type_mapping);
										add_current_mapping = false;
										break;
								}
								break;
							case MAPPING_ARG_STR_HELP_PIN_NAME:
								if(!strlen(current_arg)) continue;
								cur_gpio_mapping.pin_help_str = malloc(strlen(current_arg)*sizeof(char));
								strcpy(cur_gpio_mapping.pin_help_str, current_arg);
								break;
							case MAPPING_ARG_STR_HELP_PIN_FCT:
								if(!strlen(current_arg)) continue;
								cur_gpio_mapping.fct_help_str = malloc(strlen(current_arg)*sizeof(char));
								strcpy(cur_gpio_mapping.fct_help_str, current_arg);
								break;
							default:
								break;
						}


						//next arg
						token_comma = strtok_r(NULL, ",", &end_ln);
						cur_arg_idx++;
					}

					if(add_current_mapping){
						push(&head_chained_list_mapping_gpio, &cur_gpio_mapping);
						READ_CONF_FILE_PRINTF("Current Mapping added successfully\n\n");
					}
					else{
						READ_CONF_FILE_PRINTF("Current Mapping not added\n\n");
					}
				}
			}
		}
	}

	READ_CONF_FILE_PRINTF("Sorting Chained list...\n");
	bubbleSort(head_chained_list_mapping_gpio);
	print_all_chained_list(head_chained_list_mapping_gpio);
	print_gpios(gpio_pins, count_valid_gpios);

	if(fp){
		fclose(fp);
	}

	/* Return parameters */ 
	*chained_list_mapping = head_chained_list_mapping_gpio;
	*nb_valid_gpios = count_valid_gpios;
	*valid_gpio_pins = gpio_pins;
 }


/* Print all chained list content */
void print_all_chained_list(STRUCT_MAPPED_GPIO * head){
    STRUCT_MAPPED_GPIO * current = head;
    int idx = 0;

	READ_CONF_FILE_PRINTF("Printing Chained list:\n ");
    while (current != NULL) {
        READ_CONF_FILE_PRINTF("Chained list Mapping GPIOs, idx: %d\n", idx);
        
        print_chained_list_node(current);

        idx++;
        current = current->next_mapped_gpio;
    }
}


/* Print one chained list node */
void print_chained_list_node(STRUCT_MAPPED_GPIO * node){
    // Print Pins:
   	if(node->nb_simultaneous_pins > 1){
	    READ_CONF_FILE_PRINTF("		%d Simultaneous pins at following idx: [", node->nb_simultaneous_pins);
	    int i;
	    for (i=0; i < node->nb_simultaneous_pins; i++){
	       	READ_CONF_FILE_PRINTF("%d%s", node->pins_idx[i], (i==node->nb_simultaneous_pins-1)?"]\n":",");
	    }
    }
    else{
       	READ_CONF_FILE_PRINTF("		idx pin: %d\n", node->pins_idx[0]);
    }

    // Type mapping and Value
    if (node->type_mapping == TYPE_MAPPING_KEYBOARD){
        READ_CONF_FILE_PRINTF("		Type mapping: TYPE_MAPPING_KEYBOARD\n");
        READ_CONF_FILE_PRINTF("		Key: %d\n", node->key_value);
   }
    else if (node->type_mapping == TYPE_MAPPING_SHELL_COMMAND){
        READ_CONF_FILE_PRINTF("		Type mapping: TYPE_MAPPING_SHELL_COMMAND\n");
		READ_CONF_FILE_PRINTF("		Shell command: %s\n", node->shell_command);
    }

    // Pin help String
    if(node->pin_help_str[0]){
        READ_CONF_FILE_PRINTF("		Pins: %s\n", node->pin_help_str);
   	}

    // Function help String
    if(node->fct_help_str[0]){
        READ_CONF_FILE_PRINTF("		Function: %s\n", node->fct_help_str);
    }
}

void print_gpios(int * valid_gpio_pins, int nb_valid_gpios){
	READ_CONF_FILE_PRINTF("GPIO pins:\n[");
	for(int i = 0; i< nb_valid_gpios; i++) {
		READ_CONF_FILE_PRINTF("%d%s", valid_gpio_pins[i], (i==nb_valid_gpios-1)?"]\n":",");
	}
}