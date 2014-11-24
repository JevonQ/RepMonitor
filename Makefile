# the compiler: cc for C program
CC = cc

# compiler flags:
# -g	adds debugging information to the executable file
#
CFLAGS = -g

# the build target executable:
default: repmon

# All the object files:
#
OBJS = repmon.o utils.o rconf.o rlog.o rsynclog.o rservice.o netstat.o

# To create the executable file testrconf we need the object
# files utils.o rconf.o test.o:
#
repmon: $(OBJS) 
	$(CC) $(CFLAGS) -o repmon $(OBJS) -lkstat

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	rm -f repmon *.o core
