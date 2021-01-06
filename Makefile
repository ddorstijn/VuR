src = $(wildcard src/*.c)
obj = $(src:.c=.o)

CFLAGS = -g
LDFLAGS = -lvulkan -lglfw 

VuR: $(obj)
	$(CC) -g -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(obj) VuR

