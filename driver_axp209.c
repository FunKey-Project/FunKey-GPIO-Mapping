#include <stdio.h>
#include <stdint.h>
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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "smbus.h"
#include "driver_axp209.h"

/****************************************************************
 * Defines
 ****************************************************************/
#define DEBUG_AXP209_PRINTF 			(0)

#if (DEBUG_AXP209_PRINTF)
	#define DEBUG_PRINTF(...) printf(__VA_ARGS__);
#else
	#define DEBUG_PRINTF(...)
#endif

/****************************************************************
 * Static variables
 ****************************************************************/
static int fd_axp209;
static char i2c0_sysfs_filename[] = "/dev/i2c-0";

/****************************************************************
 * Static functions
 ****************************************************************/

/****************************************************************
 * Public functions
 ****************************************************************/
bool axp209_init(void) {
    if ((fd_axp209 = open(i2c0_sysfs_filename,O_RDWR)) < 0) {
        printf("In axp209_init - Failed to open the I2C bus %s", i2c0_sysfs_filename);
        // ERROR HANDLING; you can check errno to see what went wrong 
        return false;
    }

    if (ioctl(fd_axp209, I2C_SLAVE, AXP209_I2C_ADDR) < 0) {
        printf("In axp209_init - Failed to acquire bus access and/or talk to slave.\n");
        // ERROR HANDLING; you can check errno to see what went wrong 
        return false;
    }	

    // Enable only chosen interrupts (PEK short and long presses)
    int err;
    err = i2c_smbus_write_byte_data(fd_axp209 , AXP209_INTERRUPT_BANK_1_ENABLE, 0x00);
    if(err < 0){
        printf("ERROR initializing interrupts 1 for AXP209\n");
    }
    err = i2c_smbus_write_byte_data(fd_axp209 , AXP209_INTERRUPT_BANK_2_ENABLE, 0x00);
    if(err < 0){
        printf("ERROR initializing interrupts 2 for AXP209\n");
    }
    err = i2c_smbus_write_byte_data(fd_axp209 , AXP209_INTERRUPT_BANK_3_ENABLE, 0x03);
    if(err < 0){
        printf("ERROR initializing interrupts 3 for AXP209\n");
    }
    err = i2c_smbus_write_byte_data(fd_axp209 , AXP209_INTERRUPT_BANK_4_ENABLE, 0x00);
    if(err < 0){
        printf("ERROR initializing interrupts 4 for AXP209\n");
    }
    err = i2c_smbus_write_byte_data(fd_axp209 , AXP209_INTERRUPT_BANK_5_ENABLE, 0x00);
    if(err < 0){
        printf("ERROR initializing interrupts 5 for AXP209\n");
    }

 	return true;
}

bool axp209_deinit(void) {
    // Close I2C open interface
	close(fd_axp209);
 	return true;
}

int axp209_read_interrupt_bank_3(void){
	int val = i2c_smbus_read_byte_data ( fd_axp209 , AXP209_INTERRUPT_BANK_3_STATUS );
    if(val < 0){
        return val;
    }

    // Clear interrupts
    int err = i2c_smbus_write_byte_data(fd_axp209 , AXP209_INTERRUPT_BANK_3_STATUS, 0xFF);
    if(err < 0){
        return err;
    }

 	DEBUG_PRINTF("READ AXP209_INTERRUPT_BANK_3_STATUS:  0x%02X\n",val);
 	return 0xFF & val;
}
