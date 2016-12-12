/*
    Jesse-James Black
    171006034
    Computer Arch Russell
    y86 Emulation
*/

#include "y86emul.h"
//#include "mallocld.h"

//global variables for registers, memory, and flags
int gReg[8] = {0};
unsigned char *gMemspace;
int gPC, gMemSize, gOF = 0, gZF = 0, gSF = 0;

//the current program status, defaults to okay
ProgramStatus status = AOK;
//char gStatus = "AOK"
/*
    main is used to check arguments, determine time required and execute functions needed
*/
int main(int argc, char **argv)
{
    //exectution time
    struct timeval start, end;
    gettimeofday(&start, NULL);

    //two argument checks
    //if arguments are not two
    if (argc != 2)
    {
        fprintf(stderr, "%s\n", "Error: Incorrect amount of arguments.");
        return 0;
    }
    //if -h is found
    if (strcmp(argv[1], "-h") == 0)
    {
        fprintf(stderr, "%s\n", "Usage: y86emul <y86 instruction file>");
        return 0;
    }

    //get memory and then execute
    getMem(argv[1]);
    executeprog();

    //free the memeory
    free(gMemspace);

    //print execution time
    gettimeofday(&end, NULL);
    printf("Total execution time: %ld\n",
           ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));

    return 0;
}
/*
    Used to gather all the memory from the input file and sort it
*/
void getMem(char *y86)
{
    int length, size, count, i = 0, j = 0, k = 0;
    char c;
    char *p = malloc(sizeof(char));
    p[0] = '\0';
    char *dup, *token, *memory, *address, *arg;

    //grab the file, y86 is argv[1] from main
    FILE *inputfile = fopen(y86, "r");

    //reject if NULL
    if (inputfile == NULL)
    {
        fprintf(stderr, "%s\n", "Error: Incorrect amount of arguments or no file.");
    }

    //get all the chars
    while (1)
    {
        c = fgetc(inputfile);

        if (feof(inputfile))
        {
            break;
        }

        p = concat(p, c);
    }
    //
    //close file
    fclose(inputfile);

    //cuplicate the memory
    dup = (char *)malloc((strlen(p) + 1) * sizeof(char));
    strcpy(dup, p);
    token = strtok(dup, "\n\t\r");
    count = 0;

    //seatch for correct directives
    while (1)
    {
        if (strcmp(token, ".size") == 0)
        {
            if (count == 0)
            {
                count++;
            }
            else
            {
                //reject if more than one size directive found
                fprintf(stderr, "%s\n", "Error: More than one .size directive.");
                return;
            }
            memory = strtok(NULL, "\n\t\r");
        }
        token = strtok(NULL, "\n\t\r");

        //or reject if NULL
        if (token == NULL)
        {
            break;
        }
    }

    //find the dec equivalant of the size
    size = hextodec(memory);

    //create memory for that size directive
    gMemSize = size;
    gMemspace = (unsigned char *)malloc((size + 1) * sizeof(unsigned char));

    //gPC is the program counter
    gPC = -1;

    //set all memory to 0 TODO Use Calloc you moron
    for (i = 0; i < size; i++)
    {
        gMemspace[i] = 0;
    }

    //free the duplocated memory
    free(dup);

    //copy once
    dup = (char *)malloc((strlen(p) + 1) * sizeof(char));
    strcpy(dup, p);
    token = strtok(dup, "\n\t\r");

    //search, convert the rest of the directives
    while (1)
    {
        //look for .size directive
        if (strcmp(token, ".size") == 0)
        {
            token = strtok(NULL, "\n\t\r");
        }
        //.text
        else if (strcmp(token, ".text") == 0)
        {
            address = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(address, (strtok(NULL, "\n\t\r")));

            arg = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(arg, (strtok(NULL, "\n\t\r")));
            k = hextodec(address);

            if (gPC == -1)
            {
                gPC = k;
            }

            j = 0;

            while (j < strlen(arg))
            {
                //create space for those
                gMemspace[k] = (unsigned char)getbytes(arg, j);
                k++;
                j += 2;
            }

            //free when done
            free(address);
            free(arg);
        }
        //look for .byte directives
        else if (strcmp(token, ".byte") == 0)
        {
            address = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(address, (strtok(NULL, "\n\t\r")));

            arg = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(arg, (strtok(NULL, "\n\t\r")));
            //create space for those
            gMemspace[hextodec(address)] = (unsigned char)hextodec(arg);

            //free when done
            free(address);
            free(arg);
        }
        //look for long directives
        else if (strcmp(token, ".long") == 0)
        {
            address = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(address, (strtok(NULL, "\n\t\r")));

            arg = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(arg, (strtok(NULL, "\n\t\r")));

            union storage storage;
            storage.integer = atoi(arg);

            for (i = 0; i < 4; i++)
            {
                gMemspace[i + hextodec(address)] = storage.byte[i];
            }

            //free
            free(address);
            free(arg);
        }
        //look for string directves
        else if (strcmp(token, ".string") == 0)
        {
            address = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(address, (strtok(NULL, "\n\t\r")));

            arg = (char *)malloc((strlen(p) + 1) * sizeof(char));
            strcpy(arg, (strtok(NULL, "\n\t\r")));

            length = strlen(arg);

            i = hextodec(address);

            for (j = 1; j < length - 1; j++)
            {
                gMemspace[i] = (unsigned char)arg[j];
                i++;
            }

            //free
            free(arg);
            free(address);
        }
        token = strtok(NULL, "\n\t");
        if (token == NULL)
        {
            break;
        }
    }

    free(dup);

    //if directive other than those allowed is found reject
    if (status == INS)
    {
        fprintf(stderr, "%s\n", "Error: Invalid directive encountered.");
        printstatus();
        return;
    }
}

/*
    Used to execute the program based on instructions given
*/
void executeprog()
{
    //arguments one and two
    unsigned char regA, regB;

    //general values
    int input, value, numberOne, numberTwo, incorrectScan, i;

    //intialize the bytes
    union storage storage;
    //default status of program status
    status = AOK;

    //used to check
    char inputchar;

    //the instructions to be exectures
    while (status == AOK)
    {
        switch (gMemspace[gPC])
        {
        // NOOP
        case 0x00:
            noop();
            break;
        //HALT
        case 0x10:
            status = HLT;
            printstatus();
            break;
        //RRMOVL
        case 0x20:
            getmemspace(&regA, &regB);
            gReg[regB] = gReg[regA];
            gPC += 2;
            break;
        //IRMOVL
        case 0x30:
            getmemspace(&regA, &regB);
            if (regA < 0x08)
            {
                status = ADR;
                printstatus();
                fprintf(stderr, "%s\n", "Error: IRMOVL instruction should have two addresses.");
                break;
            }

            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            gReg[regB] = value;

            gPC += 6;

            break;

        //RMMOVL
        case 0x40:

            getmemspace(&regA, &regB);
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            storage.integer = gReg[regA];

            if ((value + gReg[regB] + 3) > gMemSize)
            {
                status = ADR;
                printstatus();
                fprintf(stderr, "%s\n", "Error: RMMOVL instruction address offset larger than memory space.");
            }
            for (i = 0; i < 4; i++)
            {
                gMemspace[value + gReg[regB] + i] = storage.byte[i];
            }

            gPC += 6;

            break;

        //MRMOVL
        case 0x50:

            getmemspace(&regA, &regB);
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            if ((value + gReg[regB] + 3) > gMemSize)
            {
                status = ADR;
                printstatus();
                fprintf(stderr, "%s\n", "Error: MRMOVL instruction address offset larger than memory space.");
            }

            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[value + gReg[regB] + i];
            }

            gReg[regA] = storage.integer;

            gPC += 6;

            break;

        //ADDL
        case 0x60:

            gZF = 0;
            gSF = 0;
            gOF = 0;

            getmemspace(&regA, &regB);

            numberOne = gReg[regA];
            numberTwo = gReg[regB];

            value = numberOne + numberTwo;

            if (value == 0)
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            else if (value < 0)
            {
                gSF = 1;
                //fprintf(stderr, "%s\n", "Sign flag tripped");
            }

            else if ((value > 0 && numberOne < 0 && numberTwo < 0) || (value < 0 && numberOne > 0 && numberTwo > 0))
            {
                gOF = 1;
                //fprintf(stderr, "%s\n", "Overflow flag tripped");
            }

            gReg[regB] = value;

            gPC += 2;

            break;

        //SUBL
        case 0x61:

            gZF = 0;
            gSF = 0;
            gOF = 0;

            getmemspace(&regA, &regB);

            numberOne = gReg[regA];
            numberTwo = gReg[regB];

            value = numberTwo - numberOne;

            if (value == 0)
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            else if (value < 0)
            {
                gSF = 1;
                //fprintf(stderr, "%s\n", "Sign flag tripped");
            }

            else if ((value > 0 && numberOne > 0 && numberTwo < 0) || 
                    (value < 0 && numberOne < 0 && numberTwo > 0))
            {
                gOF = 1;
                //fprintf(stderr, "%s\n", "Overflow flag tripped");
            }

            gReg[regB] = value;

            gPC += 2;

            break;

        //ANDL
        case 0x62:

            gSF = 0;
            gZF = 0;

            getmemspace(&regA, &regB);

            numberOne = gReg[regA];
            numberTwo = gReg[regB];

            value = numberOne & numberTwo;

            gReg[regB] = value;

            if (value == 0)
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            else if (value < 0)
            {
                gSF = 1;
                //fprintf(stderr, "%s\n", "Sign flag tripped");
            }

            gPC += 2;

            break;

        //XORL
        case 0x63:

            gZF = 0;
            gSF = 0;

            getmemspace(&regA, &regB);

            numberOne = gReg[regA];
            numberTwo = gReg[regB];

            value = numberOne ^ numberTwo;

            gReg[regB] = value;

            if (value == 0)
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            else if (value < 0)
            {
                gSF = 1;
                //fprintf(stderr, "%s\n", "Sign flag tripped");
            }

            gPC += 2;

            break;

        //MULL
        case 0x64:

            gZF = 0;
            gSF = 0;
            gOF = 0;

            getmemspace(&regA, &regB);

            numberOne = gReg[regA];
            numberTwo = gReg[regB];

            value = numberOne * numberTwo;

            if (value == 0)
            {
                gZF = 1;
                fprintf(stderr, "%s %d\n", "Zero flag tripped: ", value);
            }

            else if (value < 0)
            {
                gSF = 1;
                fprintf(stderr, "%s %d\n", "Sign flag tripped: ", value);
            }

            else if ((value < 0 && numberOne < 0 && numberTwo < 0) ||
                     (value < 0 && numberOne > 0 && numberTwo > 0) ||
                     (value > 0 && numberOne < 0 && numberTwo > 0) ||
                     (value > 0 && numberOne > 0 && numberTwo < 0))
            {
                gOF = 1;
                fprintf(stderr, "%s %d\n", "Overflow flag tripped: ", value);
            }

            gReg[regB] = value;

            gPC += 2;

            break;

        //CMPL same as SUB? TODO
        case 0x65:
            gZF = 0;
            gSF = 0;
            gOF = 0;

            getmemspace(&regA, &regB);

            numberOne = gReg[regA];
            numberTwo = gReg[regB];

            value = numberTwo - numberOne;

            if (value == 0)
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            else if (value < 0)
            {
                gSF = 1;
                //fprintf(stderr, "%s\n", "Sign flag tripped");
            }

            else if ((value > 0 && numberOne > 0 && numberTwo < 0) || (value < 0 && numberOne < 0 && numberTwo > 0))
            {
                gOF = 1;
                //fprintf(stderr, "%s\n", "Overflow flag tripped");
            }

            gPC += 2;
            break;

        //JMP
        case 0x70:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;

            gPC = value;

            break;

        //JLE
        case 0x71:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;

            if (gZF == 1 || (gSF ^ gOF))
            {
                gPC = value;
            }
            else
            {
                gPC += 5;
            }

            break;

        //JL
        case 0x72:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;

            if (gZF == 0 && (gSF ^ gOF))
            {
                gPC = value;
            }
            else
            {
                gPC += 5;
            }

            break;

        //JE
        case 0x73:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;
            if (gZF == 1)
            {
                gPC = value;
            }
            else
            {
                gPC += 5;
            }

            break;

        //JNE
        case 0x74:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;

            if (gZF == 0)
            {
                gPC = value;
            }
            else
            {
                gPC += 5;
            }

            break;

        //JGE
        case 0x75:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;

            if (!(gZF == 0 && (gSF ^ gOF)))
            {
                gPC = value;
            }
            else
            {
                gPC += 5;
            }

            break;

        //JG
        case 0x76:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;

            if (!(gZF == 1 || (gSF ^ gOF)))
            {
                gPC = value;
            }
            else
            {
                gPC += 5;
            }

            break;
        //CALL
        case 0x80:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 1)];
            }

            value = storage.integer;
            //printf("count = %d, value = %d\n", gReg[4], value);

            gReg[4] -= 4;

            storage.integer = gPC + 5;

            for (i = 0; i < 4; i++)
            {
                gMemspace[gReg[4] + i] = storage.byte[i];
               // printf("%d\n", gMemspace[i]);
            }

            gPC = value;

            break;

        //RET
        case 0x90:
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gReg[4] + i];
            }

            gPC = storage.integer;

            gReg[4] += 4;

            break;

        //PUSHL
        case 0xA0:

            getmemspace(&regA, &regB);

            gReg[4] -= 4;

            storage.integer = gReg[regA];
            for (i = 0; i < 4; i++)
            {
                gMemspace[gReg[4] + i] = storage.byte[i];
            }

            gPC += 2;

            break;

        //POPL
        case 0xB0:

            getmemspace(&regA, &regB);
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gReg[4] + i];
            }

            value = storage.integer;

            gReg[regA] = value;
            gReg[4] += 4;
            gPC += 2;

            break;

        //READB
        case 0xC0:

            gZF = 0;

            getmemspace(&regA, &regB);

            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;
            //printf("%d", value);

            if (1 > scanf("%c", &inputchar))
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            gMemspace[gReg[regA] + value] = inputchar;

            gPC += 6;

            break;

        //READL
        case 0xC1:

            gZF = 0;

            getmemspace(&regA, &regB);
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            incorrectScan = scanf("%d", &input);
            if (incorrectScan < 1)
            {
                gZF = 1;
                //fprintf(stderr, "%s\n", "Zero flag tripped");
            }

            storage.integer = input;
            for (i = 0; i < 4; i++)
            {
                gMemspace[gReg[regA] + value + i] = storage.byte[i];
            }

            gPC += 6;

            break;

        //WRTIEB
        case 0xD0:

            getmemspace(&regA, &regB);

            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            printf("%c", (char)gMemspace[gReg[regA] + value]);
            gPC += 6;

            break;

        //WRITEL
        case 0xD1:

            getmemspace(&regA, &regB);

            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[value + gReg[regA] + i];
            }

            numberOne = storage.integer;
            printf("%d", numberOne);
            gPC += 6;

            break;

        //MOVSBL
        case 0xE0:

            //get the arguments
            getmemspace(&regA, &regB);
            //get the values of the bytes
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gPC + (i + 2)];
            }

            value = storage.integer;

            //location to be put
            storage.integer = gReg[regB];

            //store the value at the specific location
            for (i = 0; i < 4; i++)
            {
                storage.byte[i] = gMemspace[gReg[regB] + value + i];
            }

            gReg[regA] = storage.integer;
            gPC += 6;
            break;
        default:
            status = INS;
            printstatus();
            break;
        }
    }

    //printstatus();
    /*// last check for flags
    if (gZF == 1)
    {
        fprintf(stderr, "%s\n", "Zero flag tripped");
        return;
    }
    else if (gSF == 1)
    {
        fprintf(stderr, "%s\n", "Sign flag tripped");
        return;
    }
    else if (gOF == 1)
    {
        fprintf(stderr, "%s\n", "Overflow flag tripped");
        return;
    }*/
}
/*
    Seperate functions for instructions
*/
void noop()
{
    gPC++;
    return;
}
/*
    concat 
*/
char *concat(char *str, char c)
{
    int length = strlen(str) + 2;
    char *ret = (char *)calloc(length, sizeof(char));
    strcpy(ret, str);
    free(str);
    ret[length - 1] = '\0';
    ret[length - 2] = c;
    return ret;
}

/*
    hex to dec conversion
*/
int hextodec(char *num)
{
    int size = strlen(num), i, ret = 0, power, temp;
    char *binstr = (char *)malloc((4 * size + 1) * sizeof(char));

    for (i = 0; i < 4 * size + 1; i++)
    {
        binstr[i] = '\0';
    }

    for (i = 0; i < size; i++)
    {
        strcat(binstr, hextobin(num[i]));
    }

    power = strlen(binstr) - 1, temp;
    for (i = 0; binstr[i] != '\0'; i++)
    {
        temp = binstr[i] - '0';
        ret += temp * (int)pow(2, power);
        power--;
    }
    free(binstr);
    return ret;
}
/*
    hex to binary conversion
*/
char *hextobin(char c)
{
    char s = tolower(c);
    switch (s)
    {
    case '0':
        return "0000";
        break;

    case '1':
        return "0001";
        break;

    case '2':
        return "0010";
        break;

    case '3':
        return "0011";
        break;

    case '4':
        return "0100";
        break;

    case '5':
        return "0101";
        break;

    case '6':
        return "0110";
        break;

    case '7':
        return "0111";
        break;

    case '8':
        return "1000";
        break;

    case '9':
        return "1001";
        break;

    case 'a':
        return "1010";
        break;

    case 'b':
        return "1011";
        break;

    case 'c':
        return "1100";
        break;

    case 'd':
        return "1101";
        break;

    case 'e':
        return "1110";
        break;

    case 'f':
        return "1111";
        break;

    case '\0':
        break;

    default:
        fprintf(stderr, "%s\n", "Invalid hex character.");
        break;
    }
    return "";
}
/*
    get bytes
*/
int getbytes(char *str, int position)
{
    char *twobytes = (char *)malloc(3 * sizeof(char));
    int ret;

    twobytes[2] = '\0';
    twobytes[0] = str[position];
    twobytes[1] = str[position + 1];

    ret = hextodec(twobytes);

    free(twobytes);

    return ret;
}
/*
    print the status of the program
*/
void printstatus()
{
    printf("\nProgram stopped: ");
    switch (status)
    {
    case AOK:
        printf("AOK\n");
        break;

    case INS:
        printf("INS\n");
        break;

    case ADR:
        printf("ADR\n");
        break;

    case HLT:
        printf("HLT\n");
        break;
    }
}
/*
    Grab the bytes from memory
*/
void getmemspace(unsigned char *regA, unsigned char *regB)
{
    *regA = (gMemspace[gPC + 1] & 0xf0) >> 4;
    *regB = (gMemspace[gPC + 1] & 0x0f);
}