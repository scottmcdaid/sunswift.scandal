/* --------------------------------------------------------------------------
	Scandal Engine
	File name: scandal_engine.c
	Author: David Snowdon
 
    Copyright (C) David Snowdon, 2002. 

	Date: 31/07/02
   -------------------------------------------------------------------------- */

/* 
 * This file is part of Scandal.
 * 
 * Scandal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * Scandal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with Scandal.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <project/scandal_config.h>

#include <arch/uart.h>
#include <arch/wdt.h> 
 
#include <scandal/engine.h>
#include <scandal/types.h>
#include <scandal/timer.h>
#include <scandal/error.h>
#include <scandal/eeprom.h>
#include <scandal/devices.h>
#include <scandal/message.h>
#include <scandal/uart.h>
#include <scandal/utils.h>
#include <scandal/wavesculptor.h>
#include <scandal/system.h>



in_channel                  in_channels[NUM_IN_CHANNELS];
in_channel_handler          in_channel_handlers[NUM_IN_CHANNELS];
standard_message_handler    user_std_msg_handler;
uint32_t                    user_std_msg_handler_set = 0;

scandal_config  my_config;
volatile u32    heartbeat_timer;
uint64_t        timesync_offset; 

/* Local Prototypes */
void            do_first_run(void);
static void 	scandal_handle_channel_overrides();

u08             handle_ext_message(can_msg*	msg);
u08             handle_std_message(can_msg*	msg);
inline u08      scandal_handle_channel(can_msg* msg);
inline u08      scandal_handle_config(can_msg* msg);
inline u08      scandal_handle_reset(can_msg* msg);
inline u08      scandal_handle_user_config(can_msg* msg);
inline u08      scandal_handle_command(can_msg* msg);
inline u08      scandal_handle_timesync(can_msg* msg);

void            set_channel_mb(u16 chan_num, s32 m, s32 b);
void            retrieve_channel_mb(u16 chan_num);

u08             scandal_get_msg_type(can_msg *msg);
u08             scandal_get_msg_priority(can_msg *msg);


/* Functions */
u08 scandal_init(void){
	u16 i;

	timesync_offset = 0; 
    
#if !DISABLE_WATCHDOG_TIMER
    /* Initialising the WDT with either user defined period or the default of 5000ms
       Default is defined in engine.h
    */
    WDT_Init(WATCHDOG_TIMER_PERIOD);
#endif

	init_can();
	sc_init_timer();
	sc_init_eeprom();

	scandal_delay(100);

	/* Initialise the local address
		determine if this is first run or not */

	sc_read_conf(&my_config);

	if(my_config.version != SCANDAL_VERSION)
               do_first_run();
	
    /* Handle channel overrides from scandal configuration */
	scandal_handle_channel_overrides();

	/* Set up infrastructure for the in-channels */
	for(i=0; i<NUM_IN_CHANNELS; i++){
		/* Zero out the in_channel's value and time */
		in_channels[i].value = 0;
		in_channels[i].rcvd_time = 0;
		in_channels[i].time = 0;
		in_channel_handlers[i] = 0;
		/* Register the ID */
		u32 id = scandal_mk_channel_id(0, my_config.ins[i].source_node,
								my_config.ins[i].source_num);
		can_register_id(0x03FFFFFF,
				id,
				0,
				CAN_EXT_MSG);
	}

#if !DISABLE_CONFIG_MESSAGES
	/* Register for my config messages */
	can_register_id(0x03FFFF00,
			scandal_mk_config_id( 0, scandal_get_addr(), 0),
			0, CAN_EXT_MSG);
#endif

#if !DISABLE_USER_CONFIG_MESSAGES	
	/* Register for user config messages */
	can_register_id(0x03FFFF00,
			scandal_mk_user_config_id( 0, scandal_get_addr(), 0),
			0, CAN_EXT_MSG);
#endif

#if !DISABLE_TIMESYNC_MESSAGES
	/* Register for timesync messages */ 
	can_register_id(0x03FFFF00, 
			scandal_mk_timesync_id(CRITICAL_PRIORITY), 
			0, CAN_EXT_MSG); 
#endif

#if !DISABLE_COMMAND_MESSAGES
	/* Register for command messages */ 
	can_register_id(0x03FFFF00, 
			scandal_mk_command_id(CRITICAL_PRIORITY, scandal_get_addr(), 0), 
			0, CAN_EXT_MSG); 
#endif

	heartbeat_timer = 0;

	return(0);

}

s32 scandal_get_m(u16 chan_num)
{
	return my_config.outs[chan_num].m;
}

s32 scandal_get_b(u16 chan_num)
{
	return my_config.outs[chan_num].b;
}

void scandal_set_m(u16 chan_num, s32 value)
{
	my_config.outs[chan_num].m = value;
	sc_write_conf(&my_config); 
}

void scandal_set_b(u16 chan_num, s32 value)
{
	my_config.outs[chan_num].b = value;
	sc_write_conf(&my_config); 
}

void scandal_register_in_channel_handler(int chan_num, in_channel_handler handler) {
	in_channel_handlers[chan_num] = handler;
}


/* Lets the user code define a standard message handler which is given all the
   standard CAN messages that are not handled by scandal. This might be useful
   for receiving messages from other CAN devices such as motor controllers or
   maximum power point trackers that might be from a different vendor
   
   Once set, user_std_msg_handler_set is flagged, indicating that we have a
   handler and all subsequent standard messages will 
   */

void register_standard_message_handler(standard_message_handler handler){
    user_std_msg_handler = handler;
    user_std_msg_handler_set = 1;
}


s32 scandal_get_in_channel_value(u16 chan_num){
	return(in_channels[chan_num].value);
}

sc_time_t scandal_get_in_channel_rcvd_time(u16 chan_num){
	return(in_channels[chan_num].rcvd_time);
}

sc_time_t scandal_get_in_channel_time(u16 chan_num){
	return(in_channels[chan_num].time);
}

u08 scandal_in_channel_is_valid(u16 chan_num){
	if(chan_num >= NUM_IN_CHANNELS)
		return 0;
	/* Channel is invalid if it was last updated at t=0, since
		that is impossible, and is the default condition */
	return(scandal_get_in_channel_rcvd_time(chan_num) != 0);
}

in_channel* scandal_get_in_channel(u16 chan_num){
	return(&in_channels[chan_num]);
}

u08 scandal_get_addr(void){
	return(my_config.addr);
}

/*! \todo To be implemented */
u32 scandal_get_mac(void){
	return(1);
}


/* Handle Scandal - to be called regularly (assumed to be once in the main loop)
	Will do nothing in the case where there is nothing to do */
void handle_scandal(void){
	u08  	err;
	can_msg	msg;

	can_poll();

	/* Check weather we're due to send a heartbeat, and if so, send it */
	if(sc_get_timer() - heartbeat_timer >= HEARTBEAT_PERIOD){
		scandal_send_heartbeat(0);	/*! \todo Send a more useful status */
		heartbeat_timer = sc_get_timer();
	}

	/* Check for pending messages */
	err = can_get_msg(&msg);
	switch(err){
	case NO_MSG_ERR:
		break;

	case NO_ERR:
		if (msg.ext)
			handle_ext_message(&msg);
		else
			handle_std_message(&msg);

		break;

	default:
		scandal_do_scandal_err(err);
	}
    
    WDT_Feed();
}

/* this is most likely to be a wave sculptor message */
u08	handle_std_message(can_msg*	msg){
    if(user_std_msg_handler_set){
        standard_message_handler handler = user_std_msg_handler;
		handler(msg);
    } else {
        scandal_handle_ws_message(msg);
    }
	return NO_ERR;
}
/* Local Functions */
u08	handle_ext_message(can_msg*	msg){
  switch(scandal_get_msg_type(msg)){
  case TIMESYNC_TYPE: 
	  scandal_handle_timesync(msg); 
	  break; 
	  
  case CHANNEL_TYPE:
	  scandal_handle_channel(msg);
	  break;
	  
  case CONFIG_TYPE:
	  scandal_handle_config(msg);
	  break;
	  
  case RESET_TYPE:
	  scandal_handle_reset(msg);
	  break;
	  
  case HEARTBEAT_TYPE:
  case USER_ERROR_TYPE:
  case SCANDAL_ERROR_TYPE:
	  break;   
	  
  case USER_CONFIG_TYPE:
	  scandal_handle_user_config(msg);
	  break; 
	  
  case COMMAND_TYPE:
	  scandal_handle_command(msg); 
	  break; 
	  
  }
  
  scandal_user_handle_message(msg);
  
  return NO_ERR;
}

void	do_first_run(void){
	u16 	i;

	my_config.version = SCANDAL_VERSION;
	my_config.addr = 0;

	for(i=0;i<NUM_IN_CHANNELS;i++){
		my_config.ins[i].source_node = 0;
		my_config.ins[i].source_num = 0;
	}

	for(i=0; i<NUM_OUT_CHANNELS; i++){
		my_config.outs[i].m = DEFAULT_M;
		my_config.outs[i].b = DEFAULT_B;
	}

	sc_write_conf(&my_config);
	scandal_user_do_first_run();
}

static void scandal_handle_channel_overrides(void){

#if SCANDAL_ADDRESS_OVERRIDE_ENABLE
	my_config.addr = SCANDAL_ADDRESS_OVERRIDE;
#endif

#if (NUM_IN_CHANNELS > 0)
#if SCANDAL_IN_CHANNEL_0_OVERRIDE_ENABLE
	my_config.ins[0].source_node = SCANDAL_IN_CHANNEL_0_OVERRIDE_ADDRESS;
	my_config.ins[0].source_num = SCANDAL_IN_CHANNEL_0_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 1)
#if SCANDAL_IN_CHANNEL_1_OVERRIDE_ENABLE
	my_config.ins[1].source_node = SCANDAL_IN_CHANNEL_1_OVERRIDE_ADDRESS;
	my_config.ins[1].source_num = SCANDAL_IN_CHANNEL_1_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 2)
#if SCANDAL_IN_CHANNEL_2_OVERRIDE_ENABLE
	my_config.ins[2].source_node = SCANDAL_IN_CHANNEL_2_OVERRIDE_ADDRESS;
	my_config.ins[2].source_num = SCANDAL_IN_CHANNEL_2_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 2)
#if SCANDAL_IN_CHANNEL_3_OVERRIDE_ENABLE
	my_config.ins[3].source_node = SCANDAL_IN_CHANNEL_3_OVERRIDE_ADDRESS;
	my_config.ins[3].source_num = SCANDAL_IN_CHANNEL_3_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 4)
#if SCANDAL_IN_CHANNEL_4_OVERRIDE_ENABLE
	my_config.ins[4].source_node = SCANDAL_IN_CHANNEL_4_OVERRIDE_ADDRESS;
	my_config.ins[4].source_num = SCANDAL_IN_CHANNEL_4_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 5)
#if SCANDAL_IN_CHANNEL_5_OVERRIDE_ENABLE
	my_config.ins[5].source_node = SCANDAL_IN_CHANNEL_5_OVERRIDE_ADDRESS;
	my_config.ins[5].source_num = SCANDAL_IN_CHANNEL_5_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 6)
#if SCANDAL_IN_CHANNEL_6_OVERRIDE_ENABLE
	my_config.ins[6].source_node = SCANDAL_IN_CHANNEL_6_OVERRIDE_ADDRESS;
	my_config.ins[6].source_num = SCANDAL_IN_CHANNEL_6_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 7)
#if SCANDAL_IN_CHANNEL_7_OVERRIDE_ENABLE
	my_config.ins[7].source_node = SCANDAL_IN_CHANNEL_7_OVERRIDE_ADDRESS;
	my_config.ins[7].source_num = SCANDAL_IN_CHANNEL_7_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 8)
#if SCANDAL_IN_CHANNEL_8_OVERRIDE_ENABLE
	my_config.ins[8].source_node = SCANDAL_IN_CHANNEL_8_OVERRIDE_ADDRESS;
	my_config.ins[8].source_num = SCANDAL_IN_CHANNEL_8_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 9)
#if SCANDAL_IN_CHANNEL_9_OVERRIDE_ENABLE
	my_config.ins[9].source_node = SCANDAL_IN_CHANNEL_9_OVERRIDE_ADDRESS;
	my_config.ins[9].source_num = SCANDAL_IN_CHANNEL_9_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 10)
#if SCANDAL_IN_CHANNEL_10_OVERRIDE_ENABLE
	my_config.ins[10].source_node = SCANDAL_IN_CHANNEL_10_OVERRIDE_ADDRESS;
	my_config.ins[10].source_num = SCANDAL_IN_CHANNEL_10_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 11)
#if SCANDAL_IN_CHANNEL_11_OVERRIDE_ENABLE
	my_config.ins[11].source_node = SCANDAL_IN_CHANNEL_11_OVERRIDE_ADDRESS;
	my_config.ins[11].source_num = SCANDAL_IN_CHANNEL_11_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 12)
#if SCANDAL_IN_CHANNEL_12_OVERRIDE_ENABLE
	my_config.ins[12].source_node = SCANDAL_IN_CHANNEL_12_OVERRIDE_ADDRESS;
	my_config.ins[12].source_num = SCANDAL_IN_CHANNEL_12_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 13)
#if SCANDAL_IN_CHANNEL_13_OVERRIDE_ENABLE
	my_config.ins[13].source_node = SCANDAL_IN_CHANNEL_13_OVERRIDE_ADDRESS;
	my_config.ins[13].source_num = SCANDAL_IN_CHANNEL_13_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 14)
#if SCANDAL_IN_CHANNEL_14_OVERRIDE_ENABLE
	my_config.ins[14].source_node = SCANDAL_IN_CHANNEL_14_OVERRIDE_ADDRESS;
	my_config.ins[14].source_num = SCANDAL_IN_CHANNEL_14_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 15)
#if SCANDAL_IN_CHANNEL_15_OVERRIDE_ENABLE
	my_config.ins[15].source_node = SCANDAL_IN_CHANNEL_15_OVERRIDE_ADDRESS;
	my_config.ins[15].source_num = SCANDAL_IN_CHANNEL_15_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_IN_CHANNELS > 16)
#if SCANDAL_IN_CHANNEL_16_OVERRIDE_ENABLE
	my_config.ins[16].source_node = SCANDAL_IN_CHANNEL_16_OVERRIDE_ADDRESS;
	my_config.ins[16].source_num = SCANDAL_IN_CHANNEL_16_OVERRIDE_CHANNEL;
#endif
#endif

#if (NUM_OUT_CHANNELS > 17)
#if SCANDAL_IN_CHANNEL_16_OVERRIDE_ENABLE
	my_config.ins[17].source_node = SCANDAL_IN_CHANNEL_17_OVERRIDE_ADDRESS;
	my_config.ins[17].source_num = SCANDAL_IN_CHANNEL_17_OVERRIDE_CHANNEL;
#endif
#endif
}

/* Functions for accessing features of messages */
u08		scandal_get_msg_type(can_msg*	msg){
	return (u08)((msg->id >> TYPE_OFFSET) & 0xFF);
}

u08		scandal_get_msg_priority(can_msg* msg){
	return ((u08)((msg->id >> PRI_OFFSET) * 0x07));
}

/* Functions for handling various types of messages */
u08	scandal_handle_channel(can_msg* msg){
	u16 i;
	u08	node;
	u16	num;

	int32_t  value;
	uint32_t time;

	node 	= (msg->id >> CHANNEL_SOURCE_ADDR_OFFSET) & 0xFF;
	num	= (msg->id >> CHANNEL_NUM_OFFSET) & 0x03FF;

	/* We don't accept messages from node 0 - its too easy for
		these messages to be generated erroneously, plus
		the default setup is all zeros */
	if(node == 0)
		return NO_ERR;

	for(i=0;i<NUM_IN_CHANNELS;i++){
		if((my_config.ins[i].source_node == node) &&
			(my_config.ins[i].source_num == num)){

			value = 0;
			value |= ((u32)(msg->data[0]) << 24);
			value |= ((u32)(msg->data[1]) << 16);
			value |= ((u32)(msg->data[2]) << 8);
			value |= ((u32)(msg->data[3]) << 0);

			time = 0;
			time |= (u32)msg->data[4] << 24;
			time |= (u32)msg->data[5] << 16;
			time |= (u32)msg->data[6] << 8;
			time |= (u32)msg->data[7] << 0;

			in_channels[i].value = value;
			in_channels[i].time = time;
			in_channels[i].rcvd_time = sc_get_timer();
			
			if (in_channel_handlers[i] != 0) {
				in_channel_handler handler = in_channel_handlers[i];
				handler(value, time);
			}

		}
	}

	return NO_ERR;
}  

u08 scandal_handle_user_config(can_msg* msg){
	u08	dest_node;
	u08	param;

	dest_node = (u08)((msg->id >> CONFIG_NODE_ADDR_OFFSET) & 0xFF);
	param = (u08)((msg->id >> CONFIG_PARAM_OFFSET) & 0xFF);

	if(dest_node != scandal_get_addr())
		return NO_ERR;

	return scandal_user_do_config(param, 
									FIRST_32_BITS(msg), 
									SECOND_32_BITS(msg));
}

u08	scandal_handle_config(can_msg* msg){
	u08	dest_node;
	u08	param;
	u16	num;

	dest_node = (u08)((msg->id >> CONFIG_NODE_ADDR_OFFSET) & 0xFF);
	param = (u08)((msg->id >> CONFIG_PARAM_OFFSET) & 0xFF);

	if(dest_node != scandal_get_addr())
		return NO_ERR;
 
	switch(param){
	case CONFIG_ADDR:
		/* 0 is the configuration broadcast address */
		my_config.addr = msg->data[0];
		sc_write_conf(&my_config);
		break;

	case CONFIG_IN_CHAN_SOURCE:
		num = ((u16)((msg->data[0]&0xFF) << 8)) | ((u16)msg->data[1]);
		my_config.ins[num].source_node = msg->data[2];
		my_config.ins[num].source_num = ((u16)msg->data[3]<<8) | (msg->data[4]);
		sc_write_conf(&my_config);
		break;

	case CONFIG_OUT_CHAN_M:
		num = ((u16)((msg->data[0]&0xFF) << 8)) | ((u16)msg->data[1]);
		my_config.outs[num].m = (u32)msg->data[2] << 24;
		my_config.outs[num].m |= (u32)msg->data[3] << 16;
		my_config.outs[num].m |= (u32)msg->data[4] << 8;
		my_config.outs[num].m |= (u32)msg->data[5] << 0;
		sc_write_conf(&my_config);
		break;

	case CONFIG_OUT_CHAN_B:
		num = ((u16)((msg->data[0]&0xFF) << 8)) | ((u16)msg->data[1]);
		my_config.outs[num].b = (u32)msg->data[2] << 24;
		my_config.outs[num].b |= (u32)msg->data[3] << 16;
		my_config.outs[num].b |= (u32)msg->data[4] << 8;
		my_config.outs[num].b |= (u32)msg->data[5] << 0;
		sc_write_conf(&my_config);
		break;
	}

	system_reset();
	return NO_ERR;
}

scandal_config getconfig(void){
	return(my_config);
}

u08 scandal_handle_reset(can_msg* msg){
	u08	dest_node;

	dest_node = (u08)((msg->id >> RESET_NODE_ADDR_OFFSET) & 0xFF);

	if(dest_node == scandal_get_addr())
		system_reset();				/* Should not return from this */

	return NO_ERR;
}

/* Functions for handling various types of messages */
u08	scandal_handle_timesync(can_msg* msg){
    uint64_t timestamp; 
    uint32_t first, second; 

    first = FIRST_32_BITS(msg); 
    second = SECOND_32_BITS(msg); 
    timestamp = ((uint64_t)first) << 32 | (uint64_t)second; 

    scandal_set_realtime(timestamp); 

    return NO_ERR; 
}

/* Functions for handling various types of messages */
u08	scandal_handle_command(can_msg* msg){
	u08	node;
	u16	num;

	/* Messages are to us */ 
	node 	= (msg->id >> COMMAND_DEST_ADDR_OFFSET) & 0xFF;
	num	= (msg->id >> COMMAND_NUM_OFFSET) & 0x03FF;

	/* Check it is addressed to us */ 
	if(node != scandal_get_addr())
	  return NO_ERR; 

	switch(num){
	  /* Handle any Scandal commands here -- dump config, for example? */ 
	  
	default:
	  scandal_user_handle_command(num, msg->data); 
	}

	return NO_ERR;
}
