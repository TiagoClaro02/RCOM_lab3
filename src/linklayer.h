#ifndef LINKLAYER
#define LINKLAYER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "support.h"

typedef struct linkLayer{
    char serialPort[50];
    int role; //defines the role of the program: 0==Transmitter, 1=Receiver
    int baudRate;
    int numTries;
    int timeOut;
} linkLayer;

int fileDescriptor;

//ROLE
#define NOT_DEFINED -1
#define TRANSMITTER 0
#define RECEIVER 1


//SIZE of maximum acceptable payload; maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 500

//CONNECTION deafault values
#define BAUDRATE_DEFAULT B38400
#define MAX_RETRANSMISSIONS_DEFAULT 3
#define TIMEOUT_DEFAULT 4
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//MISC
#define FALSE 0
#define TRUE 1

// Opens a connection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen(linkLayer connectionParameters);
// Sends data in buf with size bufSize
int llwrite(char* buf, int bufSize);
// Receive data in packet
int llread(char* packet);
// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(linkLayer connectionParameters, int showStatistics);

#endif

int llopen(linkLayer connectionParameters){
    
    //int fd;
    struct termios oldtio, newtio;

    fileDescriptor = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );

    if (fileDescriptor < 0) { perror(connectionParameters.serialPort); exit(-1); }

    if ( tcgetattr(fileDescriptor,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE_DEFAULT | CS8 | CLOCAL | CREAD;
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


    tcflush(fileDescriptor, TCIOFLUSH);

    if (tcsetattr(fileDescriptor,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    //TODO SENDER
    if(connectionParameters.role == TRANSMITTER){

        if(!sendSet(fileDescriptor)){
            printf("Error sending SET\n");
            return -1;
        }
        else{
            printf("SET sent successfully\n");
        }

        if(!waitUa(fileDescriptor)){
            printf("Error receiving UA\n");
            return -1;
        }
        else{
            printf("UA received successfully\n");
            return fileDescriptor;
        }
    }

    //TODO RECEIVER
    if(connectionParameters.role == RECEIVER){

        if(!receiveSet(fileDescriptor)){
            printf("Error receiving SET\n");
            return -1;
        }
        else{
            printf("Set received successfully\n");

            if(!sendUa(fileDescriptor)){
                printf("Error sending UA\n");
                return -1;
            }
            else{
                printf("UA sent successfully\n");
                return fileDescriptor;
            }
        }

    }

    return -1;
}

int llwrite(char* buf, int bufSize){

    int bytesWritten = 0;
    int bytes2write = bufSize;
    int count_1 = 0, count_2 = 0;
    unsigned char facb[4] = {0x5C, 0x01, 0x02, 0x03};
    unsigned char bf[2] = {0x02, 0x5C};
    unsigned char buffer[8];
    unsigned char buf2send[2*bufSize];
    unsigned char bcc2 = 0x00;
    unsigned char ESC = 0x5D;
    unsigned char RES1 = 0x7C;
    unsigned char RES2 = 0x7D;

    for(int i=0; i<4; i++){
        buf2send[count_2] = facb[i];
        count_2++;
    }

    while(count_1 <= bufSize){

        bcc2 = bcc2 ^ buf[count_1];

        if(buf[count_1] == 0x5C){
            buf2send[count_2] = ESC;
            count_2++;
            buf2send[count_2] = RES1;
            count_2++;
        }
        else if(buf[count_1] == ESC){
            buf2send[count_2] = ESC;
            count_2++;
            buf2send[count_2] = RES2;
            count_2++;
        }
        else{
            buf2send[count_2] = buf[count_1];
            count_2++;
        }
        count_1++;

    }

    if(bcc2 == 0x5C){
        buf2send[count_2] = ESC;
        count_2++;
        buf2send[count_2] = RES1;
        count_2++;
    }
    else if(bcc2 == ESC){
        buf2send[count_2] = ESC;
        count_2++;
        buf2send[count_2] = RES2;
        count_2++;
    }
    else{
        buf2send[count_2] = bcc2;
        count_2++;
    }

    buf2send[count_2] = 0x5C;
    count_2++;

    unsigned char data2send[count_2];

    for(int i=0; i<count_2; i++){
        data2send[i] = buf2send[i];
        printf("%.2x-", data2send[i]);
    }

    bytesWritten = write(fileDescriptor, &data2send, count_2);

    return bytesWritten;
}

    

int llread(char* packet){

    int stop = FALSE;
    int i=0;
    int aux;
    int state = START_STATE;
    unsigned char buffer;
    unsigned char dataReceived[1000];
    unsigned char ESC = 0x5D;
    unsigned char RES1 = 0x7C;
    unsigned char RES2 = 0x7D;
    unsigned char bcc2 = 0x00;


    while(state != BCC_OK){
        if(read(fileDescriptor, &buffer, 1) != 1){
            return -1;
        }
        printf("%.2x\n", buffer);
        state = dataInit(state, buffer);
    }

    while (stop == FALSE){
        
        if(!read(fileDescriptor, &buffer, 1)){
            return -1;
        }

        if(buffer == 0x5C){
            stop = TRUE;
            packet[i] = buffer;
            dataReceived[i] = buffer;
            //i++;
        }
        else if(buffer == ESC){
            if(!read(fileDescriptor, &buffer, 1)){
                return -1;
            }   
            if(buffer == RES1){
                packet[i] = 0x5C;
                dataReceived[i] = 0x5C;
                //printf("%c-", packet[i]);
            }
            else{
                packet[i] = 0x5D;
                dataReceived[i] = 0x5D;
                //printf("%c-", packet[i]);
            }
            i++;
            aux = i - 1;
            bcc2 = bcc2 ^ packet[aux];
        }
        else{
            packet[i] = buffer;
            dataReceived[i] = buffer;
            //printf("%c-", packet[i]);
            i++;
            aux = i - 1;
            bcc2 = bcc2 ^ packet[aux];
        }

        
        printf("%.2x-", dataReceived[aux]);

    }
    //printf("\nbcc2: %.2x\n", bcc2);
    return i;
}

int llclose(linkLayer connectionParameters, int showStatistics){
    
    if(connectionParameters.role == TRANSMITTER){

        if(!sendDisc(fileDescriptor)){
            if(showStatistics){
                printf("Error sending DISC\n");
            }
            return -1;
        }
        else{
            if(showStatistics){
                printf("DISC sent successfully\n");
            }
        }

        if(!receiveDisc(fileDescriptor)){
            if(showStatistics){
                printf("Error receiving DISC\n");
            }
            return -1;
        }
        else{
            if(showStatistics){
                printf("DISC received successfully\n");
            }
        }
        if(!sendUa(fileDescriptor)){
            if(showStatistics){
                printf("Error sending UA\n");
            }
            return -1;
        }
        else{
            if(showStatistics){
                printf("UA sent successfully\n");
            }
            return 0;
        }

    }

    if(connectionParameters.role == RECEIVER){

    
        if(!receiveDisc(fileDescriptor)){
            if(showStatistics){
                printf("Error receiving DISC\n");
            }
            return -1;
        }
        else{
            if(showStatistics){
                printf("DISC received successfully\n");
            }

            if(!sendDisc(fileDescriptor)){
                if(showStatistics){
                    printf("Error sending DISC\n");
                }
                return -1;
            }
            else{
                if(showStatistics){
                    printf("DISC sent successfully\n");
                }
            }

            if(!waitUa(fileDescriptor)){
                if(showStatistics){
                    printf("Error sending UA\n");
                }
                return -1;
            }
            else{
                if(showStatistics){
                    printf("UA received successfully\n");
                }
                return 0;
            }
        }
    }

    return -1;
}