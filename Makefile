CFLAGS = -Wall -g
all: page
page: page.o
	gcc $(CFLAGS) page.o -o page
page.o: page.c
	gcc $(CFLAGS) -c page.c
clean:
	rm -f *.o page