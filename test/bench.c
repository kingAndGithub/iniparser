#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "iniparser.h"

double epoch_double()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + (t.tv_usec * 1.0) / 1000000.0;
}

void stop_timer(char *s, double t1)
{
    double t2 = epoch_double();
    printf("%15s: %f\n", s, t2 - t1);
}

#ifndef BENCHSIZE
#define BENCHSIZE 256
#endif

int main(int argc, char * argv[])
{
    dictionary * ini ;
    char       * ini_name ;
    int          i, j ;
    FILE       * f;
    double       t1;
    char       * secs;
    char       * keys;

	if (argc<2) {
        ini_name = "bench.ini";
	} else {
		ini_name = argv[1] ;
	}

    printf("Starting benchmark with size %d^2\n", BENCHSIZE);

    unlink(ini_name);
    f = fopen(ini_name, "w");
    fclose(f);

    ini = iniparser_load(ini_name);

    secs = malloc(12 * BENCHSIZE);
    keys = malloc(12 * BENCHSIZE);
    for(i = 0 ; i < BENCHSIZE ; i++) {
        sprintf(secs + 12 * i, "sec%08x", i);
        sprintf(keys + 12 * i, "key%08x", i);
    }

    t1 = epoch_double();
    for(i = 0 ; i < BENCHSIZE ; i++) {
        iniparser_set(ini, secs + 12 * i, "1");
        for(j = 0 ; j < BENCHSIZE ; j++) {
            char buffer[64];
            memcpy(buffer, secs + 12 * i, 11);
            buffer[11] = ':';
            memcpy(buffer + 12, keys + 12 * j, 12);
            iniparser_set(ini, buffer, "1");
        }
    }
    stop_timer("Adding", t1);

    if(!(f = fopen(ini_name, "w"))) {
        exit(-1);
    }

    t1 = epoch_double();
    iniparser_dump_ini(ini, f);
    stop_timer("Saving", t1);
    fclose(f);

    dictionary_del(ini);

    t1 = epoch_double();
    ini = iniparser_load(ini_name);
    stop_timer("Loading", t1);

    t1 = epoch_double();
    for(i = 0 ; i < BENCHSIZE ; i++) {
        iniparser_set(ini, secs + 12 * i, "1");
        for(j = 0 ; j < BENCHSIZE ; j++) {
            char buffer[64];
            memcpy(buffer, secs + 12 * i, 11);
            buffer[11] = ':';
            memcpy(buffer + 12, keys + 12 * j, 12);
            iniparser_getstring(ini, buffer, NULL);
        }
    }
    stop_timer("Getting", t1);

    dictionary_del(ini);

	return 0 ;
}
