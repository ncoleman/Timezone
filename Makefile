CC = cc
CFLAGS = -Wall -O2 -c
LDFLAGS =
SOURCES = tz.c timezones.c
EXECUTABLE = tz
OBJECTS = $(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o: 
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)
