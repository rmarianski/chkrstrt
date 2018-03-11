P=chkrstrt
OBJECTS=$(P).o
CFLAGS = -g -Wall -std=c11 -pedantic -I$(HOME)/opt/include -O3
LDLIBS =

$(P): $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(P)

.PHONY: clean
