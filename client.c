/* Program:   client portion of Gomoku game
** Authors:   Joe Spinosa, Kyle Hassett, & Matt Woehle
** Date:      11/5/2016
** File Name: client.c
** Compile:   cc -o client client.c
** Run:       ./client server1.cs.scranton.edu 32300 http:/./client server1.cs.s#+1477877448 
**            This program is the client portion
**            of a 2 client per subserver game of gomoku
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define HTTPPORT "32300" 
#define BUFFERSIZE 256 

int get_server_connection(char *hostname, char *port);
void compose_http_request(char *http_request, char *filename);
void web_browser(int http_conn, char *http_request);
void print_ip( struct addrinfo *ai);
void createBoard(char board[8][8]);
void printBoard(char board[8][8]);
int isLegal(int row, int col);

int main(int argc, char *argv[]){
    int http_conn;  
    char http_request[BUFFERSIZE];

    if (argc != 4) {
        printf("usage: client HOST HTTPORT webpage\n");
        exit(1);
    }

    // steps 1, 2: get a connection to server
    if ((http_conn = get_server_connection(argv[1], argv[2])) == -1) {
       printf("connection error\n");
       exit(1);
    }

    // step 3.0: make an HTTP request
    compose_http_request(http_request, argv[3]);

    // step 4: web browser
    web_browser(http_conn, http_request);

    // step 5: as always, close the socket when done
    close(http_conn);
}

int get_server_connection(char *hostname, char *port) {
    int serverfd;
    struct addrinfo hints, *servinfo, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       return -1;
    }

    print_ip(servinfo);
    for (p = servinfo; p != NULL; p = p ->ai_next) {
       if ((serverfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }

       if ((status = connect(serverfd, p->ai_addr, p->ai_addrlen)) == -1) {
           close(serverfd);
           printf("socket connect \n");
           continue;
       }
       break;
    }

    freeaddrinfo(servinfo);
   
    if (status != -1) return serverfd;
    else return -1;
}


void  compose_http_request(char *http_request, char *filename) {
    strcpy(http_request, "GET /");
    strcpy(&http_request[5], filename);
    strcpy(&http_request[5+strlen(filename)], " ");
}

void web_browser(int http_conn, char *http_request) {
    int numbytes = 0;
    int i, bytes_sent, bytes_recv;
    char buf[BUFFERSIZE+1];
    char *message, playerid;
   
    bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0); //GET PLAYER ID
    strcpy(&playerid, buf); // assign playerid
    printf("you are player %s\n", buf);
    
    
    printf("Welcome to Gomoku!\n");
    printf("Which is really 8x8 tic tac toe\n");
    printf("Please enter your moves in form: x y (separated by a space)\n\n");    

    char board[8][8];
    createBoard(board);
    int row, col, check, tempx, tempy;
    char x;
    char y;

    strcpy(buf, "Your turn: ");

    while(strcmp(buf, "0") != 0){ // someone wins when this while ends
        
        if(strcmp(&playerid, "1") == 0){ // player 1 code starts here
            printBoard(board);//print updated board
            printf("%s", buf);
            scanf(" %c %c", &x, &y); //take in row and col
            printf("\n");
            row = x - '0';
            col = y - '0';
            //printf("intput: %d %d\n", row, col); //for testing, not necessary anymore     
    
            while((check = isLegal(row, col)) != 0){ //check for out of bounds
               printf("Invalid move please try again: ");
               scanf(" %c %c", &x, &y); //new entry after an out of bounds
               row = x - '0';
               col = y - '0';
               //printf("intput: %d %d\n", row, col); //for testing not necessary
            }
            bytes_sent = send(http_conn, (void *)&x, sizeof(x), 0);   //a1: send server row and col
            bytes_sent = send(http_conn, (void *)&y, sizeof(y), 0);   //a2
            
            bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0);//a3 GET 0,1,2
			
            while((strcmp(buf, "1") != 0 )){//check server response
                if(strcmp(buf, "0") == 0){ //0 means game over, you won
                    printBoard(board);
                    printf("You Just Won!\n");
                    exit(0);
                }
                else if(strcmp(buf, "2") == 0 ){//2 means space entered is already occupied
                    printf("Error: space already occupied\n");
                    printf("Your Turn Player 1: ");   
                    scanf(" %c %c", &x, &y); //take in new row and col
		    printf("\n");
                    row = x - '0';
                    col = y - '0';
                    printf("input: %d %d\n", row, col);
                    
                    while((check = isLegal(row, col)) == 1){ //check again for out of bounds
                        printf("Invalid move please try again: ");
                        scanf(" %c %c", &x, &y); //entry out of bounds try again before send to server
                        printf("\n");
                        row = x - '0';
                        col = y - '0';                             
                        //printf("intput: %d %d\n", x, y); //for testing, not required anymore
                    }
                
                    bytes_sent = send(http_conn, (void *)&x, sizeof(x), 0);  //a1 l: send new row
                    bytes_sent = send(http_conn, (void *)&y, sizeof(y), 0);  //a2 l: send new col

                    bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0);  //a3 l: receive next response
                }//end response 2 else if
            }// end server response while
            
            board[row][col] = 'X'; //at this point move entered is valid
            printBoard(board);     //so update and print the board           
            printf("Move is valid!, Please wait for Player 2\n");
            
            bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0); //wait for your next turn
            if(strcmp(buf, "0") == 0){ //server sends 0 if other player won
                bytes_recv = recv(http_conn, &tempx, sizeof(tempx), 0);
                row = tempx;
                bytes_recv = recv(http_conn, &tempy, sizeof(tempy), 0);
                col = tempy;
                board[row][col] = 'O';
                printf("Player 2 won :-(\n");
                exit(0);
            }

            bytes_recv = recv(http_conn, &tempx, sizeof(tempx), 0);//other player's row
            row = tempx;
            bytes_recv = recv(http_conn, &tempy, sizeof(tempy), 0);//other player's col
            col = tempy;
            board[row][col] = 'O'; //store other players board for updated move
            bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0); //a4: tells you it's your turn again
        }
        else{ //*******PLAYER 2 ACTIONS HERE********************
		    
            bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0);//receive info if p1 won
            if(strcmp(buf, "0") == 0){ //0 if p1 won
                bytes_recv = recv(http_conn, &tempx, sizeof(tempx), 0);//p1's winning row
                row = tempx;
                bytes_recv = recv(http_conn, &tempy, sizeof(tempy), 0);//p1's winning col
                col = tempy;
                board[row][col] = 'X';    //store p1's winning move
                printBoard(board);        //and print the board
                printf("Player 1 won :-(\n");
                exit(0);
            }
            bytes_recv = recv(http_conn, &tempx, sizeof(tempx), 0);//other p1 row
            row = tempx;
            bytes_recv = recv(http_conn, &tempy, sizeof(tempy), 0);//other p1 col 
            col = tempy;
            board[row][col] = 'X';    //store p1 move
            bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0);//b1: tell p2 its their turn
            
            printBoard(board);  //print updated board for p2
            printf("%s", buf);  
            scanf(" %c %c", &x, &y);//take in next move
            printf("\n");
            row = x - '0';
            col = y - '0';
            //printf("intput: %d %d\n", row, col); //for testing, not necessary
			
            while((check = isLegal(row, col)) == 1){//make sure move is in bounds
               printf("Invalid move please try again: ");
               scanf(" %c %c", &x, &y);
               row = x - '0';
               col = y - '0';
               printf("intput: %d %d\n", x, y);
            }

            bytes_sent = send(http_conn, (void *)&x, sizeof(x), 0);  //b2: send row
            bytes_sent = send(http_conn, (void *)&y, sizeof(y), 0);  //b3: send col
            bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0);   //b4: get server move response

            while((strcmp(buf, "1") != 0 )){//check response
                if(strcmp(buf, "0") == 0){//server returned 0, p2 won
                    board[row][col] = 'O';
                    printBoard(board);
                    printf("Player 2 Won!\n");
                    exit(0);
                }
                else if(strcmp(buf, "2") == 0 ){ //b4 l: move entered was already occupied, get new move 
                    printf("Error: space already occupied\n");
                    printf("Your Turn Player 2: ");   
                    scanf(" %c %c", &x, &y);
					printf("\n");
                    row = x - '0';     
                    col = y - '0'; 
                    printf("intput: %d %d\n", x, y);
                    while((check = isLegal(row, col)) == 1){  //make sure new move is in bounds
                        printf("Invalid move please try again: "); 
                        scanf(" %c %c", &x, &y); 
                        row = x - '0';           
                        col = y - '0';           
                        //printf("intput: %d %d\n", row, col); //for testing, not necessary
                    }
                }
                bytes_sent = send(http_conn, (void *)&x, sizeof(x), 0);  //b2l: send new row
                bytes_sent = send(http_conn, (void *)&y, sizeof(y), 0);  //b3l: send new col
                
                bytes_recv = recv(http_conn, buf, (BUFFERSIZE+1), 0); //b3l: get new server response             
            }
            board[row][col] = 'O';//move was valid, updat board and wait for turn again
            printBoard(board);
            printf("Move is valid!, Please wait for Player 1\n"); 
        }

    }

    exit(0);
}

void print_ip( struct addrinfo *ai) {
   struct addrinfo *p;
   void *addr;
   char *ipver;
   char ipstr[INET6_ADDRSTRLEN];
   struct sockaddr_in *ipv4;
   struct sockaddr_in6 *ipv6;
   short port = 0;

   for (p = ai; p !=  NULL; p = p->ai_next) {
      if (p->ai_family == AF_INET) {
         ipv4 = (struct sockaddr_in *)p->ai_addr;
         addr = &(ipv4->sin_addr);
         port = ipv4->sin_port;
         ipver = "IPV4";
      }
      else {
         ipv6= (struct sockaddr_in6 *)p->ai_addr;
         addr = &(ipv6->sin6_addr);
         port = ipv4->sin_port;
         ipver = "IPV6";
      }
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      printf("serv ip info: %s - %s @%d\n", ipstr, ipver, ntohs(port));
   }
}

void createBoard(char board[8][8]) {
   int i, j;
   for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++) {
         board[i][j] = '#';
      }
   }
}

void printBoard(char board[8][8]){
   int i, j;
   printf("  0 1 2 3 4 5 6 7\n");
   for (i = 0; i < 8; i++) {
      printf("%d ", i);
      for (j = 0; j < 8; j++) {
         printf("%c ", board[i][j]);
      }
      printf("%d", i);	
      printf("\n");
   }
}

int isLegal(int row, int col){
    if((row<0) || (row>8) || (col<0) || (col>8)){
       return 1;
    }
    else
       return 0;
}
