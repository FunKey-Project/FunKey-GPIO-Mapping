TOOLS_CFLAGS	:= -Wall -std=c99 -D _DEFAULT_SOURCE
#
# Programs
#
all: funkey_gpio_management termfix

funkey_gpio_management: funkey_gpio_management.o gpio-utils.o uinput.o gpio_mapping.o read_conf_file.o keydefs.o driver_pcal6416a.o driver_axp209.o smbus.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

termfix: termfix.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#
# Objects
#

%.o: %.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c $< -o $@

clean:
	rm *.o funkey_gpio_management 
