/****************************************************************************
 *   $Id:: can.c 3635 2010-06-02 00:31:46Z usb00423                         $
 *   Project: NXP LPC11xx CAN example
 *
 *   Description:
 *     This file contains CAN code example which include CAN 
 *     initialization, CAN interrupt handler, and APIs for CAN
 *     access.
 *
 ****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/

/* CAN_ functions are non scandal specific.

 * A note about transmission: Often what happens in a main loop is a test for a
 * 1 second timer, and then a burst of scandal_send_channels happens. If we try
 * to send more messages than we have transmit buffers, we could fail to send
 * some messages. To solve this problem, we have a transmit buffer. When we
 * call scandal_send_channel, can_send_msg gets called, and eventually CAN_Send
 * gets called. If CAN_Send fails (i.e. there was no free message object), then
 * we store the message temporarily in a CAN_txbuf location. In the main while
 * loop, every iteration calls handle_scandal, which calls can_poll. can_poll
 * will call send_enqueued messages which sends out any enqueued messages.

 * For example: In the steering wheel, we send out wavesculptor commands every
 * 100ms. We also send out about 15 scandal channels every 1s. This means that
 * when we go to send out those 17 messages on the bounary of 1s, our transmit
 * codepath has to be able to handle 17 messages in *very* quick succession.
 * Currently on the LPC11C14, we have 32 message objects, 20 of which are used
 * for message reception. That only leaves 12 for message transmission. We need
 * to handle the case where we send out more than 12 messages in quick
 * succession, and the transmit buffer helps us with this by spreading out the
 * message transmission over multiple main loop iterations using handle_scandal
 * and can_poll.
 */

#include <project/driver_config.h>
#include <project/scandal_config.h>

#include <string.h> /* for memcpy */

#include <arch/can.h>
#include <arch/gpio.h>
#include <arch/timer.h>

#include <scandal/stdmsp430.h>

#include <scandal/can.h>
#include <scandal/error.h>
#include <scandal/timer.h>
#include <scandal/leds.h>

#define RECV_BUFF_DIVIDE 20 /* this gives 0-20 as recv buffers and 21-32 as tx buffers */

uint8_t recv_buf_used[MSG_OBJ_MAX]; /* this will be used to determine if a recv buffer is available */

can_msg CAN_txbuf[CAN_TX_BUFFER_SIZE];
uint8_t tx_buf_start;
uint8_t tx_num_msgs;

/* statistics of all the interrupts */
volatile uint32_t BOffCnt = 0;
volatile uint32_t EWarnCnt = 0;
volatile uint32_t EPassCnt = 0;

uint8_t CANRxDone[MSG_OBJ_MAX]; //Maybe convert to a single uint32 and use bitwise operations?

#if ENABLE_RX_QUEUE
/* Position in RX Buffer queue if queing is enabled (0-31), allows discarding
   of old data if buffer is full. 
*/
int8_t RXQueuePosition; 
#endif

message_object CAN_rxbuf[MSG_OBJ_MAX]; //Input RX buffer

#if CAN_DEBUG
uint32_t CANStatusLog[100];
uint32_t CANStatusLogCount = 0;
#endif

/******************************************************************************
** Function name:		enqueue_message
**
** Descriptions:		
**
** parameters:			Message
** Returned value:		****
**
**
******************************************************************************/
uint8_t enqueue_message(can_msg* msg){
	u08 pos;
	u08 i;
	
	if(tx_num_msgs >= CAN_TX_BUFFER_SIZE)
		return BUF_FULL_ERR;

	pos = (tx_buf_start + tx_num_msgs) & CAN_TX_BUFFER_MASK;

	CAN_txbuf[pos].id = msg->id;

	for (i = 0; i < 8; i++)
		CAN_txbuf[pos].data[i] = msg->data[i];

	CAN_txbuf[pos].length = msg->length;

	tx_num_msgs++;

	return NO_ERR;
}

/******************************************************************************
** Function name:		send_queued_messages
**
** Descriptions:		Send out any enqueued messages
**
** parameters:			None
** Returned value:		*****
**
**
******************************************************************************/

uint8_t send_queued_messages(void){
	can_msg* msg;
	u08 err = NO_ERR;

	if(tx_num_msgs <= 0)
		return (NO_MSG_ERR);

	msg = &(CAN_txbuf[tx_buf_start]);

	err = CAN_Send(0, msg);

	if(err == NO_ERR){
		tx_buf_start = (tx_buf_start + 1) & CAN_TX_BUFFER_MASK;
		tx_num_msgs--;
	}

	return err;
}

/******************************************************************************
** Function name:		CAN_decode_packet
**
** Descriptions:		Get a message out of the buffer and break it into bits
**
** parameters:			Message Number, Message
** Returned value:		None
**
**
******************************************************************************/
void CAN_decode_packet(uint8_t msg_num, can_msg *msg) {
	/* copy the data */
	msg->data[0] = (CAN_rxbuf[msg_num].data[0] & 0x000000FF);
	msg->data[1] = (CAN_rxbuf[msg_num].data[0] >> 8) & 0x000000FF;
	msg->data[2] = (CAN_rxbuf[msg_num].data[1] & 0x000000FF);
	msg->data[3] = (CAN_rxbuf[msg_num].data[1] >> 8) & 0x000000FF;
	msg->data[4] = (CAN_rxbuf[msg_num].data[2] & 0x000000FF);
	msg->data[5] = (CAN_rxbuf[msg_num].data[2] >> 8) & 0x000000FF;
	msg->data[6] = (CAN_rxbuf[msg_num].data[3] & 0x000000FF);
	msg->data[7] = (CAN_rxbuf[msg_num].data[3] >>8)  & 0x000000FF;

	/* copy the id */
	msg->id = CAN_rxbuf[msg_num].id;

	/* copy the length */
	msg->length= CAN_MSG_MAXSIZE;

	/* set the type */
	msg->ext = CAN_rxbuf[msg_num].ext;

#if CAN_UART_DEBUG
	{
		int i = 0;

		if (msg->ext) {
			uint16_t priority;
			uint16_t type;
			uint16_t node_address;
			uint16_t channel_num;

			channel_num  = ((CAN_rxbuf[msg_num].id >> 0)  & 0x03FF);
			node_address = ((CAN_rxbuf[msg_num].id >> 10) & 0x00FF);
			type         = ((CAN_rxbuf[msg_num].id >> 18) & 0x00FF);
			priority     = ((CAN_rxbuf[msg_num].id >> 26) & 0x0007);

			UART_printf("got an ext can message...\n\r");

			UART_printf(" id is               (0x%x)\n\r",(unsigned int) msg->id);
			UART_printf(" priority is         %u\n\r", priority);
			UART_printf(" node_address is     %u\n\r", node_address);
			UART_printf(" message type is     %u\n\r", type);
			UART_printf(" channel_num is      %u\n\r", channel_num);

			for(i = 0; i < 8; i++)
				UART_printf("can_data[%d] = 0x%x\r\n", i, msg->data[i]);

		} else {
			UART_printf("got a std can message...\n\r");
			UART_printf(" id is               (0x%x)\n\r", (unsigned int) msg->id);

			for(i = 0; i < 0; i++)
				UART_printf("can_data[%d] = 0x%x\r\n", i, (unsigned int) msg->data[i]);
		}
	}
#endif

	/* we can now use the receive buffer for another message! */
	CANRxDone[msg_num] = 0;
}

/* Set up a message buffer to receive a particular type of message specified in filter and mask */
void CAN_set_up_filter(uint8_t msg_id, uint32_t filter_mask, uint32_t filter_addr, uint8_t ext) {
#if CAN_UART_DEBUG
  UART_printf("Filter Setup: id:%u msk:%u flt:%u ext:%u\n", msg_id, filter_mask, filter_addr, ext);
#endif
	/* This is what we're changing in the message buffer object */
	LPC_CAN->IF1_CMDMSK = WR | MASK | ARB | CTRL | DATAA | DATAB;

	if (ext == CAN_EXT_MSG) {
		/* set the first filtermask register */
		LPC_CAN->IF1_MSK1 = filter_mask & 0xFFFF;

		/* set the second filtermask register */
		LPC_CAN->IF1_MSK2 = MASK_MXTD | (filter_mask >> 16);

		/* set the first arbitration register */
		LPC_CAN->IF1_ARB1 = filter_addr & 0xFFFF;

		/* set the second arbitration register */
		LPC_CAN->IF1_ARB2 = ID_MTD | ID_MVAL | (filter_addr >> 16); // use this message object, extended

	} else {
		/* set the second filtermask register */
		LPC_CAN->IF1_MSK2 = (filter_mask << 2);

		/* set the second arbitration register */
		LPC_CAN->IF1_ARB2 = ID_MVAL | (filter_addr << 2); // use this message object, extended

	}

	LPC_CAN->IF1_ARB2 &= ~ID_DIR; // receive direction

	LPC_CAN->IF1_MCTRL = UMSK | RXIE | EOB | DLC_MAX;

	LPC_CAN->IF1_DA1 = 0x0000;
	LPC_CAN->IF1_DA2 = 0x0000;
	LPC_CAN->IF1_DB1 = 0x0000;
	LPC_CAN->IF1_DB2 = 0x0000;

	/* Transfer data to message RAM */
	LPC_CAN->IF1_CMDREQ = msg_id + 1;

	/* wait until it's done */
	while( LPC_CAN->IF1_CMDREQ & IFCREQ_BUSY )
		;
}

/* A message has been received, copy the data out of the registers and reorder it for use */
void CAN_MessageProcess( uint8_t MsgNo ) {
	uint32_t MsgID;
	uint32_t *p_add;

	while ( LPC_CAN->IF2_CMDREQ & IFCREQ_BUSY )
		;

	LPC_CAN->IF2_CMDMSK = RD|MASK|ARB|CTRL|INTPND|TREQ|DATAA|DATAB;	
	LPC_CAN->IF2_CMDREQ = MsgNo+1;    /* Start message transfer */

	/* Check new data bit */
	while ( LPC_CAN->IF2_CMDREQ & IFCREQ_BUSY )
		;

	/* where are we storing the message? */
	p_add = (uint32_t *)&CAN_rxbuf[MsgNo];

	if( LPC_CAN->IF2_ARB2 & ID_MTD ) { /* bit 28-0 is 29 bit extended frame */
		*p_add++ = CAN_EXT_MSG;
		/* mask off MsgVal and Dir */ 
		MsgID = (LPC_CAN->IF2_ARB1|((LPC_CAN->IF2_ARB2&0x5FFF)<<16));
	} else {
		*p_add++ = CAN_STD_MSG;
		/* bit 28-18 is 11-bit standard frame */
		MsgID = (LPC_CAN->IF2_ARB2 &0x1FFF) >> 2;
	}

	*p_add++ = MsgID;
	*p_add++ = LPC_CAN->IF2_MCTRL & 0x000F;	// Get Msg Obj Data length

	*p_add++ = LPC_CAN->IF2_DA1;
	*p_add++ = LPC_CAN->IF2_DA2;
	*p_add++ = LPC_CAN->IF2_DB1;
	*p_add++ = LPC_CAN->IF2_DB2;

	return;
}




/******************************************************************************
** Function name:		CAN_IRQHandler
**
** Descriptions:		CAN interrupt handler
**
** parameters:			None
** Returned value:		None
**
**
******************************************************************************/

void CAN_IRQHandler(void) {
	uint32_t canstat = canstat;
	uint32_t can_int, msg_no;

	while ( (can_int = LPC_CAN->INT) != 0 ) {
    
		if ( can_int & CAN_STATUS_INTERRUPT ) { //If a status interrupt
			canstat = LPC_CAN->STAT;
#if CAN_DEBUG
			CANStatusLog[CANStatusLogCount++] = canstat;
#endif
			if ( canstat & STAT_EWARN ) {
				EWarnCnt++;
				return;
			}

			if ( canstat & STAT_BOFF ) {
				BOffCnt++;
                LPC_CAN->CNTL = LPC_CAN->CNTL & (~0x1); //Clear the INIT flag so the CAN controller can keep doing its thing
				return;
			}

		} else { //Otherwise if it is a message object to be processed
            canstat = LPC_CAN->STAT;
			if ( (canstat & STAT_LEC) == 0 ) { /* NO ERROR */
				/* deal with RX only for now. */
				msg_no = can_int & 0x7FFF;
				if ( (msg_no >= 0x01) && (msg_no <= 0x20) ) {
					LPC_CAN->STAT &= ~STAT_RXOK;
					CAN_MessageProcess( msg_no-1 ); //msg_no goes up from 1, msg_no ranges from 0
					CANRxDone[msg_no-1] = TRUE;
          //UART_printf("# = %d ", (msg_no - 1));
				}
			} else {
      /* Should I be here? :o */
                LPC_CAN->STAT = 0; //Clear the status register so we can carry on.
            }
		}
	}
	return;
}


/******************************************************************************
** Function name:		CAN_Init
**
** Descriptions:		initialize CAN controller
**
** parameters:			Baud rate
** Returned value:		None
**
**
******************************************************************************/

void CAN_Init( uint32_t baud ) {
	LPC_SYSCON->PRESETCTRL |= (0x1<<3);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<17);

	/* The USB D- and CAN RX share the dedicated pin. The USB D+ 
	and CAN TX share the dedicated pin. so, no IO configuration is 
	needed for CAN. */
	if ( !(LPC_CAN->CNTL & CTRL_INIT) ) {
		/* If it's in normal operation already, stop it, reconfigure 
		everything first, then restart. */
		LPC_CAN->CNTL |= CTRL_INIT;		/* Default state */
	}

	/* Be very careful with this setting because it's related to
	the input bitclock setting value in CANBitClk. */
	/* popular CAN clock setting assuming AHB clock is 48Mhz:
	CLKDIV = 1, CAN clock is 48Mhz/2 = 24Mhz
	CLKDIV = 2, CAN clock is 48Mhz/3 = 16Mhz
	CLKDIV = 3, CAN clock is 48Mhz/4 = 12Mhz
	CLKDIV = 5, CAN clock is 48Mhz/6 = 8Mhz */

	/* AHB clock is 48Mhz, the CAN clock is 1/3 AHB clock = 16Mhz */
	LPC_CAN->CLKDIV = 0x02;			/* Divided by 3 */

	/* Start configuring bit timing */
	LPC_CAN->CNTL |= CTRL_CCE;
	LPC_CAN->BT = baud;
	LPC_CAN->BRPE = 0x0000;
	/* Stop configuring bit timing */
	LPC_CAN->CNTL &= ~CTRL_CCE;

	/* Initialization finishes, normal operation now. */
	LPC_CAN->CNTL &= ~CTRL_INIT;

	while ( LPC_CAN->CNTL & CTRL_INIT )
		;

	/* Enable the CAN Interrupt */
	NVIC_EnableIRQ(CAN_IRQn);

	/* By default, auto TX is enabled, enable all related interrupts */
	LPC_CAN->CNTL |= (CTRL_IE|CTRL_SIE|CTRL_EIE);
	return;
}

/******************************************************************************
** Function name:		buffer_free
**
** Descriptions:		Returns true of the buffer is BUSY so move on to the next buffer
**
** parameters:			Message Number
** Returned value:		*****
**
**
******************************************************************************/

uint8_t buffer_free(uint8_t msg_num) {
	uint32_t BufferStatus = (((LPC_CAN->TXREQ2) & (0x0000FFFF)) << 16) | ((LPC_CAN->TXREQ1) & (0x0000FFFF));

	/* Return value to be 1 or 0 depending on if the buffer is free or not */
	return ((((uint8_t) (BufferStatus >> (msg_num-1))) & 0x01) == 0);
}


/******************************************************************************
** Function name:		CAN_Send
**
** Descriptions:		Send a message
**
** parameters:			*****, Message
** Returned value:		*****
**
**
******************************************************************************/
int CAN_Send(uint16_t Pri, can_msg *msg) {
	uint32_t tx_addr;
	uint8_t  length = 8;
	int i;

	/* Data is stored in can_msg->data[0-4], timestamp is stored in can_msg->data[4-7] */
	uint32_t can_data;
	uint32_t can_timestamp;

	memcpy(&can_data, msg->data, sizeof(uint32_t));
	memcpy(&can_timestamp, msg->data+sizeof(uint32_t), sizeof(uint32_t));

 	/* find a free message buffer */
	for(i = RECV_BUFF_DIVIDE+1; i < MSG_OBJ_MAX; i++) {

		if (buffer_free(i)) {
			/* we're sending an extended message */
			if (msg->ext == CAN_EXT_MSG) {
				tx_addr = ID_EXT_MASK & msg->id;

				LPC_CAN->IF1_ARB1 = tx_addr & 0x0000FFFF;
				/* extended, outgoing */
				LPC_CAN->IF1_ARB2 = ID_MTD | ((tx_addr >> 16) & 0x00001FFF);

				/* Mxtd: 1, Mdir: 1, Mask is 0x7FF */
				LPC_CAN->IF1_MSK2 = MASK_MXTD | MASK_MDIR | (ID_EXT_MASK >> 16);
				LPC_CAN->IF1_MSK1 = ID_EXT_MASK & 0x0000FFFF;

			/* we're sending a standard message */
			} else if (msg->ext == CAN_STD_MSG) {
				tx_addr = msg->id & ID_STD_MASK;

				LPC_CAN->IF1_ARB2 = (tx_addr << 2) & 0x00001FFF;
				LPC_CAN->IF1_ARB2 &= ~ID_MTD;

				/* Mxtd: 0, Mdir: 1, Mask is 0x7FF */
				LPC_CAN->IF1_MSK2 = MASK_MDIR | (ID_STD_MASK << 2);
				LPC_CAN->IF1_MSK1 = 0x0000;
			}

			/* put the data in the buffer register */
			LPC_CAN->IF1_DA1 = can_data & 0x0000FFFF;
			LPC_CAN->IF1_DA2 = ((can_data & 0xFFFF0000) >> 16);

			/* put the timestamp in the buffer register */
			LPC_CAN->IF1_DB1 = can_timestamp & 0x0000FFFF;
			LPC_CAN->IF1_DB2 = ((can_timestamp & 0xFFFF0000) >> 16);

			LPC_CAN->IF1_ARB2 |= ID_DIR | ID_MVAL;

			/* set the length DLC field and set the transmission request bit */
			LPC_CAN->IF1_MCTRL = UMSK | TXRQ | EOB | (length & DLC_MASK);

			/* write the message object */
			LPC_CAN->IF1_CMDMSK = WR | MASK | ARB | CTRL | DATAA | DATAB;

			LPC_CAN->IF1_CMDREQ = i;

			while ( LPC_CAN->IF1_CMDREQ & IFCREQ_BUSY )
				; /* Waits until IF1 transfer thingy has done its duties */

			return NO_ERR;
		}
	}

	return NO_MSG_ERR;
}

/* 
 * for reference:
 * typedef struct can_msg {
 *   u32 id;
 *   u08 data[CAN_MSG_MAXSIZE];
 *   u08 length;
 * } can_msg;
 */

/******************************************************************************
** 
**  Scandal wrappers
*******************
** * for reference:
** typedef struct can_msg {
**   u32 id;
**   u08 data[CAN_MSG_MAXSIZE];
**   u08 length;
** } can_msg;
** 
******************************************************************************/

/******************************************************************************
** Function name:		init_can
**
** Descriptions:		Initialise - Scandal wrapper
**
** parameters:			None
** Returned value:		None
**
**
******************************************************************************/
void init_can(void) {
	CAN_Init(BITRATE50K16MHZ);
}

/******************************************************************************
** Function name:		can_get_msg
**
** Descriptions:		Get a message from the CAN controller
**
** parameters:			Message
** Returned value:		****
**
**
******************************************************************************/

u08 can_get_msg(can_msg *msg) {
	int i;

	for(i = 0; i < MSG_OBJ_MAX; i++) {
		if (CANRxDone[i] == TRUE) {
			CAN_decode_packet(i, msg);
			return NO_ERR;
		}
	}
	return NO_MSG_ERR;
}

/******************************************************************************
** Function name:		can_send_msg
**
** Descriptions:		Send a message using the CAN controller
**
** parameters:			Message, Priority
** Returned value:		****
**
**
******************************************************************************/
u08 can_send_msg(can_msg *msg, u08 priority) {
    /* FIX BY GEOFFREY, NOT NXP CODE
     * Check if the CAN is in a busoff state which can occur in a specific
     * scenario (can pin connected connected and disconnected in rapid succession
     */
    if((LPC_CAN->STAT & 0x80) != 0) {
        LPC_CAN->CNTL = LPC_CAN->CNTL & (~0x1);
    }
    
    
	/* If we can't send a message right now, enqueue it for later.
	 * handle_scandal will call can_poll every main loop iteration to send any enqueued messages */
	if (CAN_Send((uint16_t)priority, msg) == NO_MSG_ERR)
		return enqueue_message(msg);

	return NO_ERR;

}

/******************************************************************************
** Function name:		can_register_id
**
** Descriptions:	
 *	
 * Register for a message type. Currently, each message that we want to
 * register for is given a specific message buffer. This limits the maximum
 * number of in channels to be 21 - 4 = 17. (the 4 comes from the 4 types of
 * messages that scandal registers for by default). Look in scandal/engine.c
 * for where this function is called to see why. At the moment, I don't think
 * there are any nodes with large numbers of in channels. If we need to deal
 * with this, it can be done in the future.
**
** parameters:			**Mask,  Data, Priority, Ex**
** Returned value:		****
**
**
******************************************************************************/

u08 can_register_id(u32 mask, u32 data, u08 priority, u08 ext) {
	int i;

	NVIC_DisableIRQ(CAN_IRQn);
	LPC_CAN->CNTL &= ~(CTRL_IE|CTRL_SIE|CTRL_EIE);

	for(i = 0; i < RECV_BUFF_DIVIDE; i++) {
		if (!recv_buf_used[i]) {
			CAN_set_up_filter((i+1), mask, data, ext);
			recv_buf_used[i] = 1;
			NVIC_EnableIRQ(CAN_IRQn);
			LPC_CAN->CNTL |= (CTRL_IE|CTRL_SIE|CTRL_EIE);
			return NO_ERR;
		}
	}

	NVIC_EnableIRQ(CAN_IRQn);
	LPC_CAN->CNTL |= (CTRL_IE|CTRL_SIE|CTRL_EIE);
			
	return NO_MSG_ERR;
}

/******************************************************************************
** Function name:		can_baud_rate
**
** Descriptions:		NONE
**
** parameters:			Mode
** Returned value:		****
**
**
******************************************************************************/
u08  can_baud_rate(u08 mode) {
	return 0;
}


/******************************************************************************
** Function name:		can_poll
**
** Descriptions:		send messages
**
** parameters:			None
** Returned value:		None
**
**
******************************************************************************/

void can_poll(void) {
	send_queued_messages();
}

/* *******************
 * End Scandal wrappers
 */
