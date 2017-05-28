#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void createBoard(char board[8][8]);
void printBoard(char board[8][8]);
int isEmpty(char board[8][8], int row, int col);
int isLegal(int row, int col)
int winner(char board[8][8], char player, int row, int col);
int count(char board[8][8], char player, int row, int col, int dirX, int dirY);

int main(){
   int count = 0;
   int win = 0;
   int col = 0;
   int row = 0;
   int move = 0;
   char board[8][8];
   char player;
   int user = 0;

   createBoard(board);
	
   while(count <64 && win==0){      
      printBoard(board);
      user = count%2 + 1;   /* Select player based on even/odd */
   
      if (user % 2){
         player = 'X'; 
      }
      else{
         player = 'O';
      }
     
      printf("\nPlayer %d, please enter the row and column of "
         "where you want to place your %c: ", user,(user==1)?'X':'O');
         
      scanf("%d %d", &row, &col);
   	 
      while((isEmpty(board, row, col)==1) || (isLegal(row, col)==1){
         
		 printf("That is not a legal move. Please re-enter your move:\n");
         scanf("%d %d", &row, &col);
      }
      
      board[row][col]=player;
    
      if(winner(board, player, row, col) == 0){
         win = user;
      }
      
      count++;
   }
   printBoard(board);
	/* Display result message */
   if(winner == 0)
      printf("\nHow boring, it is a draw\n");
   else
      printf("\nCongratulations, player %d, YOU ARE THE WINNER!\n", user);
   
   return -1; //for server to determine game is over
}

int isEmpty(char board[8][8], int row, int col){
   if(board[row][col] == '#')
      return 0;
   else
      return 1;
}

int isLegal(int row, int col){
    if((row<0) || (row>8) || (col<0) || (col>8)){
       return 1;
    }
    else
       return 0;
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

void createBoard(char board[8][8]) {
   int i, j;
   for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++) {
         board[i][j] = '#';
      }
   }
}

int count(char board[8][8], char player, int row, int col, int dirX, int dirY) {
    // Counts the number of the specified player's pieces starting at
    // square (row,col) and extending along the direction specified by
    // (dirX,dirY).  It is assumed that the player has a piece at
    // (row,col).  This method looks at the squares (row + dirX, col+dirY),
    // (row + 2*dirX, col + 2*dirY), ... until it hits a square that is
    // off the board or is not occupied by one of the players pieces.
    // It counts the squares that are occupied by the player's pieces.
    // Note:  The values of dirX and dirY must be 0, 1, or -1.  At least
    // one of them must be non-zero.
             
   int ct = 1;  // Number of pieces in a row belonging to the player.
          
   int r, c;    // A row and column to be examined
          
   r = row + dirX;  // Look at square in specified direction.
   c = col + dirY;
   while ( r >= 0 && r < 13 && c >= 0 && c < 13 && board[r][c] == player ) {
        // Square is on the board and contains one of the players's pieces.
      ct++;
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
}  // end count()

int winner(char board[8][8], char player, int row, int col) {
        // This is called just after a piece has been played on the
        // square in the specified row and column.  It determines
        // whether that was a winning move by counting the number
        // of squares in a line in each of the four possible
        // directions from (row,col).  If there are 5 squares (or more)
        // in a row in any direction, then the game is won.
            
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
          
}  // end winner()
