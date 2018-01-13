CC=gcc
CFLAGS=-std=c11 -g
LD=gcc
LDFLAGS=-lpthread
ARCH=$(shell uname -m)

# Compiling for Desktop
ifeq ($(ARCH),x86_64)
$(info Compiling test version for desktop)
CFLAGS += -DTARGET_DESKTOP
endif

#Compiling for RPI
ifeq ($(ARCH),armv7l)
$(info Compiling for Rapsberry Pi)
CFLAGS += -DTARGET_RPI
LDFLAGS += -lpigpio -lrt
endif

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=kegerator

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(EXECUTABLE) $(OBJECTS)

clean:
	rm $(OBJECTS)
	rm $(EXECUTABLE)
