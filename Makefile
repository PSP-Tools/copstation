OUTPUT=popstation
OBJS=main.o toc.o iniparser.o
CFLAGS=-Wall -Os -I.
LDFLAGS=-L.
LIBS = -lz -lm

all: $(OUTPUT)

clean:
	rm -f $(OUTPUT) *.o

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)
