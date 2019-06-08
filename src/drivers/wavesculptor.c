#include <scandal/engine.h>
#include <scandal/timer.h>
#include <scandal/wavesculptor.h>
#include <scandal/tritium.h>
#include <scandal/can.h>
//scott #include <scandal/stdmsp430.h>
#include <scandal/error.h> //Added
#include <scandal/message.h>

ws_base_callback ws_base_handler_cb = 0;
ws_status_callback ws_status_handler_cb = 0;
ws_bus_callback ws_bus_handler_cb = 0;
ws_velocity_callback ws_velocity_handler_cb = 0;
ws_temp_callback ws_temp_handler_cb = 0;

void scandal_register_ws_base_callback(ws_base_callback cb) {
	can_register_id(CAN_ID_STD_MASK, MC_BASE, 0, CAN_STD_MSG);
	ws_base_handler_cb = cb;
}

void scandal_register_ws_status_callback(ws_status_callback cb) {
	can_register_id(CAN_ID_STD_MASK, MC_STATUS, 0, CAN_STD_MSG);
	ws_status_handler_cb = cb;
}

void scandal_register_ws_bus_callback(ws_bus_callback cb) {
	can_register_id(CAN_ID_STD_MASK, MC_BUS, 0, CAN_STD_MSG);
	ws_bus_handler_cb = cb;
}

void scandal_register_ws_velocity_callback(ws_velocity_callback cb) {
	can_register_id(CAN_ID_STD_MASK, MC_VELOCITY, 0, CAN_STD_MSG);
	ws_velocity_handler_cb = cb;
}

void scandal_register_ws_temp_callback(ws_temp_callback cb) {
	can_register_id(CAN_ID_STD_MASK, MC_HEATSINK_MOTOR_TEMP, 0, CAN_STD_MSG);
	ws_temp_handler_cb = cb;
}

void scandal_handle_ws_message(can_msg *msg) {
	switch(msg->id) {
	 case MC_BASE:
		if (ws_base_handler_cb != 0) {
			group_64 *data = (group_64 *)msg->data;
			ws_base_handler_cb((char *)(data->data_u32[1]), data->data_u32[0],
						sc_get_timer());
		}
		break;

	 case MC_STATUS:
		if (ws_bus_handler_cb != 0) {
			group_64 *data = (group_64 *)msg->data;
			int8_t rcv_err_count =  data->data_u8[7];
			uint8_t tx_err_count =  data->data_u8[6];
			uint16_t active_motor = data->data_u16[2];
			uint16_t err_flags =    data->data_u16[1];
			uint16_t limit_flags =  data->data_u16[0];
			ws_status_handler_cb(rcv_err_count, tx_err_count, active_motor,
						err_flags, limit_flags, sc_get_timer());
		}
		break;

	 case MC_BUS:
		if (ws_bus_handler_cb != 0) {
			group_64 *data = (group_64 *)msg->data;
			ws_bus_handler_cb(data->data_fp[1], data->data_fp[0], sc_get_timer());
		}
		break;

	 case MC_VELOCITY:
		if (ws_velocity_handler_cb != 0) {
			group_64 *data = (group_64 *)msg->data;
			ws_velocity_handler_cb(data->data_fp[1], data->data_fp[0], sc_get_timer());
		}
		break;

	 case MC_HEATSINK_MOTOR_TEMP:
		if (ws_temp_handler_cb != 0) {
			group_64 *data = (group_64 *)msg->data;
			ws_temp_handler_cb(data->data_fp[1], data->data_fp[0], sc_get_timer());
		}
		break;
	}
}


/*

typedef struct _Wavesculptor_Output_Struct{
    uint16_t    BaseAddress;            // Tritium base address for sending messages 
    uint16_t    DriveAddress;           // Tritium base address from which control messages are accepted
    uint16_t    ControlAddress;         // Driver controls address for receiving drive messages
    sc_time_t   lastUpdateTime;         // Last update time of fields, if this is old - our data is stale
    uint32_t    SerialNumber;           // Serial number on both WS20 and WS22 models
    char        TritiumID[4];           // Model identifier string "" on WS20 and "" on WS22
    uint8_t     RXErrorCount;           // Reserved on WS20, CAN RX Error Count on WS22
    uint8_t     TXErrorCount;           // Reserved on WS20, CAN TX Error Count on WS22
    uint16_t    ActiveMotor;            // Active motor model
    uint16_t    ErrorFlags;             // Error flags
    uint16_t    LimitFlags;             // Limit flags
    float       BusCurrent;             // Bus current (A)
    float       BusVoltage;             // Bus voltage (V)
    float       VehicleVelocity;        // Vehicle velocity (m/s)
    float       MotorVelocity;          // Motor angular velocity (rpm)
    float       Phase_1;                // WS20: Phase A Current, WS22: Phase C Current
    float       Phase_2;                // WS20: Phase B Current, WS22: Phase B Current
    float       MotorVoltageVec_Re;     // Motor voltage vector (real part)
    float       MotorVoltageVec_Im;     // Motor voltage vector (imaginary part)
    float       MotorCurrentVec_Re;     // Motor current vector (real part)
    float       MotorCurrentVec_Im;     // Motor current vector (imaginary part)
    float       MotorBEMF_Re;           // Motor Back EMF model (real part)
    float       MotorBEMF_Im;           // Motor Back EMF model (imaginary part)
    float       VoltageRail_1;          // Internal 15V rail
    float       VoltageRail_2;          // WS20: 1.65V rail, WS22: reserved
    float       VoltageRail_3;          // WS20: 2.5V rail, WS22: 3.3V rail
    float       VoltageRail_4;          // WS20: 1.2V rail, WS22: 1.9V rail
    float       FanSpeed;               // WS20: Fan speed (rpm), WS22: reserved
    float       FanDrive;               // WS20: Fan Drive (%), WS22: reserved
    float       HeatsinkTemp;           // Heatsink temperature (Deg C)
    float       MotorTemp;              // Motor temperature (Deg C)
    float       AirInletTemp;           // WS20: Air inlet temperature (Deg C), WS22: reserved
    float       ProcessorTemp;          // Processor temperature (Deg C)
    float       AirOutletTemp;          // WS20: Outlet air temperature (Deg C), WS22: reserved
    float       CapacitorTemp;          // WS20: Capacitor bank temperature (Deg C), WS22: reserved
    float       DCBusAHr;               // Cumilative charge drawn from the bus since reset (AHr)
    float       Odometer;               // Cumilative distance travelled by vehicle since reset (m)
} Wavesculptor_Output_Struct;

//can_msg struct
typedef struct can_mg {
  u32 id;
  u08 data[CAN_MSG_MAXSIZE];
  u08 length;
  u08 ext;
} can_msg;



*/
uint8_t scandal_store_ws_message(can_msg *msg, Wavesculptor_Output_Struct *dataStruct){

    uint8_t  checks = NO_ERR;
    uint16_t baseAddress = msg->id & 0x7E0;
    uint16_t baseOffset  = msg->id & 0x1F;
    //UART_printf("BA %x BO %d\r\n", baseAddress, baseOffset);
    
    /* Make sure the message is NOT extended as this is not a WS message */
    if (msg->ext){
        checks = EXT_ID_ERR; //Extended ID when we weren't expecting one
    }
    
    /* Make certain we're storing information in the right place here */
    if (!(dataStruct->BaseAddress == baseAddress)){
        checks = NO_MSG_ERR; //Wrong or no message error
        UART_printf("WRONG BASE of %d\r\n", dataStruct->BaseAddress);

    }

    if(!checks){
    
        /*
        typedef union _group_64 {
            float data_fp[2];
            unsigned char data_u8[8];
            unsigned short int data_u16[4];
            unsigned long data_u32[2];
        } group_64;
        */
        
        group_64 *data = (group_64 *)msg->data;
        //dataStruct->lastUpdateTime = sc_get_timer();
        
        switch(baseOffset){
        
        case MC_IDENTITY:
            dataStruct->TritiumID[0]=data->data_u8[0];
            dataStruct->TritiumID[1]=data->data_u8[1];
            dataStruct->TritiumID[2]=data->data_u8[2];
            dataStruct->TritiumID[3]=data->data_u8[3];
            dataStruct->SerialNumber=data->data_u32[1];
            /*T088 from u8 0 to 4*/
            //UART_printf("SETTING UP OFFSET ZERO: %c%c%c%c%c%c%c%c\r\n", data->data_u8[0],data->data_u8[1],data->data_u8[2],data->data_u8[3],data->data_u8[4],data->data_u8[5],data->data_u8[6],data->data_u8[7]);
            break;

        case MC_LIMITS:
            dataStruct->RXErrorCount = data->data_u8[7];
            dataStruct->TXErrorCount = data->data_u8[6];
            dataStruct->ActiveMotor  = data->data_u16[2];
            dataStruct->ErrorFlags   = data->data_u16[1];
            dataStruct->LimitFlags   = data->data_u16[0];
            break;
            
        case MC_BUS:
            dataStruct->BusCurrent = data->data_fp[1];
            dataStruct->BusVoltage = data->data_fp[0];
            break;
            
        case MC_VELOCITY:
            dataStruct->VehicleVelocity = data->data_fp[1];
            dataStruct->MotorVelocity = data->data_fp[0];
            dataStruct->lastUpdateTime = sc_get_timer();
            break;
            
        case MC_PHASE:
            dataStruct->Phase_1 = data->data_fp[1];
            dataStruct->Phase_2 = data->data_fp[0];
            break;
            
        case MC_V_VECTOR:
            dataStruct->MotorVoltageVec_Re = data->data_fp[1];
            dataStruct->MotorVoltageVec_Im = data->data_fp[0];
            break;
            
        case MC_I_VECTOR:
            dataStruct->MotorCurrentVec_Re = data->data_fp[1];
            dataStruct->MotorCurrentVec_Im = data->data_fp[0];
            break;
        
        case MC_BEMF_VECTOR:
            dataStruct->MotorBEMF_Re = data->data_fp[1];
            dataStruct->MotorBEMF_Im = data->data_fp[0];
            break;
        
        case MC_RAIL1:
            dataStruct->VoltageRail_1 = data->data_fp[1];
            dataStruct->VoltageRail_2 = data->data_fp[0];
            break;
        
        case MC_RAIL2:
            dataStruct->VoltageRail_3 = data->data_fp[1];
            dataStruct->VoltageRail_4 = data->data_fp[0];
            break;
            
        case MC_FAN:
            dataStruct->FanSpeed = data->data_fp[1];
            dataStruct->FanDrive = data->data_fp[0];
            break;
            
        case MC_TEMP1:
            dataStruct->HeatsinkTemp = data->data_fp[1];
            dataStruct->MotorTemp = data->data_fp[0];
            break;
            
        case MC_TEMP2:
            dataStruct->AirInletTemp = data->data_fp[1];
            dataStruct->ProcessorTemp = data->data_fp[0];
            break;
            
        case MC_TEMP3:
            dataStruct->AirOutletTemp = data->data_fp[1];
            dataStruct->CapacitorTemp = data->data_fp[0];
            break;
            
        case MC_CUMULATIVE:
            dataStruct->DCBusAHr = data->data_fp[1];
            dataStruct->Odometer = data->data_fp[0];
            break;

        default:
            break;
            //scandal_do_scandal_err(err);
        }
    
    }
    
  return checks;
}

/* Returns 1 for valid device (data received in last 5 seconds and time initialised) */
uint8_t check_device_valid(Wavesculptor_Output_Struct *dataStruct){
  
      /* If we have not received a message in the last 5 seconds, don't return a valid type of WS */
    if((dataStruct->lastUpdateTime + 2000) < sc_get_timer()){
      UART_printf("Stale Data \r\n");
      return 0; //FALSE -> not valid device
    }
    
    /* If lastUpdateTime is greater than the scandal time - the value probably hasn't been initialised yet */
    if((dataStruct->lastUpdateTime) > sc_get_timer()){
      UART_printf("Stale Data \r\n");
      return 0; //FALSE -> not valid device
    }
    
    return 1; //if we make it this far, the device is valid
}

int32_t check_device_type(Wavesculptor_Output_Struct *dataStruct){
    
  int32_t device_Type=0;
  
  if(check_device_valid(dataStruct)){
  
    /* Check for WS22 case as that should be more likely */
    if((dataStruct->TritiumID[0]=='T') && (dataStruct->TritiumID[1]=='0') && (dataStruct->TritiumID[2]=='8') && (dataStruct->TritiumID[3]=='8')){
      device_Type = WS_22;
      
    /* Otherwise check if the device is a WS20 device, which is less likely */
    } else if((dataStruct->TritiumID[0]=='T') && (dataStruct->TritiumID[1]=='R') && (dataStruct->TritiumID[2]=='I') && (dataStruct->TritiumID[3]=='a')){
      device_Type = WS_20;
    }
  }

    
  return device_Type;
}

void send_ws_drive_commands(float rpm, float phase_current, float bus_current, Wavesculptor_Output_Struct *dataStruct){
    
    int32_t deviceType;
    
    deviceType = check_device_type(dataStruct);
    
    //UART_printf("Type = %d \r\n", (int) deviceType);
/* Depending on the device type, send the appropriate drive commands to the two wavesculptors */
    if(deviceType == WS_22){
        //(dataStruc  t->ControlAddress + DC_DRIVE_OFFSET) //((dataStruct->ControlAddress) + DC_DRIVE_OFFSET), DC_POWER_OFFSET
        scandal_send_ws_drive_command(((dataStruct->ControlAddress) + DC_DRIVE_OFFSET), rpm, phase_current);
        scandal_send_ws_drive_command(((dataStruct->ControlAddress) + DC_POWER_OFFSET), 0, bus_current);
        //UART_printf("TX 22\t %x\t vel:%1.3f\t iph:%1.3f\t ibus:%1.3f\r\n", dataStruct->ControlAddress, rpm, phase_current, bus_current);

    }else if(deviceType == WS_20){
        float velocity_KMH = rpm*RPM_TO_KMH_MULTIPLIER;
        scandal_send_ws_drive_command(((dataStruct->ControlAddress) + DC_DRIVE_OFFSET), velocity_KMH, phase_current);
        scandal_send_ws_drive_command(((dataStruct->ControlAddress) + DC_POWER_OFFSET), 0, bus_current);
        //UART_printf("TX 20\t %x\t vel:%1.3f\t iph:%1.3f\t ibus:%1.3f\r\n", dataStruct->ControlAddress, velocity_KMH, phase_current, bus_current);
    }  
}


u08 scandal_send_ws_reset(Wavesculptor_Output_Struct *dataStruct) {
  can_msg msg;
  
  msg.id = ((dataStruct->ControlAddress) + 3);
  msg.ext = CAN_STD_MSG;
  
	msg.data[0] = 'E';
	msg.data[1] = 'S';
	msg.data[2] = 'E';
	msg.data[3] = 'R';
	msg.data[4] = 'S';
	msg.data[5] = 'W';
	msg.data[6] = 'T';
	msg.data[7] = 0;

  can_send_msg(&msg, 3);

	return NO_ERR;
}
