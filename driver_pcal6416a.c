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
#include "driver_pcal6416a.h"

/****************************************************************
 * Defines
 ****************************************************************/
#define DEBUG_PCAL6416A_PRINTF 			(0)

#if (DEBUG_PCAL6416A_PRINTF)
	#define DEBUG_PRINTF(...) printf(__VA_ARGS__);
#else
	#define DEBUG_PRINTF(...)
#endif

/****************************************************************
 * Static variables
 ****************************************************************/
static int fd_i2c_expander;
static unsigned int i2c_expander_addr;
static char i2c0_sysfs_filename[] = "/dev/i2c-0";

/****************************************************************
 * Static functions
 ****************************************************************/

/****************************************************************
 * Public functions
 ****************************************************************/
bool pcal6416a_init(void) {
    if ((fd_i2c_expander = open(i2c0_sysfs_filename,O_RDWR)) < 0) {
        printf("Failed to open the I2C bus %s", i2c0_sysfs_filename);
        // ERROR HANDLING; you can check errno to see what went wrong 
        return false;
    }

    i2c_expander_addr = 0;

    /// Probing PCAL9539A chip
    if (ioctl(fd_i2c_expander, I2C_SLAVE_FORCE, PCAL9539A_I2C_ADDR) < 0 ||
	pcal6416a_read_mask_interrupts() < 0) {
        printf("In %s - Failed to acquire bus access and/or talk to slave PCAL9539A_I2C_ADDR 0x%02X.\n",
            __func__, PCAL9539A_I2C_ADDR);

	/// Probing PCAL6416A chip
	if (ioctl(fd_i2c_expander, I2C_SLAVE_FORCE, PCAL6416A_I2C_ADDR) < 0 ||
	    pcal6416a_read_mask_interrupts() < 0) {
	    printf("In %s - Failed to acquire bus access and/or talk to slave PCAL6416A_I2C_ADDR 0x%02X.\n",
	        __func__, PCAL6416A_I2C_ADDR);
	} else {
	    DEBUG_PRINTF("Found I2C gpio expander chip: PCAL6416A\n");
	    i2c_expander_addr = PCAL6416A_I2C_ADDR;
	}
    } else{
        DEBUG_PRINTF("Found I2C gpio expander chip: PCAL9539A\n");
        i2c_expander_addr = PCAL9539A_I2C_ADDR;
    }

    /// GPIO expander chip found?
    if(!i2c_expander_addr){
        printf("In %s - Failed to acquire bus access and/or talk to slave, exit\n", __func__);
        return false;
    }

    uint16_t val_enable_direction = 0xffff;
    i2c_smbus_write_word_data ( fd_i2c_expander , PCAL6416A_CONFIG , val_enable_direction );

    uint16_t val_enable_latch = 0x0000;
    i2c_smbus_write_word_data ( fd_i2c_expander , PCAL6416A_INPUT_LATCH , val_enable_latch );

    uint16_t val_enable_pullups = 0xffff;
    i2c_smbus_write_word_data ( fd_i2c_expander , PCAL6416A_EN_PULLUPDOWN , val_enable_pullups );

    uint16_t val_sel_pullups = 0xffff;
    i2c_smbus_write_word_data ( fd_i2c_expander , PCAL6416A_SEL_PULLUPDOWN , val_sel_pullups );

    //uint16_t val_enable_interrupts = 0x0000;
    uint16_t val_enable_interrupts = 0x0320;
 	i2c_smbus_write_word_data ( fd_i2c_expander , PCAL6416A_INT_MASK , val_enable_interrupts );

 	return true;
}

bool pcal6416a_deinit(void) {
    // Close I2C open interface
	close(fd_i2c_expander);
 	return true;
}

int pcal6416a_read_mask_interrupts(void){
	int val_int = i2c_smbus_read_word_data ( fd_i2c_expander , PCAL6416A_INT_STATUS );
    if(val_int < 0){
        return val_int;
    }
    uint16_t val = val_int & 0xFFFF;
 	DEBUG_PRINTF("READ PCAL6416A_INT_STATUS :  0x%04X\n",val);
 	return (int) val;
}

int pcal6416a_read_mask_active_GPIOs(void){
	int val_int = i2c_smbus_read_word_data ( fd_i2c_expander , PCAL6416A_INPUT );
    if(val_int < 0){
        return val_int;
    }
    uint16_t val = val_int & 0xFFFF;
	val = 0xFFFF-val;
 	DEBUG_PRINTF("READ PCAL6416A_INPUT (active GPIOs) :  0x%04X\n",val);
 	return (int) val;
}
