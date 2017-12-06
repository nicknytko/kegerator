CC=gcc
CFLAGS=
LD=gcc
LDFLAGS=-lpigpio -lrt

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=kegerator

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(EXECUTABLE) $(OBJECTS)

clean:
	rm $(OBJECTS)
	rm $(EXECUTABLE)
