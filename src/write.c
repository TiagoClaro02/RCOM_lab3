/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define SENDER 1
#define RECEIVER 2

//? State defines
#define START_STATE 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_STATE 5

//? Data to send
#define SET 0x5C0103025C
#define UA 0x5C0105025C
#define SET_SIZE 5
#define UA_SIZE 5

volatile int STOP=FALSE;

void printMenu();
void printState(int currentState);
void printSender();
void printReceiver();
int getState(int currentState, unsigned char *str);

unsigned char set[5] = {0x5C, 0x01, 0x03, 0x02, 0x5C};
unsigned char ua[5] = {0x5C, 0x01, 0x05, 0x02, 0x5C};
unsigned char a;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    int state = START_STATE;
    

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS10", argv[1])!=0) &&
          (strcmp("/dev/ttyS11", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    //TODO ######## MAIN FUNCTION ########
    while(1){

        int option=0;
        char flush;

        while(option != SENDER && option!= RECEIVER){
            printMenu(state);
            scanf("%d", &option);
            scanf("%c", &flush);
        }
            

        //TODO SENDER
        if(option == SENDER){
            char newline;

            printSender();
            printf("0x%02x%02x%02x%02x%02x\n", set[0], set[1], set[2], set[3], set[4]);
            res = write(fd, set, SET_SIZE);
            printf("\nBytes written: %d\n", res);

            while(newline != '\n'){
                scanf("%c", &newline);
            }

        }


        //TODO RECEIVER
        if(option == RECEIVER){
            int bytes = 0, res;
            int STOP=FALSE;
            unsigned char buffer[1], newline='\0';

            printReceiver();

            while(state != STOP_STATE){
                bytes++;
                res = read(fd,buffer, 1);
                buffer[res]=0;
                getState(state, buffer);
                printf("Byte received:\t%02x\n", buffer[0]);
                state = getState(state, buffer);
                printState(state);
            }
            

            printf("\n\nBytes read: %d\n", bytes);

            state = START_STATE;

            tcflush(fd, TCIOFLUSH);

            while(newline != '\n'){
                scanf("%c", &newline);
            }
        }

    }


    //TODO ######## END FUNCTION ########
    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}

void printMenu(){
    system("clear");
    printf("########### CONTROL FRAME ###########\n\n");
    printf("\n\n1. Send control frame\n");
    printf("2. Receive control frame\n\n");
    printf("Select option:\t");
}

void printSender(){
    system("clear");
    printf("########### SENDER ###########\n\n");
    printf("Input the control frame: ");
}

void printReceiver(){
    system("clear");
    printf("########### RECEIVER ###########\n\n");
    printf("Received control frame:\n");
}

void printState(int currentState){
    if(currentState == START_STATE) printf("Current state:\tSTART\n\n");
    if(currentState == FLAG_RCV) printf("Current state:\tFLAG_RCV\n\n");
    if(currentState == A_RCV) printf("Current state:\tA_RCV\n\n");
    if(currentState == C_RCV) printf("Current state:\tC_RCV\n\n");
    if(currentState == BCC_OK) printf("Current state:\tBCC_OK\n\n");
    if(currentState == STOP_STATE) printf("Current state:\tSTOP\n\n");
}

int getState(int currentState, unsigned char *str){

    unsigned char F = 0x5C;
    unsigned char A = 0x01;
    unsigned char C = 0x03;
    unsigned char BCC = 0x02;

    switch (currentState)
    {
    case START_STATE:

        if(str[0] == F){
            currentState = FLAG_RCV;
        }
        
        break;
    
    case FLAG_RCV:

        if(str[0] == F){
            currentState = FLAG_RCV;
        }
        else if(str[0] == A){
            currentState = A_RCV;
        }
        else{
            currentState = START_STATE;
        }

        break;

    case A_RCV:

        if(str[0] == F){
            currentState = FLAG_RCV;
        }
        else if(str[0] == C){
            currentState = C_RCV;
        }
        else{
            currentState = START_STATE;
        }

        break;

    case C_RCV:

        if(str[0] == F){
            currentState = FLAG_RCV;
        }
        else if((str[0]^A) == C){
            currentState = BCC_OK;
        }
        else{
            currentState = START_STATE;
        }

        break;

    case BCC_OK:

        if(str[0] == F){
            currentState = STOP_STATE;
        }
        
        else{
            currentState = START_STATE;
        }

        break;
    }

    return currentState;
}