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
OBJS = repmon.o utils.o rconf.o rlog.o

# To create the executable file testrconf we need the object
# files utils.o rconf.o test.o:
#
repmon: $(OBJS) 
	$(CC) $(CFLAGS) -o repmon $(OBJS)

# To create the object file test.o, we need the source files
# test.c
#
repmon.o: repmon.c repmon.h rconf.h utils.h rlog.h
	$(CC) $(CFLAGS) -c repmon.c

# To create the object file rconf.o, we need the source files
# rconf.c, rconf.h
#
rconf.o: rconf.c rconf.h utils.h
	$(CC) $(CFLAGS) -c rconf.c

# To create the object file utils.o, we need the source files
# utils.c, utils.h
#
utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# To create the object file rlog.o, we need the source files
# rlog.c and rlog.h
#
rlog.o: rlog.c rlog.h
	$(CC) $(CFLAGS) -c rlog.c

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	rm -f repmon *.o core
