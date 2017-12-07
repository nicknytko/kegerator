CC=gcc
CFLAGS=-std=c11
LD=gcc
LDFLAGS=-lpigpio -lrt -lpthread

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=kegerator

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(EXECUTABLE) $(OBJECTS)

clean:
	rm $(OBJECTS)
	rm $(EXECUTABLE)
