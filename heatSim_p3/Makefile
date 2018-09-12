# Makefile, versao 2
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS= -g -Wall -pedantic
CC=gcc

all: heatSim_p3

heatSim_p3: p3_main.o matrix2d.o
	$(CC) $(CFLAGS) -pthread -o heatSim_p3 p3_main.o matrix2d.o

p3_main.o: p3_main.c
	$(CC) $(CFLAGS) -c p3_main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c

clean:
	rm -f *.o heatSim_p3

zip:
	zip heatSim_p3_solucao.zip p3_main.c matrix2d.c matrix2d.h Makefile

run:
	./heatSim_p3 10 10.0 10.0 10 10 1000 10 0.40
