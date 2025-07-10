SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
CC = aarch64-linux-gnu-gcc


firmware_burn: $(OBJS)
	$(CC) -o $@ $^ 
	sshpass -p 'temppwd' scp $@ cat@192.168.103.145:~

%.o:%.c
	$(CC) -c -o $@ $<


clean:
	rm -rf *.o firmware_burn
