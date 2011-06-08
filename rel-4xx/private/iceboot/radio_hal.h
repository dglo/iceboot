/***********************************************************************************************************

	DOMMB-TRACR Interface

	file name		radio_hal.h
	created			6/5/2006
	last			6/7/2006
	version			0.00a
	author			N. Kitamura
	description


***********************************************************************************************************/

/* 
	Base address and offsets
*/

#define		C_DOM_RADIO_CONTROL			0x500000f9
//#define		C_BASE_ADDRESS				DOM_FB_BASE

#define		C_BASE_ADDRESS				0x60000000
#define         C_RADIO_REFLECT  (C_BASE_ADDRESS+0x11)

#define		C_ADDR_SOFT_RESET			(C_BASE_ADDRESS + 0x00000001)
#define		C_ADDR_STATUS0				(C_BASE_ADDRESS + 0x0000000f)
#define		C_ADDR_STATUS1				(C_BASE_ADDRESS + 0x00000010)
#define		C_ADDR_REFLECTION			(C_BASE_ADDRESS + 0x00000011)
#define		C_ADDR_VERSION_ID			(C_BASE_ADDRESS + 0x00000012)
#define		C_ADDR_INPUT_CONTROL		(C_BASE_ADDRESS + 0x0000001e)
#define		C_ADDR_INPUT_DATA			(C_BASE_ADDRESS + 0x0000001f)
#define		C_ADDR_OUTPUT_CONTROL		(C_BASE_ADDRESS + 0x00000020)
#define		C_ADDR_OUTPUT_DATA			(C_BASE_ADDRESS + 0x00000021)

/*
	Bit masks (TBD)
*/

#define		C_BIT_EVENT_DATA			0
#define		C_BIT_MONITOR_DATA			1
#define		C_BIT_ERROR_DATA			2

/*
	Packet Specifiers
*/

#define		C_CMD_SEND_CONFIG			0x01
#define		C_CMD_SEND_TEST_PTN			0x02
#define		C_CMD_GET_MONITOR			0x01
#define		C_CMD_GET_PEDESTAL			0x02
#define		C_CMD_GET_EVENT_DATA		0x03
#define		C_CMD_GET_ERROR_MSG			0x04


/*
	Type defs and unions
*/


#define UBYTE unsigned char
#define ULONG unsigned long

union DBYTE {
	struct { UBYTE hi, lo; } byte;
	unsigned word;
};


/*
	Macros
*/

#define		RADBYTE(a) ( *(volatile UBYTE *) (a) )
#define		RADWORD(a) ( *(volatile UWORD *) (a) )

/*
* The following constants are consistent with flash_defs_v00a.vhd.
*/
#define	C_RADIO_BASE_ADDR		(0x60000000)
#define	C_RADIO_SCR				(C_RADIO_BASE_ADDR + 0x00000000)
#define	C_RADIO_RSVD0			(C_RADIO_BASE_ADDR + 0x00000001)
#define	C_RADIO_RSVD1			(C_RADIO_BASE_ADDR + 0x00000002)
#define	C_RADIO_RSVD2			(C_RADIO_BASE_ADDR + 0x00000003)
#define	C_RADIO_DA0				(C_RADIO_BASE_ADDR + 0x00000004)
#define	C_RADIO_DA1				(C_RADIO_BASE_ADDR + 0x00000005)
#define	C_RADIO_DA2				(C_RADIO_BASE_ADDR + 0x00000006)
#define	C_RADIO_DA3				(C_RADIO_BASE_ADDR + 0x00000007)
#define	C_RADIO_DB0				(C_RADIO_BASE_ADDR + 0x00000008)
#define	C_RADIO_DB1				(C_RADIO_BASE_ADDR + 0x00000009)
#define	C_RADIO_DB2				(C_RADIO_BASE_ADDR + 0x0000000a)
#define	C_RADIO_DB3				(C_RADIO_BASE_ADDR + 0x0000000b)
#define	C_RADIO_DC0				(C_RADIO_BASE_ADDR + 0x0000000c)
#define	C_RADIO_DC1				(C_RADIO_BASE_ADDR + 0x0000000d)
#define	C_RADIO_DC2				(C_RADIO_BASE_ADDR + 0x0000000e)
#define	C_RADIO_DC3				(C_RADIO_BASE_ADDR + 0x0000000f)
