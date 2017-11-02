CFLAGS = -Wall -Werror -pthread -g -std=c99
CC = mpicc

main: main.c atomic.o fixed_list.o messenger_thread.o queue.o simulation.o
	$(CC) main.c atomic.o fixed_list.o messenger_thread.o \
	 queue.o simulation.o -o main $(CFLAGS)

atomic.o: atomic.c atomic.h
	$(CC) -c atomic.c $(CFLAGS)

fixed_list.o: fixed_list.c fixed_list.h
	$(CC) -c fixed_list.c $(CFLAGS)

messenger_thread.o: messenger_thread.c messenger_thread.h queue.h simulation.h atomic.h
	$(CC) -c messenger_thread.c $(CFLAGS)

queue.o: queue.c queue.h atomic.h
	$(CC) -c queue.c $(CFLAGS)

simulation.o: simulation.c simulation.h fixed_list.h messenger_thread.h atomic.h
	$(CC) -c simulation.c $(CFLAGS)

clean:
	rm -rf tests *.o *.gcov *.dSYM *.gcda *.gcno *.swp