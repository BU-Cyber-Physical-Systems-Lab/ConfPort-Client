CROSS_COMPILE?=aarch64-linux-gnu-
CC?=$(CROSS_COMPILE)gcc
PGMS=confport_client

SRCS=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)
CFLAGS+=-O2 -Wall -g

all: $(PGMS)
%.o:%.c %.h
	$(CC) $(CFLAGS) -c $<
confport_client: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@
clean:
	@rm -f $(OBJS) $(PGMS)
