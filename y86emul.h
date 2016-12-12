/*
    Jesse-James Black
    171006034
    Computer Arch Russell
    y86 Emulation
*/

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#ifndef Y86EMUL_H
#define Y86EMUL_H

void noop();
void getMem(char *y86);
void executeprog();
char *concat(char *str, char c);
int hextodec(char *num);
char *hextobin(char c);
int getbytes(char *str, int position);
void printstatus();
void getmemspace(unsigned char *arg1, unsigned char *arg2);

typedef enum {
    AOK,
    HLT,
    ADR,
    INS

} ProgramStatus;

union storage {
    int integer;
    char byte[8];
};

#endif
