#
# iniparser tests Makefile
#

CC      = gcc -O2
CFLAGS  = -g -I../src
LFLAGS  = -L.. -liniparser
AR	    = ar
ARFLAGS = rcv
RM      = rm -f


default: all

all: iniexample parse bench

iniexample: iniexample.c
	$(CC) $(CFLAGS) -o iniexample iniexample.c -I../src -L.. -liniparser

parse: parse.c
	$(CC) $(CFLAGS) -o parse parse.c -I../src -L.. -liniparser

bench: bench.c
	$(CC) $(CFLAGS) -o bench bench.c -I../src -L.. -liniparser

clean veryclean:
	$(RM) iniexample example.ini parse bench bench.ini



