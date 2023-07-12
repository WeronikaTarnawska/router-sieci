CC = gcc -g -std=c99
CFLAGS = -Wall -Wextra -Og
SRC = router.c
OBJ = router.o

router: router.o udp.o
	$(CC) $^ -o $@

router.o: router.c
	$(CC) -c $(CFLAGS) $^ -o $@

udp.o: udp.c
	$(CC) -c $(CFLAGS) $^ -o $@

clean:
	rm -f *.o

distclean:
	rm -f *.o router 
