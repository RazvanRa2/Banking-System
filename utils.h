#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define NOTLOGGEDIN -1
#define SESSIONOPEN -2
#define WRONGPIN -3
#define NOCARDNO -4
#define BLOCKEDCARD -5
#define OPFAILED -6
#define UNLOCKFAILED -7
#define INSFUNDS -8
#define CANCELEDOP -9
#define CALLERR -10
#define SUCCESS 0

#define BUFLEN 256
#define MAX_CLIENTS	5

struct User {
    char lastName[12];
    char firstName[12];
    int cardNo;
    int pin;
    char password[8];
    double sold;
    int blocked;
    int loggedin;
    int serverIndex;
    double tranSum;
    int destIdx;
};

void error(char *msg)
{
    perror(msg);
    exit(0);
}
