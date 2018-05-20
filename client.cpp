// Copyright 2018 Razvan Radoi, first of his name
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "utils.h"

using namespace std;

int main(int argc, char *argv[]) {

    vector<string>serverAnswers;  // memorizes all server replies until quit

    bool loggedIn = false;  // utilitary variables
    bool unlocking = false;
    int cardNo = -1;
    int sockfd, n;  // sockfd = tcp
    int sockudp;  // sockudp = udp
    struct sockaddr_in serv_addr;
    struct hostent *server;
    fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    char buffer[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);  // socket tcp
    if (sockfd < 0)
        error("ERROR opening socket");

    sockudp = socket(AF_INET, SOCK_DGRAM, 0);  // socket udp
    if (sockudp < 0)
        error("ERROR opening socket");
    // setup tcp
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);

    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    FD_SET(0, &read_fds);
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;

    while(1) {
        tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("ERROR in select");

        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) { // citesc de la server
                if (i == sockfd) {
                    memset(buffer, 0 , BUFLEN);
                    int n = recv(sockfd, buffer, sizeof(buffer), 0);

                    if (strstr(buffer, "Welcome") != NULL) {
                        loggedIn = true;  // marcam sesiunea ca logged in
                    }
                    if (strstr(buffer, "IBANK> Clientul a fost deconectat") != NULL) {
                        loggedIn = false;  // demarcam logarea
                    }
                    if (strstr(buffer, "Serverul se inchide") != NULL) {
                        loggedIn = false;  // toti userii sunt deconectati la
                        goto endLabel;  // inchiderea serverului
                    }

                    if (n > 0) {  // caz default, mesaj ce trebuie doar memorat
                        printf("%s", buffer);
                        string temp(buffer);
                        serverAnswers.push_back(temp);
                    }
                } else {
              		// citesc de la tastatura
                	memset(buffer, 0 , BUFLEN);
                	fgets(buffer, BUFLEN-1, stdin);
                    if (strstr(buffer, "login")) {  // la login retin cardul logat ultima data pt unlock
                        string bufStr(buffer);
                        cardNo = stoi(bufStr.substr(6, bufStr.substr(7).find_first_of(' ') + 1));
                    }
                    if (strstr(buffer, "unlock") || unlocking) {  // daca se primeste o comanda de unlock
                        if (strstr(buffer, "unlock")) {  // ea se trimite pe udp
                            unlocking = true;
                            sprintf(buffer, "unlock %d", cardNo);
                        } else {  // cazul in care se trimite parola, trebuie sa stim ca starea de "unlocking" s-a terminat
                            unlocking = false;
                        }
                        sendto(sockudp, buffer, BUFLEN, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                    } else {
                        if (strstr(buffer, "logout") && loggedIn == false) {  // cazul logout without previous login
                            printf("-1 : Clientul nu este autentificat\n");
                            string temp("-1 : Clientul nu este autentificat\n");
                            serverAnswers.push_back(temp);
                        } else {
                        	// trimit mesaj la server
                        	n = send(sockfd,buffer,strlen(buffer), 0);

                        	if (n < 0)
                                error("ERROR writing to socket");

                            if (strstr(buffer, "quit")) {
                                goto endLabel;  // forgive me father for i have sinned
                            }
                        }
                    }
                }
            }
        }
    }

    // scrierea in fisierul .log se face la final, dupa terminarea transmisiei
    endLabel:
    close(sockfd);
    close(sockudp);
    char logFileName[30];
    sprintf(logFileName, "client-%d.log", getpid());
    ofstream out(logFileName);
    for (int cnt = 0; cnt < serverAnswers.size(); cnt++) {
        out << serverAnswers[cnt];
    }
    out.close();

    return 0;
}
