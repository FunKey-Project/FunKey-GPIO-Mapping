TOOLS_CFLAGS	:= -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual \
		   -Wcast-align -Wwrite-strings -Wnested-externs -Winline \
		   -W -Wundef -Wmissing-prototypes
#
# Programs
#
all:	funkey_gpio_management 

funkey_gpio_management:  funkey_gpio_management.o gpio-utils.o uinput.o gpio_mapping.o read_conf_file.o keydefs.o driver_pcal6416a.o
	$(CC) $(LDFLAGS) -o $@ $^


#
# Objects
#

%.o: %.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c $< -o $@

clean:
	rm *.o funkey_gpio_management 
