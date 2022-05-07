# AtMega16-Othello-game-implementation
Project made for Embedded Systems course
This is a classic othello game implemented to work with AtMega16 using STK 500 and a 10 MHz crystal.
The player has to communicate with the avr, using the commands described at the command set, facing an AI which uses a simple version of minimax algorithm with depth 1. The game will be played on an 8x8 chessboard with the player in black starting first. The coordinates on the horizontal axis will be given by numbers 1-8, while on the vertical by letters A-H. The game ends when there are no empty boxes on the board or when there are vacancies, but they can not be filled by any player. Winner will be the player with the most checkers on the board when the game is over.

## Communication Protocol
To ensure the ability to communicate with both the user and other AVR, it was necessary to define a common communication protocol. For that reason, the following settings were applied: 10MHz crystal usage, speed and protocol of the serial port 9600baud, 8Bits, 1Stop Bit, No Parity, default move time of each player 2 sec.

To facilitate communication between AVR and opponent, the following functions were created:
- **void UART_Init()** communication port initialization.
- **void USART_Transmit( unsigned char data )** information broadcasting. 
- **void USART_Transmit_Str(char data[])** data string broadcasting.
- **void AVR_Reciever(char Data[])** information receiver from pc. 

The main decoding of the messages is done by the function void **AVR_Reciever (char Data [])**, which will process the messages received by the STK500 from the opponent thanks to **USART_RXC_vect** and will perform the necessary
actions. The way it recognizes each command is by comparing each character stored in Data [] during reading, with the corresponding sequence in ASCII. Particular attention was paid to the receiving of **OK <CR>**. To properly deal with this command, **char LastTrasmit[10]** was utilized to store the latest command transmitted by the STK500. When OK <CR> is detected, it checks the content of LastTrasmit [] and compares the possible commands it could have transmitted last and once it finds the right case, it proceeds to the execution of the necessary actions.
  
## Timer implementation
For the timer that keeps the time, in which each player must do their move,
8-bit TIMER0 was initially used, but due to problems that emerged after crystal adjustment, TIMER1 was utilized. Based on use of Overflow Interrupt TIMER1_OVF_vect and using Prescaler 1024, we proceeded in the application of the following formula:
  
![explanation image](https://github.com/gflengas/AtMega16-Othello-game-implementation/blob/master/pictures/1.png)
  
The above for default value 2 sec, results in 19531. Because TIMER1 overflows when reaches the price 65535, t_count is given the price 65535-Target= 46004 which will be assigned to TCNT1 each time, the timer restarts. In case another value is given through the command ST <SP> [1-9] <CR> after the case is detected by ΑVR_Reciever and checked that the given value is acceptable, then through the formula, the appropriate value will be input in t_count. For cases where the Target will be greater than 65535, t_count will initially be set to 65535 and at the same time will be calculated t_extend = target-65535. When an overflow occurs, the value 65535-t_extend will be assigned in TCNT1, to measure the extra time and then assign timeout = 1. The timeout is the variable that AVR checks to diagnose whether there has been a time violation or not.

## Gamer Core
As mentioned in the introduction, the game will be played on an 8x8 board,
implemented by void BoardInit () . The board will be essentially an 8x8 two-dimensional board, volatile uint8_t board [8] [8]. By using BoardInit, all its positions are initialized as <SP> (32). Positions [3,3] and [4,4] take the value W (87) symbolizing White, while [3,4] and [4,3] take the value B (66) for
the Black. The initial Board is shown below:

![explanation image2](https://github.com/gflengas/AtMega16-Othello-game-implementation/blob/master/pictures/2.png)  
  
The implementation of the full function of the game was based on 2 core parts: a) User-AVR communication interface, b) Central do-while loop, which ensures the smooth execution of the players movements.

We implemented a) for Milestone 1 and added the necessary pieces of code to ensure the correct communication of the 2 players. This was achieved mainly by using while-loops, which "stuck" the code until the
corresponding wait variable takes the value 0 after the AVR receives the appropriate message. It is interesting how avr will deal with the movement of the opponent player when he receives the message MV <SP> {[A-H] [1,8]} <CR>, which we will discuss below. b) is the core of the game and in form of pseudocode the primary idea is the following:
```
board_init;
Get Players Color
do:
  if (black player):
    if (enemy's turn):
      if (valid_moves('W')):
        Passes = 0;
        PrintBoard(moves)
        read player's moves
      else:
        passes++
        if(passes<2):
        Ask player to pass
        else:
        Neither Player got a move, Game over
    if (avr's turn):
      if(valid_moves('W')): 
          Passes = 0;
          avr_move('w')
          Moves_Done++
      else:
        passes++
        if(passes<2):
          Avr passes
        else:
          Neither Player got a move, Gameover 
       black player next round
      if (white player):
        (. . .)
while((Moves_Done<64)&&(Passes<2)&&(End_Game!=1)&&(New_Game!=1))
calculate_score()
announce winner
```
          
Each time the black player goes first. If he's your opponent, then as long as he has available moves, which are detected by the function int valid_moves (char turn) and stored in the array moves, avr is waiting for
the player to send him his move, while in case he has no available move, will be waiting for PASS. If both players play PASS one after the other, it means 
there are no other moves available and the game ends. When avr detects
a move, it is processed through the following procedure:
       

It will first read the coordinates in ascii (converting the letter to number) and will transfer them in decimal form to serve our implementation. If the user has not exceeded the allowed time and if these coordinates of the array moves, there is the value V (86 in ascii), then proceeds to the execution of the move. In this way, it is ensured that we have valid motion and time. If there is a violation in one of these conditions, avr sends the appropriate message and waits for its response user.

The **void make_move(int row,int col, char turn)** function was created to execute a move, which depending on the coordinates and the color of the player makes appropriate changes and saves them on the board, and increases moves_done. In the end, he sends Ok, it ends the loop in which we were stuck waiting for the player to move and restarts the timer.If avr is the white player and has moves available then via the **int avr_move (char turn)**, will execute its move.

The following is a brief description of the valid_moves and make_move functions:
- **int valid_moves(char turn):** This function helps to detect the available moves of the player playing in each round. Initially, it empties the moves array, filling it with gaps (Space = 32 ASCII). Then a search starts inside the board. Checks the boxes one by one, if a checker is in a box, then he places the checker in the moves in the same position. But if the box is empty, it checks if any of its neighboring boxes are checkers of the opponent. In case he finds an opponent, a search begins in each direction "stepping" on opponent checkers, until he finds a gap or a player checker. If he finds a gap, then terminates the search in this direction, while if it finds the player's checker sets the box from which it started as an available movement of the player, marking it on the board moves with V.
- **void make_move(int row, int col, char turn):** This function places the player's checker in the given coordinates and starts a search in any direction by "stepping" on rival checkers until he finds a checker of the player. When he finds one, he starts moving upside down until he finds the starting position and changes the opponent's checkers one by one.

The game will terminate when: a) one of the 2 players sends an end game or quit the game, b) there are no seats available on the chessboard, and c) no player has available movement. The checkers will then be counted and the AVR will announce the winner as well through a suitable message, as well as by lighting one of the 3 LEDs that are connected via PORTB: Led1 for AVR win, Led2 for the player win, and Led3 for a draw.        
          
## AVR Tactic 
Regarding the tactics that the AVR will follow, after the proposed algorithms  have been studied (AlphaBeta, Monte Carlo), we chose to implement a variant of its AlphaBeta algorithm. This choice was due to the small size of the dashboard, which enables us to explore all possible options within the small available time we have.
          
In the previous section, we mentioned the **avr_move** function, which executes the move of the AVR. The way of thinking is this: For every available avr move, it asses what would be the best possible answer from the opponent and chooses the one that will have the least benefits for the opponent. To achieve this it saves the board and moves it to temporary arrays and then selects one by one the available moves and executes them. When it executes a move, it calls the valid_moves (Opponent) function to obtain them available moves of the opponent. Then. calculates the opponent's move, which would bring him the best possible score via the int best_move function (char player) and returns the maximum score that the opponent will achieve with the specific AVR move. This score is compared to previous scores and in case it is lower, the coordinates of the avr move are stored, since this move is better than the previous. It then restores the board to the state before executing it and try the next available until it has no moves left. Finally, it restores the board for one last time and perform the best possible move it found.
          
## Execution
The result of the code execution is shown in the images below:
                 
![explanation image3](https://github.com/gflengas/AtMega16-Othello-game-implementation/blob/master/pictures/3.png)
                      
![explanation image4](https://github.com/gflengas/AtMega16-Othello-game-implementation/blob/master/pictures/4.png)
