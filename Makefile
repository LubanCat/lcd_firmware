SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -fcommon

firmware_burn: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
# 	sshpass -p 'temppwd' scp $@ cat@192.168.103.145:~

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o firmware_burn
