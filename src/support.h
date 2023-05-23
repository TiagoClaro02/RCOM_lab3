#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

//? State defines
#define START_STATE 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_STATE 5

#define F 0x5C
#define A 0x01
#define C 0x03
#define BCCSET 0x02
#define BCCUA  0x06
#define BCCDISC 0x0A

#define SET_SIZE 5
#define UA_SIZE 5

typedef enum{
    set_msg,
    ua_msg,
    disc_msg
} message;

unsigned char set[5] = {0x5C, 0x01, 0x03, 0x02, 0x5C};
unsigned char ua[5] = {0x5C, 0x01, 0x07, 0x06, 0x5C};
unsigned char disc[5] = {0x5C, 0x01, 0x0B, 0x0A, 0x5C};

int getState(int currentState, unsigned char str, message msg);
int sendSet(int fd);
int waitUa(int fd);
int receiveSet(int fd);
int sendUa(int fd);
int dataInit(int currentState, unsigned char character);
int sendDisc(int fd);
int receiveDisc(int fd);

int getState(int currentState, unsigned char str, message msg){


    switch (currentState)
    {
    case START_STATE:

        if(str == F){
            currentState = FLAG_RCV;
        }
        
        break;
    
    case FLAG_RCV:

        if(str == F){
            currentState = FLAG_RCV;
        }
        else if(str == A){
            currentState = A_RCV;
        }
        else{
            currentState = START_STATE;
        }

        break;

    case A_RCV:
        
        if(msg == set_msg){

            if(str == F){
                currentState = FLAG_RCV;
            }
            else if(str == C){
                currentState = C_RCV;
            }
            else{
                currentState = START_STATE;
            }

            break;
        }

        if(msg == ua_msg){

            if(str == F){
                currentState = FLAG_RCV;
            }
            else if(str == 0x07){
                currentState = C_RCV;
            }
            else{
                currentState = START_STATE;
            }

            break;
        }

        if(msg == disc_msg){

            if(str == F){
                currentState = FLAG_RCV;
            }
            else if(str == 0x0B){
                currentState = C_RCV;
            }
            else{
                currentState = START_STATE;
            }

            break;
        }

    case C_RCV:

        if(str == F){
            currentState = FLAG_RCV;
        }
        else if(((msg == set_msg) && (str^A == BCCSET)) || ((msg==ua_msg) && (str^A == BCCUA)) || ((msg == disc_msg) && (str^A == BCCDISC))){
            currentState = BCC_OK;
        }
        else{
            currentState = START_STATE;
        }

        break;

    case BCC_OK:

        if(str == F){
            currentState = STOP_STATE;
        }
        
        else{
            currentState = START_STATE;
        }

        break;
    }

    return currentState;
}

int sendSet(int fd){

    int res;
    int bytes = 0;
    unsigned char buffer;

    res = write(fd, set, SET_SIZE);

    return res;
}

int waitUa(int fd){

    int bytes = 0;
    int res;
    char newline;
    unsigned char buffer;
    int state = START_STATE;

    while(state != STOP_STATE){
        bytes++;
        res = read(fd,&buffer, 1);
        state = getState(state, buffer, ua_msg);
    }

    if(state == STOP_STATE){
        return 1;
    }

    return 0;
}

int receiveSet(int fd){

    int bytes = 0, res;
    int STOP=0;
    unsigned char buffer;
    int state = START_STATE;

    while(state != STOP_STATE){
        
        bytes++;
        res = read(fd,&buffer, 1);
        state = getState(state, buffer, set_msg);

    }
    
    if(state == STOP_STATE){
        return 1;
    }

    return 0;
}

int sendUa(int fd){
    
    int res;

    res = write(fd, ua, SET_SIZE);

    return res;
}

int dataInit(int currentState, unsigned char character){
    
    switch (currentState)
    {
    case START_STATE:
        if(character == F){
            currentState = A_RCV;
        }
        break;
    
    case A_RCV:
        if(character == A){
            currentState = C_RCV;
        }
        break;

    case C_RCV:
        if(character == 0x03){
            currentState = BCC_OK;
        }
        break;
    }

    return currentState;
}

int sendDisc(int fd){
    
    int res;
    int bytes = 0;
    unsigned char buffer;

    res = write(fd, disc, SET_SIZE);

    return res;
}

int receiveDisc(int fd){
    
    int bytes = 0, res;
    int STOP=0;
    unsigned char buffer;
    int state = START_STATE;

    while(state != STOP_STATE){
        bytes++;
        res = read(fd,&buffer, 1);
        state = getState(state, buffer, disc_msg);
    }
    
    if(state == STOP_STATE){
        return 1;
    }

    return 0;
}