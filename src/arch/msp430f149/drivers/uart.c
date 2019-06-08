#include <scandal/types.h>

#include <project/hardware.h>

#include <iomacros.h>
#include <signal.h>
//scott #include <msp430x14x.h>
#include <msp430.h>

static char * tx_buf_start;
static char * tx_buf_ptr;
static int tx_buf_size;
volatile int uart0_free;
static char   received_byte; 
volatile char   is_received; 

void UART_baud_rate(u32 rate, u32 clock_speed);
void UART_flush_tx(void);

/* Send the next RS232 byte when ready */
wakeup interrupt (UART0TX_VECTOR) 
uart0_tx_int(void) {
	if (tx_buf_ptr < tx_buf_start + tx_buf_size) {
		TXBUF0 = *tx_buf_ptr++;
	} else {
		uart0_free = 1;
	}
}

interrupt (UART0RX_VECTOR)
uart0_rx_int(void) {
	received_byte = RXBUF0;
	is_received = 1; 
}

void UART_Init(uint32_t baudrate) {
	UCTL0 = SWRST | CHAR; /* Leave in reset mode, set to 8 bit data (8N1) */
	UTCTL0 = SSEL1; /* SMCLK */

	/* This is a workaround as the UART seems to 
	 * automatically generate an interrupt when interrups are enabled
	 */
	tx_buf_ptr = 0;
	tx_buf_start = 0;
	tx_buf_size = 0;
	uart0_free = 1;

	/* See mspgcc.sourceforge.net/baudrate.html */
	UART_baud_rate(115200, CLOCK_SPEED); 
	
	ME1 |= UTXE0 | URXE0; /* Module enable Tx and Rx */
/*	P3OUT |= UTXD0; */
	/* Set USART0 Tx high */
/*	P3DIR |= UTXD0; */
	/* Set USART0 Tx to output */
/*	P3SEL |= UTXD0 | URXD0; */
	/* Set USART0 pins to peripheral mode */

	
	UCTL0 &= ~SWRST; /* take out of reset */
	IE1 |= UTXIE0 | URXIE0; /* Enable interrupts for USART0 */
}

int UART_tx(char *buf, int buf_len) {
	if (uart0_free) {
		uart0_free = 0;
		/* dint ?*/
		tx_buf_ptr = buf;
		tx_buf_start = buf;
		tx_buf_size = buf_len;
		TXBUF0 = *tx_buf_ptr++;
		return 0;
		/* restore interrupts ?*/
	}
	/* else */
	return 1;
}

void UART_SendByte(u08 Data){
	UART_flush_tx();
	UART_tx((char *)&Data, 1);
}

void UART_flush_tx(void) {
  while(uart0_free != 1)
    ;
}

u08 UART_ReceiveByte(void){
  while(!is_received)
    nop();

  is_received = 0;

  return received_byte; 
}

u08 UART_CheckReceived(void){
	return is_received;
}

u08 UART_is_received(void){
	return is_received;
}

void UART_baud_rate(u32 rate, u32 clock_speed){
  if(clock_speed != 7372800)
    return; 

  UART_flush_tx(); 

  //  UCTL0 |= SWRST; /* Put into reset */

  switch(rate){
  case 300:
    UBR00=0x00; UBR10=0x60; UMCTL0=0x00; /* uart0 7372800Hz 1200bps */
    break; 

  case 600:
    UBR00=0x00; UBR10=0x30; UMCTL0=0x00; /* uart0 7372800Hz 1200bps */
    break; 

  case 1200:
    UBR00=0x00; UBR10=0x18; UMCTL0=0x00; /* uart0 7372800Hz 1200bps */
    break; 

  case 2400:
    UBR00=0x00; UBR10=0x0c; UMCTL0=0x00; /* uart0 7372800Hz 2400bps */
    break; 

  case 4800:
    UBR00=0x00; UBR10=0x06; UMCTL0=0x00; /* uart0 7372800Hz 4800bps */
    break; 

  case 9600:
    /* Clock = 7.37Mhz, Baud Rate = 9600bps, Divisor = 768 */
    UBR00=0x00; UBR10=0x03; UMCTL0=0x00;
    break; 

  case 38400:
    UBR00=0xC0; UBR10=0x00; UMCTL0=0x00; /* uart0 7372800Hz 38400bps */
    break;

  case 115200:
    /* Clock = 7.37Mhz, Baud Rate = 115200bps, Divisor = 64 */
    UBR00=0x40; UBR10=0x00; UMCTL0=0x00;
    break; 
  }

  //  UCTL0 &= ~SWRST; /* take out of reset */
}

void UART_putchar(char c) {
	UART_tx(&c, 1);
}
