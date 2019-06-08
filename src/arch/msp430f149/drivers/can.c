/* --------------------------------------------------------------------------                                 
    MCP2515 Driver
    File name: mcp2510.c
    Author: David Snowdon, David Favaloro, Etienne Le Sueur
    Description: Part of the USBCAN program

    Copyright (C) David Snowdon, 2002. 

    Date: 25-04-2002
   -------------------------------------------------------------------------- */

/* 
 * This file is part of Sunswift MCP2515 Driver.
 * 
 * Sunswift MCP2515 Driver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * Sunswift MCP2515 Driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with Sunswift MCP2515 Driver.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <project/spi_devices.h>

#include <arch/can.h>

#include <scandal/can.h>
#include <scandal/spi.h>
#include <scandal/error.h>

/* project/spi_devices.h must #define MCP2510 in order for this to compile.
   MCP2510 is the identifier of the SPI device to be used with spi_select() */

/* File scope variables */
u32	fil1_data, fil1_mask;
u32	fil2_data, fil2_mask;

/* Buffers */
#if CAN_TX_BUFFER_SIZE > 0
/* Transmit buffer */
can_msg		cantxbuf[CAN_TX_BUFFER_SIZE];
u08			tx_buf_start;
u08			tx_num_msgs;
u08			tx_buf_lock;
#endif

#if CAN_RX_BUFFER_SIZE > 0
/* Receive Buffer */
volatile can_msg	canrxbuf[CAN_RX_BUFFER_SIZE];
volatile u08		rx_buf_start;
volatile u08		rx_num_msgs;
volatile u08		rx_buf_lock;
#endif

/* Prototypes */
u08 enqueue_message(can_msg *msg);
u08 send_queued_messages(void);
u08 buffer_received(void);
void careful_clear_receive_interrupt(u08 flag);

/* -------------------------------------------------------------------------
   MCP2510 Data
   ------------------------------------------------------------------------- */
/* The fields below are set up to correspond to
   the baud rates defined by scandal */
#define CNF1_FIELD 0
#define CNF2_FIELD 1
#define CNF3_FIELD 2
/*! \todo Put in the actual register values for the various bit timings */
/* The bit timings given below were generated using intrepidcs's
   MCP2510 Bit Timing calculation utility.
   http://www.intrepidcs.com/mcp2510 */
/* All these values assume Multiple Bit Sampling and no wake up filter */
u08 bit_timings[SCANDAL_NUM_BAUD][3] = {
  {0x00, 0xD8, 0x01},    /* SCANDAL_B1000 */    /* NB: This is risky due to MCP2510 errata */
  {0x00, 0xF8, 0x05},    /* SCANDAL_B500 */
  {0x01, 0xF8, 0x05},    /* SCANDAL_B250 */
  {0x03, 0xF8, 0x05},    /* SCANDAL_B125 */
  {0x07, 0xFA, 0x07},    /* SCANDAL_B50  */
  {0x1F, 0xFF, 0x07}     /* SCANDAL_B10  */
};

/* CAN Interface Functions 
   brief Initialisation of the CAN hardware
 */
void init_can(void){
	/* This is done a little dodgy hee and needs fixing */
	u32	i;
	u08	value;

	init_spi();

	MCP2510_reset();

	// Need some kind of big delay here
#ifdef __PCM__
	delay_ms(10);
#else
	for(i=0;i<10000;i++)
		__asm__ ("nop");
#endif

	/* Configure the bit timing */
	can_baud_rate(DEFAULT_BAUD);

	/* Configure the interrupts */
	/*! \todo Configure the MCP2510 Interrupt registers */

	/* Set up recieve filters */
	value = 0;
	value |= (MCP2510_RECEIVE_EXTENDED << trRXM00); // SCOTT: this disables filter
	value |= (1 << trFILHIT00); // use acceptance filter 1
	MCP2510_write(RXB0CTRL, &value, 1);

	value = 0;
	value |= (0<<trFILHIT12) | (1<<trFILHIT11) | (0<<trFILHIT10); // use acceptance filter 2
	MCP2510_write(RXB1CTRL, &value, 1);

	/* Set the controller to normal mode */
	MCP2510_set_mode(MCP2510_NORMAL_MODE);

	fil1_data = 0; fil1_mask = 0xFFFFFFFF;
	fil2_data = 0; fil2_mask = 0xFFFFFFFF;

#if CAN_TX_BUFFER_SIZE > 0 
	tx_buf_lock = 0;
	tx_buf_start = 0;
	tx_num_msgs = 0;
#endif

#if CAN_RX_BUFFER_SIZE > 0
	rx_buf_lock = 0;
	rx_buf_start = 0;
	rx_num_msgs = 0;
#endif
	
	value = 0x03; /* interrupt on RX0 and RX1 */
	MCP2510_write(CANINTE, &value, 1);

	enable_can_interrupt();
}

/* Should be called by the host code when the controller generates an interrupt */
void can_interrupt(void){
#if CAN_RX_BUFFER_SIZE > 0
	buffer_received();
#endif
}

/* should be called by the host code when there is some time available to do some background work */
void can_poll(void){
#if CAN_RX_BUFFER_SIZE > 0
	buffer_received();
#endif

#if CAN_TX_BUFFER_SIZE > 0
	send_queued_messages();
#endif
}

/* Get a message from the CAN controller */
u08 can_get_msg(can_msg* msg){
#if CAN_RX_BUFFER_SIZE > 0
	if(rx_num_msgs == 0)
		return (NO_MSG_ERR);

	disable_can_interrupt();
	*msg = canrxbuf[rx_buf_start];
	rx_buf_start = (rx_buf_start + 1) & CAN_RX_BUFFER_MASK;
	rx_num_msgs--;
	enable_can_interrupt();

#else
	return(MCP2510_receive_message(&(msg->id), msg->data, &(msg->length), &(msg->ext)));
#endif
	return(NO_ERR);
}

/* Send a message to the CAN controller */
u08 can_send_msg(can_msg* msg, u08 priority){
#if CAN_TX_BUFFER_SIZE > 0
  u08 err;
  err = enqueue_message(msg);
  send_queued_messages();
  return err;
#else
  return(MCP2510_transmit_message(msg->id, msg->data, msg->length, priority));
#endif
}


u08 can_send_std_msg(can_msg* msg, u08 priority) {
	return(MCP2510_transmit_std_message(msg->id, msg->data, msg->length, priority));
}

#if CAN_TX_BUFFER_SIZE > 0
u08 enqueue_message(can_msg* msg){
	u08 pos;
	u08 i;
	
	if(tx_num_msgs >= CAN_TX_BUFFER_SIZE)
		return BUF_FULL_ERR;

	pos = (tx_buf_start + tx_num_msgs) & CAN_TX_BUFFER_MASK;

	cantxbuf[pos].id = msg->id;
	cantxbuf[pos].ext = msg->ext;
	for(i=0; i<8; i++)
		cantxbuf[pos].data[i] = msg->data[i];
	cantxbuf[pos].length = msg->length;

	tx_num_msgs++;

	return NO_ERR;
}

u08 send_queued_messages(void){
	can_msg* msg;
	u08 err = NO_ERR;

	if(tx_num_msgs <= 0)
		return (NO_MSG_ERR);

	msg = &(cantxbuf[tx_buf_start]);

	disable_can_interrupt();

	switch(msg->ext) {
	 case CAN_STD_MSG:
		err = MCP2510_transmit_std_message(msg->id, msg->data, msg->length, (msg->id >> 21) & 0xFF);
		break;

	 case CAN_EXT_MSG:
		err = MCP2510_transmit_message(msg->id, msg->data, msg->length, (msg->id >> 21) & 0xFF);
		break;
	}

	enable_can_interrupt();

	if(err == NO_ERR){
	    tx_buf_start = (tx_buf_start + 1) & CAN_TX_BUFFER_MASK;
	    tx_num_msgs--;
   	}

	return err;
}
#endif

#if CAN_RX_BUFFER_SIZE > 0
u08 buffer_received(void){
	u08 	pos, err=NO_ERR;
	can_msg* msg;

	while(err == NO_ERR) {
		/* Copy a received message out of mail box 0 */
		if(rx_num_msgs >= CAN_RX_BUFFER_SIZE){
			disable_can_interrupt();
			careful_clear_receive_interrupt(trRX0IF);
			careful_clear_receive_interrupt(trRX1IF);
			enable_can_interrupt();
			return BUF_FULL_ERR;
		} else {
			disable_can_interrupt();
			pos = (rx_buf_start + rx_num_msgs) & CAN_RX_BUFFER_MASK;
			msg = (can_msg*)&(canrxbuf[pos]);
      msg->id=0x69A5;
			err = MCP2510_receive_message(&(msg->id), msg->data, &(msg->length), &(msg->ext));

			if(err == NO_ERR) {
        msg->id++;
        enqueue_message(msg);
        msg->id--;
				rx_num_msgs++; }
			enable_can_interrupt();
		}
	}

	return err;
}
#endif

/* Register an message ID we want to receive
 * On the MCP2515, there are limited filters. So when we register for a new ID, we should just
 * make the filter a superposition of the previous and new filters. */
u08 can_register_id(u32 mask, u32 data, u08 priority, u08 ext){
	u08 buf[4];
	u32 data_diff;

	/* we need to be in configuration mode to modify these registers */
	MCP2510_set_mode(MCP2510_CONFIGURATION_MODE);

	switch (ext) {
	 case CAN_EXT_MSG:
		/* set up the filters for rxbuf0, the EXT buffer for scandal messages */
		data_diff = data ^ fil1_data; /* The bits which are different will be 1, the bits that aren't will be 0 */

		/* If fil1_mask is its original value, set fil1_mask the requested value */
		if(fil1_mask == 0xFFFFFFFF) {
			fil1_data = data;
			fil1_mask = mask;
		} else {
			fil1_mask = (fil1_mask & mask) & (~data_diff);
			fil1_data = data & fil1_mask;
		}

		buf[0] = fil1_data >> 21;
		buf[1] = (((fil1_data >> 18) & 0x07) << 5) | ((u32)1<<EXIDE) | ((fil1_data >> 16) & 0x03);
		buf[2] = ((fil1_data >> 8) & 0xFF);
		buf[3] = ((fil1_data >> 0) & 0xFF);
		MCP2510_write(RXF1SIDH, buf, 4);

		buf[0] = fil1_mask >> 21;
		buf[1] = (((fil1_mask >> 18) & 0x07) << 5) | ((u32)1<<EXIDE) | ((fil1_mask >> 16) & 0x03);
		buf[2] = ((fil1_mask >> 8) & 0xFF);
		buf[3] = ((fil1_mask >> 0) & 0xFF);
		MCP2510_write(RXM0SIDH, buf, 4);

		break;

	 case CAN_STD_MSG:
		/* set up the filters for rxbuf1, the STD buffer for tritium messages */
		data_diff = data ^ fil1_data; /* The bits which are different will be 1, the bits that aren't will be 0 */

		/* If fil1_mask is its original value, set fil1_mask the requested value */
		if (fil2_mask == 0xFFFFFFFF) {
			fil2_data = data;
			fil2_mask = mask;
		} else {
			fil2_mask = (fil2_mask & mask) & (~data_diff);
			fil2_data = data & fil2_mask;
		}

		buf[0] = (fil2_data >> 3);
		buf[1] = ((fil2_data << 5) & (0xE)) | ((u32)0<<EXIDE);
		MCP2510_write(RXF2SIDH, buf, 2);

		buf[0] = (fil2_mask >> 3);
		buf[1] = (fil2_mask << 5) & (0xE);
		MCP2510_write(RXM1SIDH, buf, 2);

		break;
	}

	/* go back into normal mode */
	MCP2510_set_mode(MCP2510_NORMAL_MODE);

	return NO_ERR;
}

/*! Set the baud rate mode to one of the rate modes */
u08 can_baud_rate(u08 mode){
	return(MCP2510_bit_timing(mode));
}

/*
 * MCP2510 Functions
 * NB: All of these functions assume that the SPI driver calls will not fail
 */

/*! Read the MCP2510 TX Error count */
u08 MCP2510_read_tx_errors(void){
	u08 value;
	MCP2510_read(TEC, &value, 1);
	return value;
}

/*! Read the MCP2510 RX Error count */
u08 MCP2510_read_rx_errors(void){
	u08 value;
	MCP2510_read(REC, &value, 1);
	return value;
}

/*! Set the operating mode of the MCP2510 */
void MCP2510_set_mode(u08 mode){
	/* This piece of ugliness is the workaround for a silicon bug
		as suggested in the revision AA errata */

	if(mode == MCP2510_LISTEN_MODE){
		MCP2510_bit_modify(CANCTRL, 0xE0, (MCP2510_LOOPBACK_MODE << 5));
		MCP2510_bit_modify(CANCTRL, 0xE0, (mode << 5));
	}else{
		MCP2510_bit_modify(CANCTRL, 0xE0, (mode << 5));

		/* This piece of ugliness is the workaround for a silicon bug
			as suggested in the errata */
		while(MCP2510_get_mode() != mode){
			MCP2510_bit_modify(CANCTRL,0xE0, (MCP2510_CONFIGURATION_MODE << 5));
			MCP2510_bit_modify(CANCTRL,0xE0, (mode << 5));
		}
	}

}

void MCP2510_set_clkout_mode(u08 mode){
  MCP2510_bit_modify(CANCTRL, 0x03, mode); 
}

/* Retrieve the operating mode of the MCP2510 */
u08 MCP2510_get_mode() {
    u08 value;
    MCP2510_read(CANSTAT, &value, 1);
	return(value >> 5);
}

/* Note: This routine will fail if none of the transmit buffers are available */
/* Transmit a message using the MCP2510 and a free Tx buffer */
u08 MCP2510_transmit_message(u32 id, u08 *buf, u08 size, u08 priority) {
	unsigned char value;
	unsigned char idbuf[8];

	value = MCP2510_read_status();
	if((value & (1<<2)) != 0)    /* If the buffer is pending a transmission, return BUF_FULL_ERR */
		return BUF_FULL_ERR;

	/* In order to comply with the CAN standard, the size (DLC) must
	 be less than or equal to 8 */
	if(size>8)
		size = 8;

	/* Load the ID */
	idbuf[0] = (id >> 21) & 0xFF;
	idbuf[1] = (((id >> 18) & 0x07) << 5) | (1<<EXIDE) | ((id >> 16) & 0x03);
	idbuf[2] = ((id >> 8) & 0xFF);
	idbuf[3] = ((id >> 0) & 0xFF);
	MCP2510_write(TXB0SIDH, idbuf, 4);

	/* Load the data */
	MCP2510_write(TXB0D0, buf, size);

	/* Set the size byte */
	MCP2510_write(TXB0DLC, &size, 1);

	/* Set the priority and flag the buffer to be transmitted */
	MCP2510_bit_modify(TXB0CTRL, TXBNCTRL_TXP_MASK, priority);
	MCP2510_RTS(0x01);

	return(NO_ERR);
}

u08 MCP2510_transmit_std_message(u32 id, u08 *buf, u08 size, u08 priority) {
	unsigned char value;
	unsigned char sid_l;
	unsigned char sid_h;

	value = MCP2510_read_status();
	if((value & (1<<4)) != 0) /* If the buffer is pending a transmission, return BUF_FULL_ERR */
		return BUF_FULL_ERR;

	/* In order to comply with the CAN standard, the size (DLC) must
	 be less than or equal to 8 */
	if(size>8)
		size = 8;

	/* Load the ID */
	sid_h = (id >> 3) & 0xFF; // the id should be 11 bits. Take the high 8
	sid_l = ((id & 0x7) << 5) | (0<<EXIDE); // take the last 3 bits, and make sure that it is set to STD.

	MCP2510_write(TXB1SIDH, &sid_h, 1);
	MCP2510_write(TXB1SIDL, &sid_l, 1);

	/* Load the data */
	MCP2510_write(TXB1D0, buf, size);

	/* Set the size byte */
	MCP2510_write(TXB1DLC, &size, 1);

	/* Set the priority and flag the buffer to be transmitted */
	MCP2510_bit_modify(TXB1CTRL, TXBNCTRL_TXP_MASK, priority);
	MCP2510_RTS(0x02);

	return(NO_ERR);
}

void careful_clear_receive_interrupt(u08  flag){
	u08 value;

	/* This is a hack solution to a pretty nasty silicon bug.
		It needs to be improved such that we don't lose any
		incoming messages */

	value = MCP2510_read_status();

	if(value & (1<<2)){
		/* Clear the TXREQ bit */
		MCP2510_bit_modify(TXB0CTRL, (1<<trTXREQ0), (0<<trTXREQ0));
		/* Read thestatus again */
		value = MCP2510_read_status();
		if(value & (1<<2)){	/* Check to see if the flag is still set, indicating the message is currently being tramitted */
			while( value & (1<<2) ) /* Wait for the bit to be cleared */
				value = MCP2510_read_status();
			/* Once its been cleared, reset the receive flag */
			MCP2510_bit_modify(CANINTF, (1<<flag), 0x00);
		} else {
			/* Clear the receive flag */
			MCP2510_bit_modify(CANINTF, (1<<flag), 0x00);
			/* Re set the tx request */
			MCP2510_bit_modify(TXB0CTRL, (1<<trTXREQ0), (1<<trTXREQ0));
		}
	} else { /* The bit wasn't set at all... Clear the flag */
		MCP2510_bit_modify(CANINTF, (1<<flag), 0x00);
	}
}

u08 MCP2510_receive_message(u32* id, u08* buf, u08* length, u08 *ext){

	u08 value;

	value = MCP2510_read_status();

	/* if we receive an interrupt for RXBUF0, it will be an extended scandal message */
	if(value & (1<<STATUS_RX0IF)) { /* Valid message recieved */
		/* Make sure we haven't suffered an overflow error */
		MCP2510_read(EFLG, buf, 1);

		if(*buf & (1<<trRX0OVR)){
			careful_clear_receive_interrupt(trRX0IF);
			MCP2510_bit_modify(EFLG, (1<<trRX0OVR), 0x00);
			return NO_MSG_ERR;
		}

		/* Copy in the identifier - to be updated to support
		   extended identifiers */
		MCP2510_read(RXB0SIDH, buf, 4);

    /* Check message type */
    if(buf[1] & (1 << EXIDE)){
      (*id) = ((u32)buf[0]) << 21;
      (*id) |= ((u32)(buf[1] >> 5) & 0x07) << 18 ;
      (*id) |= ((u32)(buf[1] & 0x03)) << 16;
      (*id) |= ((u32)buf[2]) << 8;
      (*id) |= ((u32)buf[3]) << 0;
      //(*id) = 0x1ebbddcc;
      (*ext) = 1;
    } else {
      (*id) = ((u32)buf[0]) << 3;
      (*id) |= ((u32)buf[1] & 0x70) >> 5;
      (*ext) = 0;
    }

		/* Read the length */
		MCP2510_read(RXB0DLC, buf, 1);
		*length = buf[0] & 0x0F;

		/* Read length number of bytes from the recieve buffer */
		MCP2510_read(RXB0D0, buf, *length);

		/* The function below clears the receive interrupt in a manner that should
		 * work around the silicon bug detailed in errata item 6 */
		careful_clear_receive_interrupt(trRX0IF);

		return NO_ERR;
	}
//#ifdef STD_ENABLE
#if 1
  /* if we receive an interrupt for RXBUF1, it will be a standard tritium message */
	if(value & (1<<STATUS_RX1IF)) { /* Valid message recieved */
		/* Make sure we haven't suffered an overflow error */
		MCP2510_read(EFLG, buf, 1);

		if(*buf & (1<<trRX1OVR)){
			MCP2510_bit_modify(EFLG, (1<<trRX1OVR), 0x00);
			careful_clear_receive_interrupt(trRX1IF);
			return NO_MSG_ERR;
		}

		/* Copy in the identifier */
		MCP2510_read(RXB1SIDH, buf, 4);

		/* Check message type */
		if(buf[1] & (1 << EXIDE)){
      (*id) = ((u32)buf[0]) << 21;
      (*id) |= ((u32)(buf[1] >> 5) & 0x07) << 18 ;
      (*id) |= ((u32)(buf[1] & 0x03)) << 16;
      (*id) |= ((u32)buf[2]) << 8;
      (*id) |= ((u32)buf[3]) << 0;
      //(*id) = 0x1abbddcc;
      (*ext) = 1;
	  } else {
      (*id) = ((u32)buf[0]) << 3;
      (*id) |= ((u32)buf[1] & 0x70) >> 5;
      (*ext) = 0;
    }

		/* Read the length */
		MCP2510_read(RXB1DLC, buf, 1);
		*length = buf[0] & 0x0F;

		/* Read length bytes from the recieve buffer */
		MCP2510_read(RXB1D0, buf, *length);

		careful_clear_receive_interrupt(trRX1IF);

		return NO_ERR;
	}
#endif
	/* There was no message to be recieved,
	 return an error */
	return NO_MSG_ERR;
}


/*! Reset */
/* Note that the controller won't be available for 128 Fosc cycles */
void MCP2510_reset(void){
	spi_select_device(MCP2510);
	spi_transfer(MCP2510_RESET_COMMAND);
	spi_deselect_all();
}

/*! Read */
/* There must be enough space in the buffer to store the data */
/*! \todo Make the read and write commands use interrupt driven SPI transfers */
/*! \todo Return decent error messages! */
void MCP2510_read(u08	addr, u08* buf, u08 num_bytes){
	unsigned char i;

	spi_select_device(MCP2510);
	spi_transfer(MCP2510_READ_COMMAND);
	spi_transfer(addr);

	for(i = 0; i < num_bytes; i++)
		buf[i] = spi_transfer(0);

	spi_deselect_all();
}

/*! Write */
/*! \todo Make the read and write commands use interrupt driven SPI transfers */
/*! \todo Return decent error messages! */
void MCP2510_write(u08 addr, u08* buf, u08 num_bytes){
	u08 i;

	spi_select_device(MCP2510);
	spi_transfer(MCP2510_WRITE_COMMAND);
	spi_transfer(addr);

	for(i = 0; i < num_bytes; i++)
		spi_transfer(buf[i]);

	spi_deselect_all();
}

/*! Request to Send
 *
 *  Sends the data in one of the transmit buffers
 */
void MCP2510_RTS(u08 buf_bitfield){
	spi_select_device(MCP2510);
	spi_transfer(MCP2510_RTS_COMMAND | buf_bitfield );
	spi_deselect_all();
}

/*! Read Status
 *
 *  Returns the status register
 */
u08 MCP2510_read_status(){
	u08 value;

	spi_select_device(MCP2510);
	spi_transfer(MCP2510_READSTATUS_COMMAND);
	value = spi_transfer(0);
	spi_deselect_all();

	return(value);
}

/*! Bit modify */
/*  Modify the bits of a particular register using a mask and a data field */
/*  \todo Modify the bit modify command to use interrupt drive SPI */
void MCP2510_bit_modify(u08 addr, u08 mask, u08 data){
	spi_select_device(MCP2510);
	spi_transfer(MCP2510_BITMODIFY_COMMAND);
	spi_transfer(addr);
	spi_transfer(mask);
	spi_transfer(data);
	spi_deselect_all();
}

/*! Set the bit timing (Baud Rate) */
u08 MCP2510_bit_timing(u08 mode){
	MCP2510_write(CNF1, &(bit_timings[mode][CNF1_FIELD]), 1);
	MCP2510_write(CNF2, &(bit_timings[mode][CNF2_FIELD]), 1);
	MCP2510_write(CNF3, &(bit_timings[mode][CNF3_FIELD]), 1);
	return NO_ERR;
}
