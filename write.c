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

volatile int STOP=FALSE;

void printMenu();
void printSender();
void printReceiver();

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

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

    int option=0;
    char flush;

    while(option != SENDER && option!= RECEIVER){
        printMenu();
        scanf("%d", &option);
        scanf("%c", &flush);
    }
        
    if(option == SENDER){
        char buffer[255];
        int buffer_size;
        printSender();
        fgets(buffer, 255, stdin);
        buffer_size = strlen(buffer);
        res = write(fd, buffer, buffer_size);
        printf("\nBytes written: %d\n", res);

        tcflush(fd, TCIOFLUSH);
    }

    if(option == RECEIVER){
        int bytes = 0, res;
        int STOP=FALSE;
        char buffer[1], strRead[255];

        printReceiver();

        while(STOP == FALSE){
            bytes++;
            res = read(fd,buffer, 1);
            buffer[res]=0;
            strRead[bytes] = buffer[0];
            if(buffer[0] == '\n') STOP = TRUE;
            
        }

        for(int i=0;i<bytes;i++){
            printf("%c", strRead[i]);
        }
        printf("\n\nBytes read: %d\n", bytes);
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
    printf("1. Send control frame\n");
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
    printf("Received control frame:");
}