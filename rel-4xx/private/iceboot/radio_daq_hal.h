/***********************************************************************************************************

	DOMMB RADIO DAQ Interface

	file name		radio_daq_hal.h
	created			7/2006
	last			7/2006
	version			0.00
	author			H. Landsman
	description


***********************************************************************************************************/

/* 
	Base address and offsets
*/


//#define		C_BASE_ADDRESS				DOM_FB_BASE

#define         SEGMENT_SIZE                            0x00000010
#define         CONTROL_BIT_SIZE                        0x00000002
#define         NUMBER_OF_SEGMENTS                      10000
#define         SEGMENT_START_ADDRESS                   (0x1000010)
#define         COUNTER_ADDRESS                         (0x1000000)
#define 	FPGA_BASE 				(0x90080000)
#define		FPGA_CLOCK_HI    			(FPGA_BASE + 0x1044)
#define		FPGA_CLOCK_LOW    			(FPGA_BASE + 0x1040)




/*
 BASE
*/
#define          FL_BASE                              0x60000000
#define 	TRACR_VERSION				(FL_BASE+0x0002)
#define 	TRACR_STATUS				(FL_BASE+0x0010)
#define 	REFLECTION				(FL_BASE+0x0011)
#define 	EVENT_FIFO_READ				(FL_BASE+0x0025)
#define 	EVENT_FIFO_STATUS			(FL_BASE+0x0026)
#define 	EVENT_FIFO_READ_COUNT			(FL_BASE+0x0027)
#define 	EVENT_FIFO_WRITE_COUNT			(FL_BASE+0x0028)
#define 	FIFO_RESET				(FL_BASE+0x0029)

/*
 ATWD
*/
#define 	ATWD_ADDRESS				0x90000424

/*
 Radio data structure
*/
#define         RADIO_PACKET_SIZE                       12000
#define         RADIO_HEADER_SIZE                       100

/*
  Virtual addresses
*/

#define         VIRTUAL_UPPER                          0x21+FL_BASE
#define         VIRTUAL_LOWER                          0x22+FL_BASE
#define         VIRTUAL_VAL                            0x23+FL_BASE
/*
   Virtual commands
*/

#define         POWER_CONTROL_REGISTER                0x04
#define         DIE_TMP_HIGH                          0x22
#define         DIE_TMP_LOW                           0x23
#define         ROBUST_STATUS_REG                     0x41
#define         ACU_VC                                0x80
#define         TEST_PATTERN_VC                       0x43
#define         FIRST_DAC_VC                          0x98
#define         LAST_DAC_VC                           0xB7  
#define         FIRST_SCALER_VC                       0x50
#define         LAST_SCALER_VC                        0x7F  


/* 
Real addresses
 */

#define FIFO_READ   FL_BASE + 0x25

/*
	Bit masks (TBD)
*/

/*
#define		C_BIT_NO_DATA			        0
#define		C_BIT_EVENT_DATA			1
#define		C_BIT_MONITOR_DATA			2
#define		C_BIT_TCAL_DATA			        3
#define		C_BIT_ERROR_DATA			4
*/

/*
	Packet Specifiers
*/
/*
#define		C_CMD_SEND_CONFIG			0x01
#define		C_CMD_SEND_TEST_PTN			0x02
#define		C_CMD_GET_MONITOR			0x01
#define		C_CMD_GET_PEDESTAL			0x02
#define		C_CMD_GET_EVENT_DATA		0x03
#define		C_CMD_GET_ERROR_MSG			0x04
*/

/*
	Type defs and unions
*/
/*
	Macros
*/

#define		RADBYTE(a) ( *(volatile UBYTE *) (a) )
#define		RADLONG(a) ( *(volatile ULONG *) (a) )
#define		RADWORD(a) ( *(volatile UWORD *) (a) )

const char *radio_r(const char *p);
const char *radio_r1(const char *p);
const char *radio_rs(const char *p);
const char *radio_w(const char *p);
const char *radio_w_loop(const char *p);
const char *radio_readDAC(const char *p);
const char *radio_readScaler(const char *p);
const char *radio_fifo(const char *p);
const char *reflection2mb(const char *p);
const char *reflection(const char *p);
const char *mbreflection(const char *p);
const char *read_reflection(const char *p);
const char * radio_atwd_time(const char *p);
const char * radio_atwd_parse(const char *p);
const char *write_v(const char *p);
const char *read_v(const char *p);
const char *radio_stat(const char *p);
const char *forced_trig(const char *p);
const char *radio_trig(const char *p);
const char *Radio_TCal(const char *p);
const char *bdumpi(const char *p);
const char *radio_dumpscaler(const char *p);
const char *enableFBminY(const char *p);
