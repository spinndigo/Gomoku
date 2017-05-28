/* Program:   server portion of Gomoku game
** Authors:   Joe Spinosa, Kyle Hassett, & Matt Woehle
** Date:      11/5/2016
** File Name: server.c
** Compile:   cc -lpthread -o server server.c
** Run:       ./server
**            This program is the subserver dispatching portion
**            of a 2 client per subserver game of gomoku
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pthread.h>

#define HOST "server1.cs.scranton.edu" // the hostname of the HTTP server
#define HTTPPORT "32300"               // the HTTP port client will be connecting to
#define BACKLOG 10                     // how many pending connections queue will hold
#define P1 1 // joe added this
void *get_in_addr(struct sockaddr * sa);             // get internet address
int get_server_socket(char *hostname, char *port);   // get a server socket
int start_server(int serv_socket, int backlog);      // start server's listening
int accept_client(int serv_sock);                    // accept a connection from client
void start_subserver(int reply_sock_fd1, int reply_sock_fd2); // start subserver as a thread
void *subserver(void *game);            // subserver - subserver
void print_ip( struct addrinfo *ai);                 // print IP info from getaddrinfo()
int isEmpty(char board[8][8], int row, int col);
void createBoard(char board[8][8]);
int winner(char board[8][8], char player, int row, int col);
int count(char board[8][8], char player, int row, int col, int dirX, int dirY);
void printBoard(char board[8][8]);

struct bothsocks{
	long replyfd1;
	long replyfd2;
};

int main(void){
    int http_sock_fd;			// http server socket
    int reply_sock_fd[2];	// client connection 
    int yes;
    char *msg = "Waiting for another player...\n";
    char *msg2 = "Both players Ready!\n";
    int len,bytes_sent,i,len2;
    len = strlen(msg);
    len2 = strlen(msg2);

    // steps 1-2: get a socket and bind to ip address and port
    http_sock_fd = get_server_socket(HOST, HTTPPORT);

    // step 3: get ready to accept connections
    if (start_server(http_sock_fd, BACKLOG) == -1) {
       printf("start server error\n");
       exit(1);
    }

    while(1) {  // accept() client connections
        // step 4: accept a client connection
        for(i=0; i<2; i++){
	        if((reply_sock_fd[i] = accept_client(http_sock_fd)) == -1) {
                continue;
	        }
        }
        
		start_subserver(reply_sock_fd[0], reply_sock_fd[1]);
    }	
} 


int get_server_socket(char *hostname, char *port) {
    struct addrinfo hints, *servinfo, *p;
    int status;
    int server_socket;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       exit(1);
    }

    for (p = servinfo; p != NULL; p = p ->ai_next) {
       // step 1: create a socket
       if ((server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }
       // if the port is not released yet, reuse it.
       if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         printf("socket option\n");
         continue;
       }
       // step 2: bind socket to an IP addr and port
       if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
           printf("socket bind \n");
           continue;
       }
       break;
    }
    print_ip(servinfo);
    freeaddrinfo(servinfo); // servinfo structure is no longer needed. free it.

    return server_socket;
}

int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}

int accept_client(int serv_sock) {
    int reply_sock_fd = -1;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct sockaddr_storage client_addr;
    char client_printable_addr[INET6_ADDRSTRLEN];

    // accept a connection request from a client
    // the returned file descriptor from accept will be used
    // to communicate with this client.
    if ((reply_sock_fd = accept(serv_sock, 
       (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            printf("socket accept error\n");
    }
    else {// here is for info only, not really needed.
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), 
                          client_printable_addr, sizeof client_printable_addr);
        printf("server: connection from %s at port %d\n", client_printable_addr,
                            ((struct sockaddr_in*)&client_addr)->sin_port);
    }
    return reply_sock_fd;
}

void start_subserver(int reply_sock_fd1, int reply_sock_fd2) {
   pthread_t pthread;
   struct bothsocks *game;
   game = (struct bothsocks *) malloc(sizeof(struct bothsocks));
   printf("%d,%d\n", reply_sock_fd1, reply_sock_fd2);
   game->replyfd1 = reply_sock_fd1;
   game->replyfd2 = reply_sock_fd2;
   if (pthread_create(&pthread, NULL, subserver, (void *)game) != 0) {
      printf("failed to start subserver\n");
   }
   else {
      printf("subserver %ld started\n", (unsigned long)pthread);
   }
}

void *subserver(void *game) {
    struct bothsocks *players;
    players = game;

    int html_file_fd, i, status;
    int read_count = -1;
    int BUFFERSIZE = 256;
    char buffer[BUFFERSIZE+1];
    int len, bytes_sent, bytes_recv;

    long reply_sock_fd_long1 = (long) players->replyfd1;
    long reply_sock_fd_long2 = (long) players->replyfd2;
    int reply_sock_fd1 = (int) reply_sock_fd_long1;
    int reply_sock_fd2 = (int) reply_sock_fd_long2;

    printf("subserver ID = %ld\n", (unsigned long) pthread_self());

    strcpy(buffer, "1");
    bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1), 0);
    strcpy(buffer, "2");
    bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1), 0);

    char board[8][8];
    createBoard(board); // the board needs the '#' symbols
    int row, col;
    int count = 64;
    int win = -1;
    char player, x, y;
    
    while(win != 1 && count > 0){
        bytes_recv = recv(reply_sock_fd1, &x, sizeof(x), 0);//c1 get p1's move's row
        bytes_recv = recv(reply_sock_fd1, &y, sizeof(y), 0);//c2 get p1's move's col
        row = x - '0';
        col = y - '0'; 
        player = 'X';

        while(isEmpty(board, row, col) == 1){  //make sure spot not taken 
            strcpy(buffer, "2");                                           
            bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1), 0);//if taken loop until valid move
            
            bytes_recv = recv(reply_sock_fd1, &x, sizeof(x), 0);   //c3 l: get next row
            bytes_recv = recv(reply_sock_fd1, &y, sizeof(y), 0);   //c3 l: get next col
            row = x - '0';
            col = y - '0';
        }

        board[row][col] = player; //update server copy of board after player 1's move
        //printf("board is assigned\n");
        
        if((win = winner(board, player, row, col)) == 1){ //check for winner
           strcpy(buffer, "1");
           bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1),0); //c3: respond with continue game
           win = 0;
        }
        else{//p1 won, tell players and close subserver
            strcpy(buffer, "0");
            bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1), 0); //c3: respond p1 with game over
            bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1), 0); //tell p2 game over
            bytes_sent = send(reply_sock_fd2, (void *)&row, sizeof(row), 0);//pass p1 move row to p2
            bytes_sent = send(reply_sock_fd2, (void *)&col, sizeof(col), 0);//pass p1 move col to p2
            win = 1;
            exit(0);
        }
        count--;//p1 turn is over
		
        //P2 MOVE STARTS HERE
        strcpy(buffer, "1");//make p2 bypass game over by p1's victory code
        bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1), 0);
        bytes_sent = send(reply_sock_fd2, (void *)&row, sizeof(row), 0); //pass p1 move row to p2
        bytes_sent = send(reply_sock_fd2, (void *)&col, sizeof(col), 0); //pass p1 move col to p2
        strcpy(buffer, "Your turn Player 2: ");                          //c4: initiate p1 move
        bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1), 0);    //c4
        
        bytes_recv = recv(reply_sock_fd2, &x, sizeof(x), 0);//c5: get p2's move's row  
        bytes_recv = recv(reply_sock_fd2, &y, sizeof(y), 0);//c5: get p2's move's col
        row = x - '0';
        col = y - '0';
        player = 'O';
		
        while((isEmpty(board, row, col)) == 1){ //make sure space not taken up                        
            strcpy(buffer, "2"); //request new move
            bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1), 0); 

            bytes_recv = recv(reply_sock_fd2, &x, sizeof(x), 0);//get p2's new row  
            bytes_recv = recv(reply_sock_fd2, &y, sizeof(y), 0);//get p2's new col
            row = x - '0';
            col = y - '0';
        }

        board[row][col] = player;                                            
        
        if((win = winner(board, player, row, col)) == 1){ //c7: check for p2 being winner
           strcpy(buffer, "1"); //no winner
           bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1),0);
           win = 0; //keep playing                                                          
        } 
        else{ //game over, p2 won let players know, update the board
            strcpy(buffer, "0");
            bytes_sent = send(reply_sock_fd2, buffer, (BUFFERSIZE+1), 0); //tell p2 they won
            bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1), 0); //tell p1, p2's winning move
            bytes_sent = send(reply_sock_fd1, (void *)&row, sizeof(row),0);//pass p2 move row
            bytes_sent = send(reply_sock_fd1, (void *)&col, sizeof(col),0);//pass p2 move col
            win = 1;  // game over
            exit(0);
        }                                                         
        count--;// turn over, keep playing 
        
        strcpy(buffer, "1");// arbitrary to get past p1's code if p2 won
        bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1), 0);
        bytes_sent = send(reply_sock_fd1, (void *)&row, sizeof(row), 0); //pass p2's move row to p1
        bytes_sent = send(reply_sock_fd1, (void *)&col, sizeof(col), 0); //pass p2's move col to p1
        strcpy(buffer, "Your turn Player 1: ");                         //c8: tell p1 its their turn
        bytes_sent = send(reply_sock_fd1, buffer, (BUFFERSIZE+1), 0);   //c8
    }

    return NULL;
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

void *get_in_addr(struct sockaddr * sa) {
   if (sa->sa_family == AF_INET) {
      printf("ipv4\n");
      return &(((struct sockaddr_in *)sa)->sin_addr);
   }
   else {
      printf("ipv6\n");
      return &(((struct sockaddr_in6 *)sa)->sin6_addr);
   }
}

int isEmpty(char board[8][8], int row, int col){
   if(board[row][col] == '#'){
      return 0; }
   else{
      return 1; }
}

void createBoard(char board[8][8]) {
   int i, j;
   for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++) {
         board[i][j] = '#';
      }
   }
}

int winner(char board[8][8], char player, int row, int col) {
   if (count( board, player, row, col, 1, 0 ) >= 5)
      return 0;
   if (count( board, player, row, col, 0, 1 ) >= 5)
      return 0;
   if (count( board, player, row, col, 1, -1 ) >= 5)
      return 0;
   if (count( board, player, row, col, 1, 1 ) >= 5)
      return 0;
             
   //Not over
   return 1;
          
}

int count(char board[8][8], char player, int row, int col, int dirX, int dirY) {
   int ct = 1;  // Number of pieces in a row belonging to the player.       
   int r, c;    // A row and column to be examined
          
   r = row + dirX;  // Look at square in specified direction.
   c = col + dirY;
   while ( r >= 0 && r < 13 && c >= 0 && c < 13 && board[r][c] == player ) {
      ct++; // Square is on the board and contains one of the players's pieces.
      r += dirX;  // Go on to next square in this direction.
      c += dirY;
   }
    
   r = row - dirX;  // Look in the opposite direction.
   c = col - dirY;
   while ( r >= 0 && r < 13 && c >= 0 && c < 13 && board[r][c] == player ) {
      // Square is on the board and contains one of the players's pieces.
      ct++;
      r -= dirX;   // Go on to next square in this direction.
      c -= dirY;
   }
   return ct;
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

