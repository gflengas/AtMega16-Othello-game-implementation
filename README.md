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
  

The above for default value 2 sec, results in 19531. Because TIMER1 overflows when reaches the price 65535, t_count is given the price 65535-Target= 46004 which will be assigned to TCNT1 each time, the timer restarts. In case another value is given through the command ST <SP> [1-9] <CR> after the case is detected by ΑVR_Reciever and checked that the given value is acceptable, then through the formula, the appropriate value will be input in t_count. For cases where the Target will be greater than 65535, t_count will initially be set to 65535 and at the same time will be calculated t_extend = target-65535. When an overflow occurs, the value 65535-t_extend will be assigned in TCNT1, to measure the extra time and then assign timeout = 1. The timeout is the variable that AVR checks to diagnose whether there has been a time violation or not.

## Gamer Core
  
