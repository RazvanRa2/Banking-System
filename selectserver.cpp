#include <string>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "utils.h"

// Metoda verifica daca se poate face un transfer bancar
int checkTransfer(char * command, struct User * bankUsers, int usersNo, int userIndex) {
    int commandSize = strlen(command);
    int cardNoStart = 9;
    int sumStart = 10;
    for (int i = cardNoStart + 1; i < commandSize; i++) {
        if (command[i] == ' ') {
            sumStart = i + 1;
            break;
        }
    }
    char cardNoChar[17];
    // as of may 2018, jeff bezos is worth $130 billion.
    // his money "fits" in "sum" so we're good
    char sum[15];
    memset(cardNoChar, 0, 17 * sizeof(char));
    memset(cardNoChar, 0, 15 * sizeof(char));
    memcpy(cardNoChar, &command[cardNoStart], (sumStart - 2) - cardNoStart + 1);
    memcpy(sum, &command[sumStart], strlen(command) - sumStart);
    int triedCardNo = atoi(cardNoChar);
    double triedSum = (double) atof(sum);
    bool availableNo = false;
    for (int i = 0; i < usersNo; i++) {
        if (bankUsers[i].cardNo == triedCardNo) {
            availableNo = true;
            break;
        }
    }
    if (availableNo == false) {
        return NOCARDNO;
    }

    double availableFunds = 0;
    for (int i = 0; i < usersNo; i++) {
        if (bankUsers[i].serverIndex == userIndex) {
            availableFunds = bankUsers[i].sold;
            break;
        }
    }
    if (availableFunds >= triedSum) {
        return triedCardNo;
    }

    return INSFUNDS;
}

// Metoda verifica daca se poate face logarea unui utilizator
int checkLogin(char * command, struct User * bankUsers, int usersNo) {
    int commandSize = strlen(command);
    int cardNoStart = 5;
    int pinStart = 7;
    for (int i = cardNoStart + 1; i < commandSize; i++) {
        if (command[i] == ' ') {
            pinStart = i + 1;
            break;
        }
    }
    char cardNoChar[17];
    char pin[5];
    memset(cardNoChar, 0, 17 * sizeof(char));
    memset(pin, 0, 5 * sizeof(char));

    memcpy(cardNoChar, &command[cardNoStart], (pinStart - 2) - cardNoStart + 1);
    memcpy(pin, &command[pinStart], strlen(command) - pinStart);
    int triedCardNo = atoi(cardNoChar);
    int triedPin = atoi(pin);
    printf("%d %d\n", triedCardNo, triedPin);
    int cardNoIndex = -1;
    for (int i = 0; i < usersNo; i++) {
        if (bankUsers[i].cardNo == triedCardNo) {
            cardNoIndex = i;
        }
    }
    if (cardNoIndex == -1) {
        return NOCARDNO;
    }

    if (bankUsers[cardNoIndex].blocked >= 2) {
        return BLOCKEDCARD;
    }
    if (bankUsers[cardNoIndex].pin == triedPin && bankUsers[cardNoIndex].blocked < 2 && bankUsers[cardNoIndex].loggedin == 1) {
        return SESSIONOPEN;
    }

    if (bankUsers[cardNoIndex].pin == triedPin && bankUsers[cardNoIndex].blocked < 2 && bankUsers[cardNoIndex].loggedin == 0) {
        bankUsers[cardNoIndex].loggedin = 1;
        return cardNoIndex;
    }

    bankUsers[cardNoIndex].blocked++;
    return WRONGPIN;
}

int main(int argc, char *argv[]) {

    struct User bankUsers[MAX_CLIENTS];  // date despre userii bancii
    int usersNo;
    FILE * fp = fopen(argv[2], "r");  // datele sunt citite din fisierul de intrare
    fscanf(fp, "%d", &usersNo);
    for (int i = 0; i < usersNo; i++) {
        fscanf(fp, "%s", bankUsers[i].lastName);
        fscanf(fp, "%s", bankUsers[i].firstName);
        fscanf(fp, "%d", &bankUsers[i].cardNo);
        fscanf(fp, "%d", &bankUsers[i].pin);
        fscanf(fp, "%s", bankUsers[i].password);
        fscanf(fp, "%lf", &bankUsers[i].sold);
        bankUsers[i].blocked = 0;
        bankUsers[i].loggedin = 0;
        bankUsers[i].tranSum = -1;
        bankUsers[i].destIdx = -1;
     }
     fclose(fp);
     int unlockingCard = -1;  // variabile utilitare
     int socktcp, newsocktcp, portno;
     unsigned int clilen;
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     int n, i, j;

     int sockudp;
     struct sockaddr_in my_sockaddr, from_station ;


     fd_set read_fds;
     fd_set tmp_fds;
     int fdmax;

     if (argc < 2) {
         fprintf(stderr,"Usage : %s port\n", argv[0]);
         exit(1);
     }
     // set-up udp si tcp
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);

     socktcp = socket(AF_INET, SOCK_STREAM, 0);
     if (socktcp < 0)
        error("ERROR opening socket");

     sockudp = socket(PF_INET, SOCK_DGRAM, 0);
     if (sockudp < 0)
        error("ERROR opening socket");


     portno = atoi(argv[1]);  // se foloseste portul specificat

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if (bind(socktcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
              error("ERROR on binding tcp");

     if (bind(sockudp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
            error("ERROR on binding udp");

     listen(socktcp, MAX_CLIENTS);

     FD_SET(socktcp, &read_fds);
     FD_SET(sockudp, &read_fds);
     FD_SET(STDIN_FILENO, &read_fds);

     fdmax = socktcp;
     // in bucla infinita
	 while (1) {
		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("ERROR in select");

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {  // se realizeaza o noua conexiune
				if (i == socktcp) {
					clilen = sizeof(cli_addr);
					if ((newsocktcp = accept(socktcp, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} else {
						FD_SET(newsocktcp, &read_fds);
						if (newsocktcp > fdmax) {
							fdmax = newsocktcp;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsocktcp);
				}else {
                    if (i == sockudp) {  // s-a primit ceva pe udp
                        memset(buffer, 0, BUFLEN);
                        if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                            if (n == 0) {
                                printf("selectserver: socket %d hung up\n", i);
                            } else {
                                error("ERROR in recv");
                            }
                            close(i);
                            FD_CLR(i, &read_fds);
                        } else {
                            printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
                            if (strstr(buffer, "unlock")) {  // daca e un mesaj de unlock, se incearca gasirea cardului
                                std::string bufAsString(buffer);
                                unlockingCard = std::stoi(bufAsString.substr(7));

                                bool found = false;
                                for (int j = 0; j < usersNo; j++) {
                                    if (bankUsers[j].cardNo == unlockingCard) {
                                        found = true;
                                        break;
                                    }
                                }
                                if (found) {
                                    sprintf(buffer, "UNLOCK> Introduceti parola secreta\n");
                                } else {
                                    sprintf(buffer, "UNLOCK> -4 : Numar card inexistent\n");
                                }
                            } else {  // daca este mesajul cu parola, se verifica daca e un moment oportun (in unlocking window)
                                bool found = false;  // si daca parola este corecta
                                for (int j = 0; j < usersNo; j++) {
                                    if (bankUsers[j].cardNo == unlockingCard && strncmp(bankUsers[j].password, buffer, strlen(bankUsers[j].password)) == 0) {
                                        sprintf(buffer, "UNLOCK> Card deblocat\n");
                                        bankUsers[j].blocked = 0;
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    sprintf(buffer, "UNLOCK> -7 : Deblocare esuata\n");
                                }
                            }
                            // se raspunde clientului
                            for (int j = 0; j < usersNo; j++) {
                                if (bankUsers[j].cardNo == unlockingCard) {
                                    sendto(bankUsers[j].serverIndex, buffer, strlen(buffer), 0, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in));
                                }
                            }
                        }
                    } else {
    					memset(buffer, 0, BUFLEN);
    					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
    						if (n == 0) {
    							printf("selectserver: socket %d hung up\n", i);
    						} else {
                                if (i == 0) {
                                    read(0, buffer, sizeof(buffer));
                                    if (strstr(buffer, "quit")) {  // daca se primeste o comanda de quit pe server
                                        const char * temp = "IBANK> Serverul se inchide, sunteti deconectat\n";
                                        strcpy(buffer, temp);
                                        for (int j = 4; j < fdmax + 1; j++)  // se transmite un mesaj specific la toti clientii
                                            n = send(4, buffer, strlen(buffer), 0);
                                        printf("IBANK> quit : se va inchide serverul\n");
                                        close(socktcp);  // se inchide conexiunea dupa inceperea deconectarii clientilor
                                        close(sockudp);
                                        exit(0);
                                    }
                                } else {
    							    error("ERROR in recv");
                                }
    						}
    						close(i);
    						FD_CLR(i, &read_fds);
    					} else {  // vine un mesaj pe tcp
                            printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);

                            if (strstr(buffer, "login") != NULL) {  // cazul mesajului de login
                                int loginResult = checkLogin(buffer, bankUsers, usersNo);

                                // se memoreaza cardul pentru unlock
                                std::string bufStr(buffer);
                                int tempCardNo = std::stoi(bufStr.substr(6, bufStr.substr(7).find_first_of(' ') + 1));

                                for (int j = 0; j < usersNo; j++) {  // se memoreaza pentru fiecare client
                                    if (bankUsers[j].cardNo == tempCardNo) {  // ultimul index de la care s-a conectat
                                        bankUsers[j].serverIndex = i;
                                    }
                                }

                                if (loginResult >= 0) {  // logarea s-a efectuat cu succes
                                    bankUsers[loginResult].serverIndex = i;
                                    sprintf(buffer, "Welcome %s %s\n",
                                    bankUsers[loginResult].lastName,
                                    bankUsers[loginResult].firstName);
                                } else {
                                    const char * temp;
                                    if (loginResult == NOCARDNO) {  // logarea nu s-a efectuat cu succes
                                        temp = "IBANK> -4 : Numar card inexistent\n";
                                    } else {
                                        if (loginResult == WRONGPIN) {
                                            temp = "IBANK> -3 : Pin gresit\n";
                                        } else {
                                            if (loginResult == BLOCKEDCARD) {
                                                temp = "IBANK> -5 : Card blocat\n";
                                            } else {
                                                temp = "IBANK> -2 : Sesiune deja deschisa\n";
                                            }
                                        }
                                    }
                                    strcpy(buffer, temp);
                                }

                            }

                            if (strstr(buffer, "logout") != NULL) {  // cazul mesajului de logout
                                int userIndex = -1;
                                for (int cnt = 0; cnt < usersNo; cnt++) {
                                    if (bankUsers[cnt].serverIndex == i) {
                                        userIndex = cnt;
                                        break;
                                    }
                                }
                                bankUsers[userIndex].serverIndex = -1;
                                if (bankUsers[userIndex].blocked < 2) {
                                    bankUsers[userIndex].loggedin = 0;
                                    bankUsers[userIndex].blocked = 0;
                                }
                                const char * temp = "IBANK> Clientul a fost deconectat\n";
                                strcpy(buffer, temp);
                            }

                            if (strstr(buffer, "listsold") != NULL) {  // cazult mesajului de interogare sold
                                int userIndex = -1;
                                for (int cnt = 0; cnt < usersNo; cnt++) {
                                    if (bankUsers[cnt].serverIndex == i) {
                                        userIndex = cnt;
                                        break;
                                    }
                                }
                                sprintf(buffer, "IBANK> %.2lf\n", bankUsers[userIndex].sold);
                            }

                            if (strstr(buffer, "transfer") != NULL) {  // cazul transferului
                                // se verifica daca se poate efectua transferul
                                int status = checkTransfer(buffer, bankUsers, usersNo, i);
                                if (status == NOCARDNO) {  // nu exista cardul catre care se transmit banii
                                    const char * temp = "IBANK> Numar card inexistent\n";
                                    strcpy(buffer, temp);
                                } else {
                                    if (status == INSFUNDS) {  // utilizatorul nu are suficiente fonduri
                                        const char * temp = "IBANK> Founduri insuficiente\n";  // buzunarul meu intra mereu pe cazul asta
                                        strcpy(buffer, temp);
                                    } else {
                                        // se determina cardul si suma
                                        int commandSize = strlen(buffer);
                                        int cardNoStart = 9;
                                        int sumStart = 10;
                                        for (int j = cardNoStart + 1; j < commandSize; j++) {
                                            if (buffer[j] == ' ') {
                                                sumStart = j + 1;
                                                break;
                                            }
                                        }
                                        char cardNoChar[17];
                                        memset(cardNoChar, 0, 17 * sizeof(char));
                                        memcpy(cardNoChar, &buffer[cardNoStart], (sumStart - 2) - cardNoStart + 1);
                                        int triedCardNo = atoi(cardNoChar);
                                        int dest = -1;
                                        for (int j = 0; j < usersNo; j++) {
                                            if (triedCardNo == bankUsers[j].cardNo) {
                                                dest = j;
                                                break;
                                            }
                                        }
                                        for (int j = 0; j < usersNo; j++) {
                                            if (i == bankUsers[j].serverIndex) {
                                                bankUsers[j].destIdx = dest;
                                                break;
                                            }
                                        }

                                        char sum[15];
                                        memset(cardNoChar, 0, 15 * sizeof(char));
                                        memcpy(sum, &buffer[sumStart], strlen(buffer) - sumStart);
                                        double triedSum = (double) atof(sum);
                                        int me = -1;
                                        for (int j = 0; j < usersNo; j++) {
                                            if (bankUsers[j].serverIndex == i) {
                                                me = j;
                                                break;
                                            }
                                        }
                                        // se cere confirmarea transferului
                                        bankUsers[me].tranSum = triedSum;
                                        bankUsers[me].destIdx = dest;
                                        char temp[BUFLEN];
                                        sprintf(temp, "IBANK> Transfer %.2lf catre %s %s? [y/n]\n", triedSum, bankUsers[dest].lastName, bankUsers[dest].firstName);
                                        strcpy(buffer, temp);
                                    }
                                }
                            }
                            // s-a primit un mesaj de confirmare a unui transfer
                            if (strcmp(buffer, "y\n") == 0) {
                                int index = -1;
                                for (int j = 0; j < usersNo; j++) {
                                    if (i == bankUsers[j].serverIndex) {
                                        index = j;
                                        break;
                                    }
                                }
                                if (bankUsers[index].tranSum != -1) {
                                    bankUsers[index].sold -= bankUsers[index].tranSum;
                                    bankUsers[bankUsers[index].destIdx].sold += bankUsers[index].tranSum;
                                    bankUsers[index].tranSum = -1;
                                    bankUsers[index].destIdx = -1;
                                }
                                const char * temp = "IBANK> Transfer realizat cu succes\n";
                                strcpy(buffer, temp);
                            }
                            // s-a primit anularea unui transfer
                            if (strcmp(buffer, "n\n") == 0) {
                                int index = -1;
                                for (int j = 0; j < usersNo; j++) {
                                    if (i == bankUsers[j].serverIndex) {
                                        index = j;
                                        break;
                                    }
                                }
                                bankUsers[index].tranSum = -1;
                                bankUsers[index].destIdx = -1;

                                const char * temp = "IBANK> -9 : operatie anulata\n";
                                strcpy(buffer, temp);
                            }
                            // mesaj de quit din partea clientului
                            if (strstr(buffer, "quit") != NULL) {
                                const char * temp = "IBANK> Sesiunea va fi inchisa\n";
                                strcpy(buffer, temp);
                            }

                            printf("%d\n", i);
                            n = send(i, buffer, strlen(buffer), 0);
    					}
                    }
				}
			}
		}
     }
     close(sockudp);
     close(socktcp);

     return 0;
}
