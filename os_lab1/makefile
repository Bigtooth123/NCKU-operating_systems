CC := gcc
override CFLAGS += -O3 -Wall

SOURCE1 := sender.c
BINARY1 := sender

SOURCE2 := receiver.c
BINARY2 := receiver

all: $(BINARY1) $(BINARY2)

$(BINARY1): $(SOURCE1) $(patsubst %.c, %.h, $(SOURCE1))
	$(CC) $(CFLAGS) $< -o $@

$(BINARY2): $(SOURCE2) $(patsubst %.c, %.h, $(SOURCE2))
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(BINARY1) $(BINARY2)