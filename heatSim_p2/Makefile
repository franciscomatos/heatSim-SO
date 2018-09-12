# Makefile, versao 2
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS= -g -Wall -pedantic 
CC=gcc

all: heatSim_p1 

heatSim_p1: main.o matrix2d.o mplib3.o leQueue.o leQueue.h mplib3.h 
	$(CC) $(CFLAGS) -pthread -o heatSim_p1 main.o matrix2d.o mplib3.o leQueue.o

main.o: main.c matrix2d.h
	$(CC) $(CFLAGS) -c main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c

leQueue.o: leQueue.c leQueue.h
	$(CC) $(CFLAGS) -c leQueue.c

mplib3.o: mplib3.c mplib3.h leQueue.h
	$(CC) $(CFLAGS) -c mplib3.c

clean:
	rm -f *.o heatSim_p1 

zip:
	zip heatSim_p1_solucao.zip main.c matrix2d.c matrix2d.h leQueue.c leQueue.h mplib3.c mplib3.h Makefile

run:
	./heatSim_p1 10 10.0 10.0 0.0 0.0 10 2 0
