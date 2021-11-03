
#include <stdint.h>
#include "can.h"
#include "can_user.h"
#include "hardware.h"


// abstract:
// Here we will convert a classical CAN message to a CAN-FD message.


// identifier is needed by PEAK-Flash.exe -> do not delete
const char Ident[] __attribute__ ((used)) = { "PCAN-Router_FD"};


// variables for LED toggle
static uint8_t LED_toggleCAN1;
static uint8_t LED_toggleCAN2;

uint8_t crcTable[256];


uint8_t CRCSAECalc(uint8_t * buf, uint8_t len) {
	const uint8_t * ptr = buf;
	uint8_t _crc = 0xFF;
	while(len--) _crc = crcTable[_crc ^ *ptr++];
	return ~_crc;
}

void CRCSAEInit(void) {
	uint8_t _crc;
	for (int i = 0; i < 0x100; i++) {
		_crc = i;
		for (uint8_t bit = 0; bit < 8; bit++) _crc = (_crc & 0x80) ? ((_crc << 1) ^ 0x1D) : (_crc << 1);
		crcTable[i] = _crc;
	}
}

// main_greeting()
// transmit a message at module start
static void  main_greeting ( void)
{
	CANTxMsg_t  Msg;
	
	
	Msg.bufftype = CAN_BUFFER_TX_MSG;
	Msg.dlc      = CAN_LEN8_DLC;
	Msg.msgtype  = CAN_MSGTYPE_STANDARD;
	Msg.id       = 0x123;
	
	Msg.data32[0] = 0x67452301;
	Msg.data32[1] = 0xEFCDAB89;
	
	// overwrite byte 0 with FPGA version
	Msg.data8[0] = FPGA_VERSION;
	
	// Send Msg
	CAN_Write ( CAN_BUS1, &Msg);
}





// main()
// entry point from startup
int  main ( void)
{
	// init hardware and timer 0. Timer 0 is free running
	// with 1 us resolution without any IRQ.
	HW_Init();
	
	// init CAN
	CAN_UserInit();
	
	// set green LEDs for CAN1 and CAN2
	HW_SetLED ( HW_LED_CAN1, HW_LED_GREEN);
	HW_SetLED ( HW_LED_CAN2, HW_LED_GREEN);
	
	// send the greeting message
	main_greeting();

	// initialize CRC table
	CRCSAEInit();
	
	// main loop
	while ( 1)
	{
		CANRxMsg_t  RxMsg;
		
		// CAN1定义为CAN报文
		// process messages from CAN1
		if ( CAN_UserRead ( CAN_BUS1, &RxMsg) == CAN_ERR_OK)
		{
			// message received from CAN1
			LED_toggleCAN1 ^= 1;

			if ( LED_toggleCAN1)
			{
				HW_SetLED ( HW_LED_CAN1, HW_LED_ORANGE);
			}
			
			else
			{
				HW_SetLED ( HW_LED_CAN1, HW_LED_GREEN);
			}

			// 生成的 CAN FD 报文
			CANRxMsg_t RxMsg64 = RxMsg;
			
			switch (RxMsg.id)
			{
				case 0x170:
					for (int i = 0; i < 64; i++)
					{
						RxMsg64.data8[i] = 0;
					}
					RxMsg64.id = 0x170;
					RxMsg64.msgtype = CAN_MSGTYPE_FDF;
					RxMsg64.dlc = CAN_LEN64_DLC;

					RxMsg64.data8[5] |= RxMsg.data8[5] & 0xff;
					RxMsg64.data8[4] |= RxMsg.data8[4] & 0x07;
					RxMsg64.data8[7] |= RxMsg.data8[7] & 0xff;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0xc0;
					RxMsg64.data8[2] |= RxMsg.data8[2] & 0xff;
					RxMsg64.data8[1] |= RxMsg.data8[1] & 0x01;
					RxMsg64.data8[4] |= RxMsg.data8[4] & 0xf8;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0x3f;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x0f;

					CAN_Write (CAN_BUS2, &RxMsg64);
					break;
				case 0x24F:
					for (int i = 0; i < 64; i++)
					{
						RxMsg64.data8[i] = 0;
					}
					RxMsg64.id = 0x24F;
					RxMsg64.msgtype = CAN_MSGTYPE_FDF;
					RxMsg64.dlc = CAN_LEN64_DLC;

					RxMsg64.data8[1] |= RxMsg.data8[1] & 0x70;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0x20;
					RxMsg64.data8[1] |= RxMsg.data8[1] & 0x02;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x30;
					RxMsg64.data8[7] |= RxMsg.data8[7] & 0xff;
					RxMsg64.data8[0] |= RxMsg.data8[0] & 0xff;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0x1c;
					RxMsg64.data8[1] |= RxMsg.data8[1] & 0x04;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x0f;
					RxMsg64.data8[2] |= RxMsg.data8[2] & 0xff;
					RxMsg64.data8[1] |= RxMsg.data8[1] & 0x01;
					RxMsg64.data8[5] |= (RxMsg.data8[1] & 0x80) >> 7;
					RxMsg64.data8[5] |= RxMsg.data8[1] & 0x80;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0xc0;

					CAN_Write (CAN_BUS2, &RxMsg64);
					break;
				case 0x17E:
					for (int i = 0; i < 64; i++)
					{
						RxMsg64.data8[i] = 0;
					}
					RxMsg64.id = 0x17E;
					RxMsg64.msgtype = CAN_MSGTYPE_FDF;
					RxMsg64.dlc = CAN_LEN64_DLC;

					RxMsg64.data8[7] |= RxMsg.data8[7] & 0xff;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x40;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x30;
					RxMsg64.data8[1] |= ((RxMsg.data8[1] & 0xe0) >> 1) | RxMsg.data8[0] << 7;
					RxMsg64.data8[0] |= (RxMsg.data8[0] & 0xff) >> 1;
					RxMsg64.data8[5] |= RxMsg.data8[5] & 0x01;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x0f;

					CAN_Write (CAN_BUS2, &RxMsg64);
					break;
				case 0x180:
					for (int i = 0; i < 64; i++)
					{
						RxMsg64.data8[i] = 0;
					}
					RxMsg64.id = 0x180;
					RxMsg64.msgtype = CAN_MSGTYPE_FDF;
					RxMsg64.dlc = CAN_LEN64_DLC;

					RxMsg64.data8[7] |= RxMsg.data8[7] & 0xff;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0x0f;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0x20;
					RxMsg64.data8[1] |= RxMsg.data8[1] & 0xff;
					RxMsg64.data8[0] |= RxMsg.data8[0] & 0xff;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0x80;
					RxMsg64.data8[2] |= RxMsg.data8[2] & 0xff;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0x40;

					CAN_Write (CAN_BUS2, &RxMsg64);
					break;
				case 0x699:
					for (int i = 0; i < 64; i++)
					{
						RxMsg64.data8[i] = 0;
					}
					RxMsg64.id = 0x699;
					RxMsg64.msgtype = CAN_MSGTYPE_FDF;
					RxMsg64.dlc = CAN_LEN64_DLC;

					RxMsg64.data8[0] |= RxMsg.data8[0] & 0xff;
					RxMsg64.data8[2] |= RxMsg.data8[2] & 0xff;
					RxMsg64.data8[1] |= RxMsg.data8[1] & 0xff;
					RxMsg64.data8[3] |= RxMsg.data8[3] & 0xff;
					RxMsg64.data8[4] |= RxMsg.data8[4] & 0xff;
					RxMsg64.data8[6] |= RxMsg.data8[6] & 0xff;
					RxMsg64.data8[5] |= RxMsg.data8[5] & 0xff;
					RxMsg64.data8[7] |= RxMsg.data8[7] & 0xff;

					CAN_Write (CAN_BUS2, &RxMsg64);
					break;

				default:
					CAN_Write ( CAN_BUS2, &RxMsg);
					break;
			}
		}
		
		
		// CAN2定义为CANFD报文
		// process messages from CAN2
		if ( CAN_UserRead ( CAN_BUS2, &RxMsg) == CAN_ERR_OK)
		{
			// message received from CAN2
			LED_toggleCAN2 ^= 1;

			if ( LED_toggleCAN2)
			{
				HW_SetLED ( HW_LED_CAN2, HW_LED_ORANGE);
			}
			
			else
			{
				HW_SetLED ( HW_LED_CAN2, HW_LED_GREEN);
			}
			
			// 生成的 CAN 报文
			CANRxMsg_t RxMsg8 = RxMsg;

			switch (RxMsg.id)
			{
				case 0x20B:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x258;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[7] |= RxMsg.data8[63] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[62] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[61] & 0x0f;
					RxMsg8.data8[2] |= RxMsg.data8[26] & 0xff;
					RxMsg8.data8[3] |= RxMsg.data8[27] & 0xff;
					RxMsg8.data8[5] |= RxMsg.data8[29] & 0xff;
					RxMsg8.data8[4] |= RxMsg.data8[28] & 0xff;

					CANRxMsg_t RxMsg8_2 = RxMsg;
					for (int i = 0; i < 8; i++)
					{
						RxMsg8_2.data8[i] = 0;
					}
					RxMsg8_2.id = 0x208;
					RxMsg8_2.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8_2.dlc = CAN_LEN8_DLC;

					RxMsg8_2.data8[3] |= RxMsg.data8[11] & 0xff;
					RxMsg8_2.data8[2] |= RxMsg.data8[10] & 0x1f;
					RxMsg8_2.data8[2] |= RxMsg.data8[10] & 0x80;
					RxMsg8_2.data8[2] |= RxMsg.data8[10] & 0x60;
					RxMsg8_2.data8[1] |= RxMsg.data8[9] & 0xff;
					RxMsg8_2.data8[0] |= RxMsg.data8[8] & 0x1f;
					RxMsg8_2.data8[0] |= RxMsg.data8[8] & 0x80;
					RxMsg8_2.data8[0] |= RxMsg.data8[8] & 0x60;

					CAN_Write (CAN_BUS1, &RxMsg8);
					CAN_Write (CAN_BUS1, &RxMsg8_2);
					break;
				case 0x244:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x30A;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[1] |= RxMsg.data8[2] & 0x30;

					CAN_Write (CAN_BUS1, &RxMsg8);
					break;
				case 0x247:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x264;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[7] |= RxMsg.data8[31] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[30] & 0xff;
					RxMsg8.data8[0] |= RxMsg.data8[0] & 0x04;
					RxMsg8.data8[6] |= RxMsg.data8[29] & 0x0f;
					RxMsg8.data8[2] |= RxMsg.data8[2] & 0xff;
					RxMsg8.data8[1] |= RxMsg.data8[1] & 0xff;
					RxMsg8.data8[0] |= RxMsg.data8[0] & 0x03;

					RxMsg8.data8[7] = CRCSAECalc(RxMsg8.data8, 7);
					CAN_Write (CAN_BUS1, &RxMsg8);
					break;
				case 0x17A:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x187;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[5] |= RxMsg.data8[5] & 0xff;
					RxMsg8.data8[4] |= RxMsg.data8[4] & 0x1f;
					RxMsg8.data8[4] |= RxMsg.data8[4] & 0x20;
					RxMsg8.data8[7] |= RxMsg.data8[63] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[62] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[61] & 0x0f;

					CAN_Write (CAN_BUS1, &RxMsg8);
					break;
				case 0x2DE:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x2DE;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[4] |= (RxMsg.data8[1] & 0x06) >> -5;

					CAN_Write (CAN_BUS1, &RxMsg8);
					break;
				case 0x1BA:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x1C0;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[7] |= RxMsg.data8[7] & 0xff;
					RxMsg8.data8[4] |= RxMsg.data8[4] & 0xf0;
					RxMsg8.data8[3] |= RxMsg.data8[3] & 0xff;
					RxMsg8.data8[2] |= RxMsg.data8[2] & 0x03;
					RxMsg8.data8[4] |= RxMsg.data8[4] & 0x08;
					RxMsg8.data8[1] |= RxMsg.data8[1] & 0xe0;
					RxMsg8.data8[0] |= RxMsg.data8[0] & 0xff;
					RxMsg8.data8[2] |= RxMsg.data8[2] & 0xfc;
					RxMsg8.data8[1] |= RxMsg.data8[1] & 0x1f;
					RxMsg8.data8[6] |= RxMsg.data8[6] & 0x0f;

					CAN_Write (CAN_BUS1, &RxMsg8);
					break;
				case 0x17D:
					for (int i = 0; i < 8; i++)
					{
						RxMsg8.data8[i] = 0;
					}
					RxMsg8.id = 0x1D1;
					RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
					RxMsg8.dlc = CAN_LEN8_DLC;

					RxMsg8.data8[4] |= (RxMsg.data8[38] & 0xc0) >> 4;
					RxMsg8.data8[7] |= RxMsg.data8[63] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[62] & 0xff;
					RxMsg8.data8[6] |= RxMsg.data8[61] & 0x0f;

					CAN_Write (CAN_BUS1, &RxMsg8);
					break;
			
			default:
				CAN_Write ( CAN_BUS1, &RxMsg);
				break;
			}
		}
	}
}