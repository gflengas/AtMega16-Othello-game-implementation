/*
 * OthelloGame.c 
 * Project for Embedded Systems 
 * Created: 4/8/2019 
 * Author : Georgios Flengas, Alexandros Chatzipetros
 */ 

#include <avr/io.h>
#define F_CPU 1000000UL
#define BAUD 9600	//set the baud rate as 9600 bits per second
#define MYUBRR 64 //(F_CPU/16/BAUD-1) //
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

char buf[256],LastTrasmit[10];
uint8_t reader=0;
uint8_t writer=0;

volatile uint8_t t_count;
volatile uint8_t t_limit;
volatile uint8_t timeout;
volatile uint8_t t_extend;
volatile uint8_t extended;

volatile uint8_t board[8][8];
volatile uint8_t moves[8][8];
char OtherPlayer;
volatile uint8_t Moves_Done;
volatile uint8_t Passes;
volatile uint8_t End_Game;
volatile uint8_t New_Game;
volatile uint8_t wait,wait2,avrmovedone;
volatile uint8_t badmove;
volatile uint8_t avr_score;
volatile uint8_t user_score; 
void UART_Init();
void USART_Transmit( unsigned char data );
void USART_Transmit_Str(char data[]);
void AVR_Reciever(char Data[]);
void BoardInit();
void PrintBoard(uint8_t table[8][8]);
void calculate_score();
int valid_moves(char turn);
void make_move(int row, int col, char turn);
int FirstMove(char turn);
void avr_move(char turn);
int best_move(char player);
int get_score(char turn);

int main(void)
{
	uint8_t CurrentPlayer=1;
    UART_Init();
	UCSRB |= (1 << RXCIE); // Enable the USART Receive Complete interrupt (USART_RXC)
	DDRB |= (1 << PB1) | (1 << PB2) | (1 << PB3); //Connect the leds that we are going to use
	PORTB = 0xFF; 
	t_count = 46004;
	timeout=0;
	t_extend=0;
	extended=0;
	t_limit=0 ;//default,can be change with the appropriate command this is ovf count for 2 secs 
	//Prescaler 1024
	TCCR1B |= (1<<CS12) | (1<<CS10);
	TCNT1 = t_count; // 2 sec default case
	//Enable timer overflow interrupt TOIE0 inside the TIMSK Register
	TIMSK = (1<<TOIE1);
	sei();

    while (1) 
    { 
		//initalise new game
		Moves_Done=4;
		Passes=0;
		End_Game=0;
		New_Game=0;
		wait=0;
		wait2=0;
		avrmovedone=0;
		avr_score = user_score = 0; 
		BoardInit();
		PrintBoard(board);
		wait=1;
		USART_Transmit_Str("Pick Black/White\r");
		while(wait!=0){
			
		}
		TCNT1 = t_count;
		timeout=0;
		t_extend=0;
		extended=0;
		//Main loop where we play the game until its over,ended or ask for a new game
		do{
			//we keep looping between players that play the game, code to be implemented for next phase 
			if (CurrentPlayer==1)
			{
				if (OtherPlayer=='B') {//case player is black 
					if(valid_moves('B')){
						Passes = 0;
						PrintBoard(moves);
						USART_Transmit_Str("Type your move\r");
						wait=1;
						while(wait!=0){}
						while(badmove!=0){} //invalid move reply OK to move on
						
					} else{ // No valid moves
					
						if(++Passes<2){
							USART_Transmit_Str("You have to pass\r");
							wait=1;
							while(wait!=0){}
						}else{
							USART_Transmit_Str("Neither of us have available move, game over\r");
             				End_Game=1;
						}
					}
				}else{ // avr is black
					if(valid_moves('B')){ // Check for valid moves 
          				Passes = 0;// Reset pass count
           				avr_move('B');
           				Moves_Done++; //Increment move count 
						avrmovedone=1;
						while(avrmovedone!=0){} //avr move reply OK to move on
					}else{
           				if(++Passes<2){
							   USART_Transmit_Str("MP\r");//AVR has no available move so it passes 
								 LastTrasmit[0]=77;
							   LastTrasmit[1]=80;
							   LastTrasmit[2]=13;
								 wait2=1;
								 while(wait2!=0){}
						}else{
							USART_Transmit_Str("Neither of us have available move, game over\r");
             				End_Game=1;
						}
         			}
				}
				CurrentPlayer++;
			}
			else if (CurrentPlayer==2)
			{
				if (OtherPlayer=='W') {//case player is white 
					if(valid_moves('W')){
						Passes = 0;
						PrintBoard(moves);
						USART_Transmit_Str("Type your move\r");
						wait=1;
						while(wait!=0){}
						while(badmove!=0){} //invalid move reply OK to move on
					} else{ // No valid moves
					
						if(++Passes<2){
							USART_Transmit_Str("You have to pass\r");
							wait=1;
							while(wait!=0){}
						}else{
							USART_Transmit_Str("Neither of us have available move, game over\r");
             				End_Game=1;
						}
					}
				}else{ // avr is white
					if(valid_moves('W')){ // Check for valid moves 
          				Passes = 0;// Reset pass count
           				avr_move('W');
						avrmovedone=1;
					    while(avrmovedone!=0){} //avr move reply OK to move on
           				Moves_Done++; //Increment move count 
					}else{
           				if(++Passes<2){
							   USART_Transmit_Str("MP\r");//AVR has no available move so it passes 
							   LastTrasmit[0]=77;
							   LastTrasmit[1]=80;
							   LastTrasmit[2]=13;
								 wait2=1;
								 while(wait2!=0){}
						}else{
							USART_Transmit_Str("Neither of us have available move, game over\r");
             				End_Game=1;
						}
         			}
				}
				CurrentPlayer--;
			}
		}while((Moves_Done<64)&&(Passes<2)&&(End_Game!=1)&&(New_Game!=1));
		
		PrintBoard(board);
		calculate_score();
		//Open led,Transmit and set LastTrasmit
		if(avr_score>user_score){//avr wins 
			PORTB ^= (1<<PB1);
			USART_Transmit_Str(" WN\r");
			LastTrasmit[0]=87;
			LastTrasmit[1]=78;
			LastTrasmit[2]=13;
			wait=1;
			while(wait!=0){}
		}else if(avr_score<user_score){//user wins
			PORTB ^= (1<<PB2);
			USART_Transmit_Str(" LS\r");
			LastTrasmit[0]=76;
			LastTrasmit[1]=83;
			LastTrasmit[2]=13;
			wait=1;
			while(wait!=0){}
		}else{ //tie
			PORTB ^= (1<<PB3);
			USART_Transmit_Str(" TE\r");
			LastTrasmit[0]=84;
			LastTrasmit[1]=69;
			LastTrasmit[2]=13;
			wait=1;
			while(wait!=0){}
		}
    }
}

ISR(USART_RXC_vect)
{
	buf[writer] = UDR;

	
	if (buf[writer]== 13)
	{
		AVR_Reciever(buf);
	}
	writer++;
	if (writer >=256)
	{
		writer=0;
	}
	
}

ISR (TIMER1_OVF_vect){
	if (t_limit<65535){
		timeout=1;
	}else{
		if(extended==0){
			TCNT1=65535-t_extend;
			extended=1;
		}else{
			timeout=1;
		}		
	}
	
}


void UART_Init()
{
	// Load upper 8-bits of the baud rate value into the high byte of the UBRR register
	UBRRH = (unsigned char)(MYUBRR >> 8);
	// Load lower 8-bits of the baud rate value into the low byte of the UBRR register
	UBRRL = (unsigned char)MYUBRR;
	
	// Turn on the transmission and reception circuitry
	UCSRB |= (1 << RXEN) | (1 << TXEN);
	// Use 8-bit character sizes
	UCSRC |=  (1<<URSEL) | (1 << UCSZ0) | (1 << UCSZ1);
}

void USART_Transmit( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) )
	;
	/* Put data into buffer, sends the data */
	UDR = data;
}

void USART_Transmit_Str(char data[]){
	

	for (uint8_t i = 0 ; i < strlen(data)  ; i++ ){
		while(!(UCSRA & (1 << UDRE)))
		;
		UDR = data[i];
	}

}

void AVR_Reciever(char Data[])
{//have to adjust for buffer overflow on writer how reader reacts
	int sec,X,Y;
    if ((Data[reader] == 65)&&(Data[reader+1] == 84)&&(Data[reader+2] == 13)) // AT<CR>
	{
		USART_Transmit_Str("OK\r");
	}
	else if ((Data[reader] == 82)&&(Data[reader+1] == 83)&&(Data[reader+2] == 84)&&(Data[reader+3] == 13)) //RST<CR>
	{
		//Reset (Warm Start)
		USART_Transmit_Str("OK\r");
		BoardInit();//Board is reseted 
		PORTB = 0xFF;//Leds are toggled off
		wait=0;
		wait2=0;
		avrmovedone=0;
		PrintBoard(board);
	}
	else if ((Data[reader] == 83)&&(Data[reader+1] == 80)&&(Data[reader+2] == 32)) //SP<SP>{B,W}<CR>
	{
		if (Data[reader+3]== 87) {
			OtherPlayer='W';
		}else
		{
			OtherPlayer='B';
		}
		wait=0;
		USART_Transmit_Str("\r");
	}
	else if ((Data[reader] == 78)&&(Data[reader+1] == 71)&&(Data[reader+2] == 13)) // NG<CR>
	{
		New_Game=1;
		USART_Transmit_Str("OK\r");
	}
	else if ((Data[reader] == 69)&&(Data[reader+1] == 71)&&(Data[reader+2] == 13)) // EG<CR>
	{
		End_Game=1;
		USART_Transmit_Str("OK\r");
	}
	else if ((Data[reader] == 83)&&(Data[reader+1] == 84)&&(Data[reader+2] == 32)) //ST<SP>[1,9]<CR>
	{
		//Set Time make sure its[1,9] also set default to 2 sec 
		//$$Ftimer = CPU Frequency/Prescaler $$ $$Ftimer = 16MHz/1024 = 15.625KHz $$ $$Ttick = 1/ 15.625K = 64 \mu seconds$$ $$Ttotal = 64\mu s X 255 = 16ms$$
		//Ftimer=10000000/1024=9765.625, then Ttick=1/39062.5=0.0001024 then Ttotal=255*0.0001024 and Ovf_Count=SETTIME/Ttotal= SETTIME/0.026112
		//timer1 Ftimer=10000000/1024=9765.625, then Ttick=1/39062.5=0.0001024 SETTIME/Ttick=2/0.0001024=19531 so 65535-19531=46004 set TCNT1= 46004 and once we ovf timeout
		sec=(Data[reader+3]- '0');
		if ((sec>=1)&&(sec<=9))
		{
			t_limit=(Data[reader+3]- '0')/0.0001024;
			if (t_limit<65535)
			{
				t_extend=0;
				t_count=65535-t_limit;
			}else{
				t_extend=t_limit-65535;
				t_count=65535;
			}
			USART_Transmit_Str("OK\r");
		}
		else{
			USART_Transmit_Str("WRONG VALUE set default\r");
		}
	}
	else if ((Data[reader] == 77)&&(Data[reader+1] == 86)&&(Data[reader+2] == 32)) //MV<SP>{[A-H][1,8]}<CR>
	{
		//Add flag to make sure its pc's move 
		//Read opponent move 1st a letter and then ( ,=44) a number check limits and submit to board if valid
		//-17
		Y=(Data[reader+3]- '0')-17;
		X=(Data[reader+4]- '0')-1;
		if(timeout==0){
			if (moves[X][Y]==86) {
				make_move(X,Y,OtherPlayer);
				PrintBoard(board);
				Moves_Done++;
			}else
			{
				//invalid move 
				USART_Transmit_Str("IL\r");
				LastTrasmit[0]=73;
				LastTrasmit[1]=76;
				LastTrasmit[2]=13;
				badmove=1;
				
			}
		}else{
			//time is up. my move 
			USART_Transmit_Str("IT\r");
			LastTrasmit[0]=73;
			LastTrasmit[1]=84;
			LastTrasmit[2]=13;
			badmove=1;
			
		}
		USART_Transmit_Str("OK\r");
		//restart timer here, next players move 
		wait=0;
		TCNT1 = t_count;
		extended=0;
		timeout=0;
	}
	else if ((Data[reader] == 80)&&(Data[reader+1] == 83)&&(Data[reader+2] == 13)) // PS<CR>
	{
		//PC got no move avr's turn flag on 
		Passes++;
		USART_Transmit_Str("OK\r");
		wait=0;
		TCNT1 = t_count;
		extended=0;
		timeout=0;
	}
	else if ((Data[reader] == 79)&&(Data[reader+1] == 75)&&(Data[reader+2] == 13)) // OK<CR>
	{
		//Reply Case based on what avr transmitted last 
		if ((LastTrasmit[0] == 77 )&&(LastTrasmit[1] == 80)&&(LastTrasmit[2] == 13)) { //MP<CR>
			//My Pass, answer after avr declares that it has no move 
			wait2=0;
			TCNT1 = t_count;
			extended=0;
			timeout=0;
		}
		else if ((LastTrasmit[0] == 87 )&&(LastTrasmit[1] == 78)&&(LastTrasmit[2] == 13)) { //WN<CR>
			//I Win, answer after avr declares that its the winner open led 1
			wait=0;
			
		}
		else if ((LastTrasmit[0] == 76 )&&(LastTrasmit[1] == 83)&&(LastTrasmit[2] == 13)) { //LS<CR>
			//I Lose, answer after avr declares that its the loser open led 2
			wait=0;
			
		}
		else if ((LastTrasmit[0] == 84 )&&(LastTrasmit[1] == 69)&&(LastTrasmit[2] == 13)) { //ΤΕ<CR>
			//Tie, answer after avr declares that its a tie open led 3
			wait=0;
			
		}
		else if ((LastTrasmit[0] == 81 )&&(LastTrasmit[1] == 84)&&(LastTrasmit[2] == 13)) { //QT<CR>------------------------------------
			//I Quit, answer after avr declares that its quitting 
			End_Game=1;
		}
		else if ((LastTrasmit[0] == 77 )&&(LastTrasmit[1] == 77)&&(LastTrasmit[2] == 32)) { //MM<SP>{[A-H],[1-8]}<CR>
			//Answer after My Move is sent from avr
			//restart timer here next player's move 
			avrmovedone=0;
			TCNT1 = t_count;
			extended=0;
			timeout=0;
		}
		else if ((LastTrasmit[0] == 73 )&&(LastTrasmit[1] == 76)&&(LastTrasmit[2] == 13)) { //IL<CR>
			//Illegal Move Accepted by opponent
			badmove=0;
		}
		else if ((LastTrasmit[0] == 73)&&(LastTrasmit[1] == 84)&&(LastTrasmit[2] == 13)) { //IT<CR>
			//Illegal Time Accepted by opponent
			badmove=0;	
		}
	}
	else if ((Data[reader] == 80)&&(Data[reader+1] == 76)&&(Data[reader+2] == 13)) // PL<CR>
	{
		//Reply Case based on what avr transmitted last  
		if ((LastTrasmit[0] == 73 )&&(LastTrasmit[1] == 76)&&(LastTrasmit[2] == 13)) { //IL<CR>
			//Illegal Move Rejected by opponent
			badmove=0;
		}
		else if ((LastTrasmit[0] == 73)&&(LastTrasmit[1] == 84)&&(LastTrasmit[2] == 13)) { //IT<CR>
			//Illegal Time Rejected by opponent
			badmove=0;
		}
	}
	else{ //Unknown case we print back and <CR>
		for( int j= reader; j<writer ; j++)
		{
			USART_Transmit(Data[j]);
		}
		USART_Transmit(13);
		USART_Transmit_Str("ER\r");
		
	}
	reader=writer+1;//reader point to next data that we ll put into the buffer
}

void BoardInit(){
	  /* Blank all the board squares */    
     for(int row = 0; row < 8; row++)
       for(int col = 0; col < 8; col++)
         board[row][col] = 32;//Space for empty Spaces

     /* Place the initial four counters in the center */
     board[3][3] = board[4][4] = 87; //W for White
     board[3][4] = board[4][3] = 66; //B for Black

}

void PrintBoard(uint8_t table[8][8]){

   USART_Transmit(13);//Start top line
   for(int col = 0 ; col<8 ;col++)
   {
	   USART_Transmit(32);
	   USART_Transmit(32);
	   USART_Transmit(32);
	   USART_Transmit(65+col);// Display the top line 
   }
   USART_Transmit(13);// End the top line

   for(int row = 0; row<8; row++)//Display the intermediate rows
   {
		USART_Transmit(32);
		USART_Transmit(43);
	    for(int col = 0; col<8; col++)
		{
			USART_Transmit(45);
			USART_Transmit(45);
			USART_Transmit(45);
			USART_Transmit(43);	// ---+
		}
		USART_Transmit(13);
		USART_Transmit(49+row); //row number
    	USART_Transmit(124);


     	for(int col = 0; col<8; col++){
			USART_Transmit(32);
			USART_Transmit(table[row][col]);
			USART_Transmit(32);
			USART_Transmit(124);
	 	}
		USART_Transmit(13);  
   }
   USART_Transmit(32);
   USART_Transmit(43);
   for(int col = 0 ; col<8 ;col++)
   {
			USART_Transmit(45);
			USART_Transmit(45);
			USART_Transmit(45);
			USART_Transmit(43);	// ---+
	}//Display the bottom line 
    USART_Transmit(13);// End the bottom  line
}

void calculate_score(){
	char transmit_data[10];
	if (OtherPlayer=='W')
	{
		user_score=get_score('W');
		avr_score=get_score('B');
	} 
	else
	{
		user_score=get_score('B');
		avr_score=get_score('W');
	}
	USART_Transmit_Str("The final score is:");
	USART_Transmit_Str("AVR: ");
	USART_Transmit_Str((char*)itoa(avr_score,transmit_data,10));
	USART_Transmit_Str(" User: ");
	USART_Transmit_Str((char*)itoa(user_score,transmit_data,10));
	USART_Transmit(13);
}

int valid_moves(char turn){
   int x = 0;
   int y = 0;
   int no_of_moves = 0;  //Number of valid moves
   
   int opponent = (turn == 'W')? 66 : 87; //Set the opponent
   int player = (turn == 'W')? 87 : 66;
   //Initialize moves
   for(int row = 0; row < 8; row++)
     for(int col = 0; col < 8; col++)
       moves[row][col] = 32;//space

   //A valid move must be on a blank square and must enclose at least one opponent square between two player squares 
   for(int row = 0; row < 8; row++)
     for(int col = 0; col < 8; col++){
       if(board[row][col] != 32){ //check for blank space
         moves[row][col]=board[row][col];//if not set current and move to next
		 continue; 
		}
       //Check all the squares around the blank square for the opponents counter
       for(int rowdelta = -1; rowdelta <= 1; rowdelta++)
         for(int coldelta = -1; coldelta <= 1; coldelta++){ 
           //Don't check outside the board, or the current square
           if(row + rowdelta < 0 || row + rowdelta >= 8 ||col + coldelta < 0 || col + coldelta >= 8 || (rowdelta==0 && coldelta==0))
             continue;

           if(board[row + rowdelta][col + coldelta] == opponent){
             /* If we find the opponent, move in the delta direction
             over opponent counters searching for a player counter */
             x = row + rowdelta;
             y = col + coldelta;
			
             // Look for a player square in the delta direction 
             for(;;){
               x += rowdelta;                  // Go to next square in delta direction
               y += coldelta;                  
				
               // Abort if we move out of bound 
               if(x < 0 || x >= 8 || y < 0 || y >= 8) 
			   	break;

               // Abort if we find a blank square
               if(board[x][y] == 32) 
			   	break;
                //  If the square has a player counter then we have a valid move 
				   
               if(board[x][y] == player){
                 moves[row][col] = 86;   // Mark as valid V
                 no_of_moves++;         // Increase valid moves count 
                 break;                 // Go check another square
               }
             } 
           } 
         }  
     }
   return no_of_moves; 
}

void make_move(int row, int col, char turn){
   int x = 0; 
   int y = 0; 
   int opponent = (turn == 'W')? 66 : 87; //Set the opponent
   int player = (turn == 'W')? 87 : 66;

   board[row][col] = player;

   //Check all the squares around this square for the opponents counter                
   for(int rowdelta = -1; rowdelta <= 1; rowdelta++)
     for(int coldelta = -1; coldelta <= 1; coldelta++){ 
       //Don't check off the board, or the current square
       if(row + rowdelta < 0 || row + rowdelta >= 8 || col + coldelta < 0 || col + coldelta >= 8 || (rowdelta==0 && coldelta== 0))
         continue;

       // Check
       if(board[row + rowdelta][col + coldelta] == opponent){
         // If we find the opponent, search in the same direction for a player counter 
         x = row + rowdelta;        
         y = col + coldelta;      

         for(;;){
           x += rowdelta;
           y += coldelta;  

           // Abort if we move out of bound 
           if(x < 0 || x >= 8 || y < 0 || y >= 8)
             break;
 
           // Abort if we find a blank square
           if(board[x][y] == 32)
             break;

           // If we find the player counter, go backwards from here changing all the opponents counters to player       
           if(board[x][y] == player)
           {
             while(board[x-=rowdelta][y-=coldelta]==opponent) //change every opponent counter until you find a player counter
               board[x][y] = player;
             break;
           } 
         }
       }
     }
}

int FirstMove(char turn){
	for(int row = 0; row < 8; row++){
     for(int col = 0; col < 8; col++){
       if(moves[row][col] == 86){ //valid move
         make_move(row,col,turn);
				 LastTrasmit[0]=77;
				 LastTrasmit[1]=77;
				 LastTrasmit[2]=32;
		 return 1;
	   }
	 }
	}
	return 0;
}

void avr_move(char turn){
   int best_row = 0;  // Best row index         
   int best_col = 0;  // Best column index  
   int new_score = 0; // Score for current move  
   int score = 64; //Minimum opponent score
   uint8_t temp_board[8][8]; //copy of board 
   uint8_t av_moves[8][8]; // Local valid moves array 
   char opponent = (turn == 'W')? 'B' : 'W'; //Set the opponent

	//create copy of board and moves
	for(int i = 0; i < 8; i++)
         for(int j = 0; j < 8; j++){
           temp_board[i][j] = board[i][j];
					 av_moves[i][j] = moves[i][j];					
				 }
   // Go through all valid moves 
   for(int row = 0; row < 8; row++)
     for(int col = 0; col < 8; col++){
       if(av_moves[row][col] != 86) //go next if not valid move block
         continue;
 
       // Restore board 
       for(int i = 0; i < 8; i++)
         for(int j = 0; j < 8; j++)
           board[i][j] = temp_board[i][j];
   
       //Try this move 
       make_move(row, col, turn); 

       // find valid moves for the opponent after this move
       valid_moves(opponent);

       //find the score for the opponents best move 
       new_score = best_move(opponent);

       if(new_score<score){                     
         score = new_score;  
         best_row = row; 
         best_col = col;  
       }
     }
	// Restore board 
   for(int i = 0; i < 8; i++)
    for(int j = 0; j < 8; j++)
     board[i][j] = temp_board[i][j];

   // Make the best move
   make_move(best_row, best_col, turn); 
	 LastTrasmit[0]=77;
	 LastTrasmit[1]=77;
	 LastTrasmit[2]=32;
	 LastTrasmit[3]=best_col+65 ; //add to turn to letter
	 LastTrasmit[4]=best_row+49; //add to turn to number
	 LastTrasmit[5]=13;
	 USART_Transmit_Str(LastTrasmit);
}

int best_move(char player){
	 int new_score = 0; // Score for current move  
   int score = 0; //Maximum opponent score
   uint8_t temp_board[8][8]; //copy of board 

   //create copy of board 
	for(int i = 0; i < 8; i++)
         for(int j = 0; j < 8; j++){
           temp_board[i][j] = board[i][j];				
				 }
   // Go through all valid moves 
   for(int row = 0; row < 8; row++)
     for(int col = 0; col < 8; col++){
       if(moves[row][col] != 86) //go next if not valid move block
         continue;

       // Restore board 
       for(int i = 0; i < 8; i++)
         for(int j = 0; j < 8; j++)
           board[i][j] = temp_board[i][j];

       //Try this move 
       make_move(row, col, player);  
       new_score = get_score(player);  

       if(score<new_score)         
        score = new_score;  
     }
   return score;                  
}

int get_score(char turn){
   int score = 0; 
   int player = (turn == 'W')? 87 : 66;

   /* Check all board squares */
   for(int row = 0; row < 8; row++)
     for(int col = 0; col < 8; col++){ 
			 if (board[row][col] == player)
			 {
				 score++;
			 }
  	 }
   return score;     
}
