# Makefile, versao 4
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CC       = gcc
CFLAGS   = -g -std=gnu99 -Wall -pedantic -pthread

.PHONY: all clean zip

all: heatSim_p4

heatSim_p4: p4_main.o matrix2d.o util.o
	$(CC) $(CFLAGS) -o $@ $+

p4_main.o: p4_main.c matrix2d.h util.h
	$(CC) $(CFLAGS) -o $@ -c $<

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -o $@ -c $<

util.o: util.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o heatSim_p3

zip: heatSim_p4_solucao.zip

heatSim_p4_solucao.zip: Makefile p4_main.c matrix2d.h util.h matrix2d.c matrix2d.h util.c
	zip $@ $+

run:
	./heatSim_p4 20 10 10 10 10 10 20 0.5 salvaguarda 2

