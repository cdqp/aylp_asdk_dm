# Makefile for dpg0101 utility program
CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld

CFLAGS = -Wall

TARGETS = dpg0101
OBJS = dpgutil.o display_text.o file.o conf.o cardutil.o

all: $(TARGETS)

$(TARGETS): $(OBJS)
	$(CC) -o $(TARGETS) $(OBJS) -lgpgconf

clean:
	rm -f *.o *~ $(TARGETS)
