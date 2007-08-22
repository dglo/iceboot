/***********************************************************************************************************

	DOMMB-TRACR Interface

	file name		radio_daq_hal.c
	created			7/2006
	last
	author		        H.Landsman
	description


***********************************************************************************************************/


//#include	"DOM_FB_regs.h"		This should be included already
//#include	"fb-hal.h"			This should be included already

#include "hal/DOM_MB_hal.h"

#include "radio_hal.h"
#include "radio_daq_hal.h"    

#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sfi.h"

static void radio_daq_reset(void) {
  int i;
  int N;
  printf ("number of seg=%d, %x ", NUMBER_OF_SEGMENTS,NUMBER_OF_SEGMENTS);
  printf ("SEGMENT_SIZE= %d, %x",SEGMENT_SIZE,SEGMENT_SIZE); 
  for(N=0;N<NUMBER_OF_SEGMENTS;N++) {
    for (i=0; i< SEGMENT_SIZE; i++) 
      { 
	RADBYTE( SEGMENT_START_ADDRESS + 4*N*SEGMENT_SIZE + 2 + i*4) = 0;
	RADBYTE( SEGMENT_START_ADDRESS + 4*N*SEGMENT_SIZE + 2 + i*4+1) = 0;
	RADBYTE( SEGMENT_START_ADDRESS + 4*N*SEGMENT_SIZE + 2 + i*4+2) = 0;
	RADBYTE( SEGMENT_START_ADDRESS + 4*N*SEGMENT_SIZE + 2 + i*4+3) = 0;
	/*printf("address= %08x \r\n",  SEGMENT_START_ADDRESS + N*SEGMENT_SIZE + i*4);*/
      }
    
    RADBYTE( SEGMENT_START_ADDRESS + 4*N*SEGMENT_SIZE + 1)=0;
    
    printf("setting zrop %d",SEGMENT_START_ADDRESS + 4*N*SEGMENT_SIZE + 1);
  }
}


static void readV(void){
  int commandH = pop();
  int commandL = pop();
  RADBYTE( VIRTUAL_UPPER)=commandH;
  RADBYTE( VIRTUAL_LOWER)=commandL;
  push(RADBYTE(VIRTUAL_VAL));
}

static void writeV(void){
  int commandH = pop();
  int commandL = pop();
  int value = pop();
  RADBYTE( VIRTUAL_UPPER)=commandH;
  RADBYTE( VIRTUAL_LOWER)=commandL;
  RADBYTE( VIRTUAL_VAL)=value;
}

static void bdump(void) {
   int cnt = pop();
   unsigned *addr = (unsigned *) pop();
   write(1, addr, cnt);
}

static void radioScalerStat(void){
  int addr=pop();  // pop Address to dump the header to.
  int addr0=addr;
  int i;
  RADBYTE(VIRTUAL_UPPER)=0x00;
  for (i=FIRST_SCALER_VC; i<=LAST_SCALER_VC; i++){ 
    RADBYTE(VIRTUAL_LOWER)=i;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
  } 
  
  push (addr0);
  push (addr-addr0);
 
}

static void radioDACStat(void){
  int addr=pop();  // pop Address to dump the header to.
  int addr0=addr;
  int i;
  RADBYTE(VIRTUAL_UPPER)=0x00;
  for (i=FIRST_DAC_VC; i<=LAST_DAC_VC; i++){
    RADBYTE(VIRTUAL_LOWER)=i;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
  }
 
  push (addr0);
  int temp=addr-addr0;
  push (temp);
 
}


static void radio_pp_counter(void) {	
  unsigned short N1;
 
  N1 = RADBYTE( COUNTER_ADDRESS  );	
  N1=N1+1;
  if (N1 == NUMBER_OF_SEGMENTS) {
    N1=0;}
  RADBYTE( COUNTER_ADDRESS) = N1;
  /* printf("Counterr was set to  %d \r\n", N1);*/
}

static void radioStat(void){
  int headerVersion=3;
  int addr=pop();  // pop Address to dump the header to.
  int addr0=addr;
  //addr=addr+2;
  RADBYTE(addr++)=0xca; // cafe
  RADBYTE(addr++)=0xfe;// cafe header
  RADBYTE(addr++)=headerVersion;
  domID(NULL); // Print mbid 
  const int cnt = pop();
  unsigned addrid = pop();
  int i=0;
  while (i<cnt){  // now read domid
      RADBYTE(addr++)=RADBYTE(addrid+i);
      i=i+1;
      } 
    RADBYTE(addr++)=0xff;
    RADBYTE(addr++)=0xff;

    // Get mb temperature
    //int temp=halReadTemp(); // print mb temperature
    //unsigned *cp;
    //cp = (unsigned *) addr;
    //*cp = (unsigned ) temp;
  // get TRACR die temperature
  
    RADBYTE(VIRTUAL_UPPER)=0x00;
    RADBYTE(VIRTUAL_LOWER)=DIE_TMP_HIGH;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE(VIRTUAL_LOWER)=DIE_TMP_LOW;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE(VIRTUAL_LOWER)=POWER_CONTROL_REGISTER;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE(VIRTUAL_LOWER)=ROBUST_STATUS_REG;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

    RADBYTE(addr++)=RADBYTE(TRACR_VERSION);
    RADBYTE(addr++)=RADBYTE(TRACR_STATUS);

    RADBYTE(addr++)=RADBYTE(EVENT_FIFO_STATUS);
    RADBYTE(addr++)=RADBYTE(EVENT_FIFO_READ_COUNT);
    RADBYTE(addr++)=RADBYTE(EVENT_FIFO_WRITE_COUNT);
    //unsigned time1= *(const unsigned *(0x90081044));
    //unsigned *time1add=0x90081040;
    //unsigned *time2add=0x90081044;
    RADBYTE(addr++)=0xaa;
    RADBYTE(addr++)=0xaa;
    addr=addr+2;
    // Now fill Virtual address with the current tracr time. We will read it out later.
    // Imeddiatly after that read mb time. Start with least significant bytes.
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x10;
    RADBYTE( VIRTUAL_VAL)=1; 
    // MB clock
    RADLONG(addr)=RADLONG(FPGA_CLOCK_LOW)  ;
    addr=addr+4;
    RADLONG(addr)= RADLONG(FPGA_CLOCK_HI) ;
    addr=addr+4;
    RADBYTE(VIRTUAL_LOWER)=ACU_VC;  // read acu status
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE(VIRTUAL_LOWER)=TEST_PATTERN_VC;  // read acu status
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    //RADBYTE(addr++)=0xbb;
    //RADBYTE(addr++)=0xbb;
    //RADBYTE(addr++)=0xcc;
    //RADBYTE(addr++)=0xcc;
    //RADBYTE(addr++)=0xdd;
    //RADBYTE(addr++)=0xdd;
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x16;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x15;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x14;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x13;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x12;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    RADBYTE( VIRTUAL_UPPER)=0x00;
    RADBYTE( VIRTUAL_LOWER)=0x11;
    RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    // Now put in DAC
    RADBYTE(VIRTUAL_UPPER)=0x00;
    for (i=FIRST_DAC_VC; i<=LAST_DAC_VC; i++){
      RADBYTE(VIRTUAL_LOWER)=i;
      RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    }
 
    // now put in scaler
    //for (i=FIRST_SCALER_VC; i<=LAST_SCALER_VC; i++){
    //RADBYTE(VIRTUAL_LOWER)=i;
    //RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
    //}
    RADBYTE(addr++)=0xee;
    RADBYTE(addr++)=0xee;

    if (addr>addr0+RADIO_HEADER_SIZE){
      // header size is too large. put bad bad at the end
      RADBYTE(addr0+RADIO_HEADER_SIZE-2)=0xad;
      RADBYTE(addr0+RADIO_HEADER_SIZE-1)=0x0b;
      RADBYTE(addr0+RADIO_HEADER_SIZE-4)=0xad;
      RADBYTE(addr0+RADIO_HEADER_SIZE-3)=0x0b;
    }
    else {
      // Fill rest with 00 and put babe at the end
      while (addr<addr0+RADIO_HEADER_SIZE-2) {
	RADBYTE(addr++)=0x00;
      }
      RADBYTE(addr++)=0xba; // suffix
      RADBYTE(addr++)=0xbe; // suffix
    }
  //printf("Temp= %i, add=%i ",temp, addr);
    push (addr);
  push (addr-addr0);
}


static void read_fifo(void){
  int number = pop();
  int addr0 = pop();
  int addr=addr0;
  int LEN=2;
  int i;
  int N_Zero_read_count=0;
  int dead, beef, be, counter;
  //long ta, tb;
  int Event_Fifo;
  int N1;
  N1=0;
  beef=0;
  //int N1,N2,t1,t2,t3,t4,t5,t6;
  if (number==0){
    //while (dead==0 || beef==0) { // Added to make sure to read out until a good WF will be read.
    i=0;   
    dead=0;
    beef=0; 
    be=0;
    N_Zero_read_count=0;
    counter=0; // this will serve as a timer to stop waiting for a trigger.
    // 12/06/2007 disabled the timer to stop watin.

    Event_Fifo=RADBYTE(EVENT_FIFO_STATUS);
    while (Event_Fifo==3 && counter<5000000) {
      Event_Fifo=RADBYTE(EVENT_FIFO_STATUS); 
      counter++; 
      counter--;
      /*if (counter==20) {  // Currently disabled
        RADBYTE(addr++)=0xab;
        RADBYTE(addr++)=0xcd;
	push(addr);
	DumpScaler();
	addr=addr+pop()+1;  // Pop out the length of the data, and update the address
	pop();//pop out the starting address. We don't need it.
	RADBYTE(addr++)=0xab;
	RADBYTE(addr++)=0xcd;
	counter=0;}*/
    }
    // hagar:
    halUSleep(50); //wait at least 41 usec till robust will start pushing data
    ///    while (dead==0 && RADBYTE(EVENT_FIFO_STATUS)!=3)   /* Look for DEAD in the next 1000 words or until fifo is empty */
    //printf("fifo=%d, %d \n ",Event_Fifo,counter);
    while (dead==0 ) //&& N_Zero_read_count<3)   // Look for DEAD in the next 1000 words or until fifo is empty. Use event fifo read count, but allow one exra count
      { 
	if (RADBYTE(EVENT_FIFO_READ_COUNT)==0) 	{ N_Zero_read_count++;}
	N1=RADBYTE(FIFO_READ);
	//printf("dead: reading %d count=%d, %d, %d \n",N1,N_Zero_read_count,RADBYTE(EVENT_FIFO_READ_COUNT),RADBYTE(EVENT_FIFO_STATUS));
	//printf ("%x ",N1);

	if (N1==0xde) {
	  //printf ("Found DE \n\n");
	  //dead=1; // This should be removed. The event header DEAD is now DEA9      
	  N1=RADBYTE(FIFO_READ);
	  //printf ("%x ",N1);
		if (N1==0xad || N1==0xde) {
		  dead=1;
		  //printf ("Found dead \n");
		}
	}
      }
    //printf("zero=%d \n ",N_Zero_read_count);

    LEN=2;
    i=0;
    if (dead==1) {    
      RADBYTE(addr)=0xde;   /* Found dead so print it to output*/
      RADBYTE(addr+1)=0xad;
      addr=addr+2;
     
	 /* now read all stuff until got beef, or until empty fifo */

	//	 while (beef!=1 && RADBYTE(EVENT_FIFO_STATUS)!=3) {
      while (beef!=1) { // && N_Zero_read_count<3) {
	  if (RADBYTE(EVENT_FIFO_READ_COUNT)==0) 	{ N_Zero_read_count++;}
	 	N1=RADBYTE(FIFO_READ);  /* read next word */	
		RADBYTE(addr)=N1;
		addr++;
		LEN++;
		//printf("beef: reading %d count=%d, %d, %d \n",N1,N_Zero_read_count,RADBYTE(EVENT_FIFO_READ_COUNT),RADBYTE(EVENT_FIFO_STATUS));
		if (N1==0xbe) {be=1; /*printf("found be");*/}
		else if (N1==0xef && be==1) {/*printf("found ef");*/ beef=1;}
		else if (be==1 && N1!=0xef) {be=0;}
	 }
      //push(tb);
      //push(1);
    }
  }
  //  }
  push(addr0);
  push(LEN);

  
  if (number==1) {   /* Read until buffer is empty or no more than 5000 events*/ 
    i=0;
    int addr=pop();
    int counter=0;
    int N1;
    int dead=0;
    int de=0;
    beef=0;
    be=0;
    //while (dead==0 && N_Zero_read_count<3)   /* Look for DEAD in the next 1000 words or until fifo is empty. Use event fifo read count, but allow one exra count*/ 
    //{
    //	if (RADBYTE(EVENT_FIFO_READ_COUNT)==0) 	{ N_Zero_read_count++;}
    
    //	while (RADBYTE(EVENT_FIFO_STATUS)!=3 && counter<5000) {
    while (counter<5000 && N_Zero_read_count<3) {
      if (RADBYTE(EVENT_FIFO_READ_COUNT)==0) 	{ N_Zero_read_count++;}
	  
      N1=RADBYTE(FIFO_READ);
      if (N1==0xde) {
	//printf ("Found DE \n\n");
	dead=1; // This should be removed. The event header DEAD is now DEA9      
	N1=RADBYTE(FIFO_READ);
	if (N1==0xde) {de=1;}
	else if (N1==0xad && de==1) {counter=counter+1; de=0;}
	else {de=0;}
	/*	  if (N1==0xbe) {be=1;}
		  else if (N1==0xef && be==1) {beef=1; be=0;}
		  else {de=0;}*/
	i=i+1;
	RADBYTE(addr+i)=RADBYTE(FIFO_READ);
      }
      push(addr);
      push(i);
    }
  }
  
  if (number>1) {
    
    i=0;
    //    int N1;
    radio_pp_counter();
    /* N1=RADBYTE(COUNTER_ADDRESS);*/
    /*printf ("done writing %i",SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1+2);*/
    int addr=pop();
    for (i=0; i<number; i++) {
      /*    RADBYTE( SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1+2+i)=RADBYTE(FIFO_READ);*/
      RADBYTE( addr+i)=RADBYTE(FIFO_READ);
    }
    
    push(addr);
    push(number);
    /*  printf ("done writing %i",SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1+2+i);*/
    
  }
}

static void DumpScaler(void){
  // void *scalerStart=malloc(100);
  int addr=pop();
  int addr0=addr;
  int i;
  RADBYTE(VIRTUAL_UPPER)=0x00;
  for (i=0x50; i<1+0x7f; i++){
     RADBYTE( VIRTUAL_LOWER)=i;
     RADBYTE(addr++)=(RADBYTE(VIRTUAL_VAL));
   } 
  RADBYTE(VIRTUAL_LOWER)=0x44;
  RADBYTE(addr++)=(RADBYTE(VIRTUAL_VAL));
  RADBYTE(VIRTUAL_LOWER)=0x45;
  RADBYTE(addr++)=(RADBYTE(VIRTUAL_VAL));
  push(addr0);
  push(addr-addr0);
  //bdump;
  //free(scalerStart);
}


static void radioTriggerGo(void){
  int mode=pop(); // mode=1 for forcedtrigger
  int N=pop();  // Read how many WF to read
  int i=0;
  int lastATWDread=0;
  int Npackage=1;
  int n=0;
  if (N<Npackage){
    Npackage=N;}
  void *headStart = malloc(RADIO_PACKET_SIZE*Npackage); // alocate memory for N packets. address=(int)ptr
  int pack_length=0;
  while (i<N){    
    push((int)headStart+pack_length);


    //printf ("writnring header here %i \n",ptr);
    radioStat(); //print header
  
    
    int lenHeader=pop();
    int endHeader=pop();
    if (mode==1) {	
      push(1);  // One
      push(02); // Shot
      push(00); // Trigger
      writeV(); //
      halUSleep(10000);
    }
    push (endHeader);
    //printf ("writnring fifo here %i \n",endHeader);
    push(0);
    read_fifo();
    push(lastATWDread);
    //read_atwd();
    lastATWDread=pop();
    int lfifo=pop();
    pop();
    //printf ("lfifo=%d, afio=%d",lfifo, afifo);
    pack_length=pack_length+lfifo + lenHeader; // packet length =  header length + fifo length
    //printf ("lfifo=%d, afio=%d lenHeader=%d  pack=%d",lfifo, afifo,lenHeader,pack_length);


    //printf ("length %i",pack_length);
    i=i+1;
    n=n+1;
    if (pack_length>0 && (n>=Npackage || i==N)){
      n=0;
      push((int)headStart);
      push(pack_length);
      //push (33);
      bdump();
      pack_length=0;
    }

  } /* while(i<N) */
  free(headStart);
}

static void radioTrigger(void){


  push(0);  // The number of WF are already in. This will tell that we are in forced run.
  //RADBYTE(FIFO_RESET)=1; //Reset should be done before starting DAC. Otherwise we loose the "DEAD"
  radioTriggerGo();
}

static void forcedTrigger(void){
  RADBYTE(FIFO_RESET)=1; 
  // since there is some time delay, I start by forced triggereing. This way each forced trigger will be read by the next to following read_fifo.
  push(1);  // One
  push(02); // Shot
  push(00); // Trigger
  writeV(); //
  push(1);  // The number of WF are already in. This will tell that we are in forced run.
  radioTriggerGo();
}


static void lforcedTrigger(void){
  RADBYTE(FIFO_RESET)=1; 
  int N=pop();  // Read how many WF to read
  int i=0;
  // since there is some time delay, I start by forced triggereing. This way each forced trigger will be read by the next to following read_fifo.
  push(1);  // One
  push(02); // Shot
  push(00); // Trigger
  writeV(); //
  
  while (i<N){
    void *headStart = malloc(RADIO_PACKET_SIZE); // alocate memory for N packets. address=(int)ptr
    push((int)headStart);
    //printf ("writnring header here %i \n",ptr);
    radioStat(); //print header
    halUSleep(50); //wait at least 41 usec till robust will start pushing data
    //int temp=pop(); // an extra push correction, hagar
    int lenHeader=pop();
    int endHeader=pop();
    push(1);  // One
    push(02); // Shot
    push(00); // Trigger
    writeV(); //
    halUSleep(10000);

    push (endHeader);
    //printf ("writnring fifo here %i \n",endHeader);
    push(0);
    read_fifo();
    int lfifo=pop();
    //int afifo=pop();
    //printf ("lfifo=%d, afio=%d",lfifo, afifo);
    int pack_length=lfifo + lenHeader; // packet length =  header length + fifo length
    //printf ("lfifo=%d, afio=%d lenHeader=%d  pack=%d",lfifo, afifo,lenHeader,pack_length);

   
    push((int)headStart);
    push(pack_length);

    bdump();
    free(headStart);
    i=i+1;
  }

  /*i=0;
  while (i<N) {
    bdump();
    i=i+1;}*/
  RADBYTE(FIFO_RESET)=1;
}

static void forcedTrigger_old(void){
  int N=pop(); // Number of Wave forms to read in a single loop.
  void *ptr = malloc(N*RADIO_PACKET_SIZE); // alocate memory for N packets. address=(int)ptr
  int addr=(int)ptr;
  int i=0;
  int curr_address=addr;
  int curr_length=0;
  while (i<N){
    // forced trigger:
    /*RADBYTE( VIRTUAL_UPPER)=DIE_TMP_LOW_H;
    RADBYTE( VIRTUAL_LOWER)=DIE_TMP_LOW_L;
    RADBYTE(VIRTUAL_VAL)=1;*/
    // dump header:
    /*
      
    */
    push(1);  // One
    push(02); // Shot
    push(00); // Trigger
    writeV(); //
    
    push (curr_address);
    push (0);
    read_fifo();
    //pop();
    //pop();
    //pop();
    curr_length=curr_length+pop();
    curr_address=pop();
    i=i+1;
  }

  
  bdump();
  // write(1, (unsigned *) addr, curr_length); //This is like bdump on address addr, length=curr_length
}



static void RadioTCal(void){
  //int i=0x16;
  int addr=pop();
  int addr0=addr;
  //int c=1;
  //long time1=0, time2=0, timeMB;
  //  int t0,t1,t2,t3,t4,t5,t6;
  long timeMB1,timeMB2;
  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x10;
  RADBYTE( VIRTUAL_VAL)=1; 
  timeMB1=RADLONG(FPGA_CLOCK_LOW)  ;
  timeMB2=RADLONG(FPGA_CLOCK_HI) ;
  RADLONG(addr)=RADLONG(FPGA_CLOCK_LOW)  ;
  addr=addr+4;
  RADLONG(addr)= RADLONG(FPGA_CLOCK_HI) ;
  addr=addr+4;
  RADBYTE(addr++)=0xaa;
  RADBYTE(addr++)=0xaa;
  // while (i>=0x10){
  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x16;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);
  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x15;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x14;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x13;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x12;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x11;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

  RADBYTE( VIRTUAL_UPPER)=0x00;
  RADBYTE( VIRTUAL_LOWER)=0x10;
  RADBYTE(addr++)=RADBYTE(VIRTUAL_VAL);

  RADBYTE(addr++)=0xff;

  RADBYTE(addr++)=0xff;

  //push(t0+t1*256+t2*256*256);
  // push(t3+t4*256+t5*256*256);
  //push(t6);

  //push(t1);
  //    if (i<0x13){
  //time1=time1+t1*c;
  //i=i-1;
  //c=c*256;
  // }
  
  //   timeMB=timeMB1+timeMB2*(2**24);
  //push(time);
  //push(timeMB);
  push(addr0);
  push(addr-addr0);
  //    push((timeMB-time)& 0x0000ffffff000000);
  //push((timeMB-time)& 0xffff000000000000); 
}

static void read_atwd(void){
  int LastRead=pop();
  int ATWD_SIZE=2048;
  //int LastWrite = (unsigned *) ATWD_ADDRESS;
  int LastWrite=RADBYTE(ATWD_ADDRESS); 
  if (LastRead!=LastWrite && LastWrite>=0 && LastRead>=0) {
    push(LastRead);
    push(ATWD_SIZE);
    bdump();
    push(LastRead+ATWD_SIZE);
  }
  else {push(0);}
  //printf("LR=%d LW=%d",LastRead,LastWrite);
}



static void radio_write_record(void) {	
  printf ("hello I am here");
  int N1;
  int i;
  int j;
  j=0;
  N1 = RADBYTE( COUNTER_ADDRESS  )*4;	  
  /*  printf("SEGMENT_START_ADDRESS=%d+N1 %d *SEGMENT_SIZE %d+i*4+2)",SEGMENT_START_ADDRESS,N1,SEGMENT_SIZE);*/
  if  (RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1)==1){
    printf("Buffer overflow %d=%d. Stop writting",SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE +1,RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1));}

/*  RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1)=0;*/
/*  printf("read counter*4=%d",N1); */
  else{
    radio_pp_counter();
    for (i=0; i< SEGMENT_SIZE; i++) 
      /*  for (i=0; i< 10; i++) */
      { printf ("i=%d",i);
      j=j+1;j=N1/4;
      RADBYTE( SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1 + 2 + i*4) = j;
      RADBYTE( SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1 + 2 + i*4+1) = j;
      RADBYTE( SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1 + 2 + i*4+2) = j;
      RADBYTE( SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1 + 2 + i*4+3) = 100;
      printf(" %d address= %08x \r\n",  j,SEGMENT_START_ADDRESS + SEGMENT_SIZE*N1 + i*4);
      }
    RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE + 1)=1;
    /*  printf ("set status of %d in address %08x= %d\r\n", N1, SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1, RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1));

    printf("done writing %d words in addresses %08x %08x\r\n",i,SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+2,SEGMENT_START_ADDRESS+SEGMENT_SIZE*N1 + 2 +i*4+3);*/
  }
}
static void reflection_to_mb(void) {
  int number=pop();
  int N1;
  int i;
  int x;
  x=0;
  time_t Stime,Ftime;
/*  Stime=RADBYTE(FPGA_CLOCK_LOW);*/
  Stime=0;

    for (i=0; i< number; i++) 
    { x=x+1;
      if (x>255) {x=0;}
      RADBYTE(C_RADIO_REFLECT)=x;
      N1= RADBYTE( COUNTER_ADDRESS  );	
      N1=N1+1; 
      //halUSleep(10000);
      RADBYTE( SEGMENT_START_ADDRESS + N1) = RADBYTE(C_RADIO_REFLECT);
      radio_pp_counter();
    }
  Ftime=0;
  /*
  printf("%d %f \r\n",number,Stime);
  */
}
 
static void radio_reflection(void) {
  int N1=pop();
  RADBYTE(C_RADIO_REFLECT)=N1;
  /*halUSleep(10);*/

  int N2= RADBYTE(C_RADIO_REFLECT);
  printf("%i \r\n",N2);
/*     const char *addr = (const char *) C_RADIO_REFLECT;
       write(1, addr, 1);*/

}
static void mb_reflection(void) {
  int N1=pop();
  RADBYTE(COUNTER_ADDRESS)=N1;
  int N2= RADBYTE(C_RADIO_REFLECT);
  printf("%i \r\n",N2);
 /*  const char *addr = (const char *) COUNTER_ADDRESS;
  write(1, addr, 1);*/

}

static void radio_read_reflection(void) {
  int N2= RADBYTE(C_RADIO_REFLECT);
  printf("%i \r\n",N2);
}

static void radio_read_record(void) {	
  unsigned short N1;
  int i;
  N1 = RADBYTE( COUNTER_ADDRESS  );	
  unsigned *addr=(unsigned *) (SEGMENT_START_ADDRESS + N1*SEGMENT_SIZE);

  for (i=0; i< SEGMENT_SIZE-10; i++) 
    { printf("%08x \r\n",addr[i]); }

  /*
  printf("addresses= %08x %08x %08x %08x %80x \r\n",addr,addr[0],addr[1],addr[2],addr[3]);
  */
}

	
static void radio_read_1record() {	
  /*  unsigned short N1; */
  //int i;
  int N1=pop();
  printf ("looking status of %d in address %08x= %d\r\n", N1, SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1, RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1));
  /*N1 = RADBYTE( COUNTER_ADDRESS  );	*/
  if ( RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1)==1) {
    push(SEGMENT_START_ADDRESS + N1*SEGMENT_SIZE);
    push(SEGMENT_SIZE);
    odump(NULL);
    RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE + 1)=0;

  }
  printf("done\r\n");
}

static void radio_read_ok(void) {	
  int N1=pop();
  int a;
  a=RADBYTE( SEGMENT_START_ADDRESS+N1*SEGMENT_SIZE+1);
  push(a);

}




static void radio_zero_counter(void) {	
  //unsigned short N1, N2;
  RADBYTE( COUNTER_ADDRESS) = 0x00;	}

// implimentation of ratwd_time
const char * radio_atwd_time(const char *p) {
  // There is a known problem with accepting larger pointers
  // (somewhere >23781376) 
  unsigned *wf_base;
  int sanity;
  int output;
  wf_base = (unsigned *) pop();
  // sanity check from function radio_atwd_parse in a compact form
  sanity =  wf_base[0] & 0xFFFF0000;
  sanity = sanity>>16;
  if(1 == sanity) {
    //printf("%08lx%04x", wf_base[1], wf_base[0] & 0x0000FFFF);
    // the number above is big.  unless you have an unsigned long 
    // long, you should be careful, very careful
    printf("\n");
   
    output = 1;
  }else{
    /*printf("invalid event");
      printf("\n");*/
    //printf("%08lx%04x",0,0);
    //printf("\n");
    output = 0;
  }
  push(output);

  return p;
}

// implimentation of ratwd_parse
const char * radio_atwd_parse(const char *p) {
  unsigned int sanity;
  unsigned *wf_base;
  wf_base = (unsigned *) pop();

  // sanity check
  sanity =  wf_base[0] & 0xFFFF0000; 	// select the high byte of the word
  sanity = sanity>>16;			// shift the high byte to the lower byte
  push(sanity == 1);			// push the result of the sanity check
					// to the stack 1->good 0->bad
  printf("Sanity check: %x", sanity);
  printf("\n");
  if (sanity == 1) {
    // printout timestamp
    //printf("timestamp: %08lx%04x", wf_base[1], wf_base[0] & 0x0000FFFF);
    printf("\n");
    // check the ATWD sample info
    if (wf_base[2] & 0x00010000) {
      printf("ATWD B ");
    }else{
      printf("ATWD A ");
    }
    unsigned int size;
    size = wf_base[2] & 0x00180000;
    size = size>>19;
    printf("ATWD channel upto %u\n", size);
    //printf("\n");
    // print out available ATWD waveforms
    int i = 0; // a counter
    int channel = 0;
    int offset = 0;
    while (i<128) {
      i++;
      printf("%i\t", 128-i);
      for(channel = 0; channel< size + 1; channel++) {
        offset = 0x210 + channel * 0x100;
        if (i%2 == 1) {
          printf("%u\t", wf_base[offset/4 + (i-1)/2] & 0x000003FF);
        }else{
          printf("%u\t", (wf_base[offset/4 + (i-1)/2] & 0x03FF0000)>>16 );
        }
      }
      printf("\n");
    }
  }
  return p;
}

/* The data packets between DOMMB and TRACR have the following format:
		Byte 1	Packet length high byte
		Byte 2	Packet length low byte
		Byte 3	Data byte 1
		Byte 4	Data byte 2
			. . . . .
		Byte N	Data byte N
 */


const char *radio_czero(const char *p) {
  radio_zero_counter();
  return p;
}

const char *reflection2mb(const char *p) {
  reflection_to_mb();
  return p;
}

const char *read_reflection(const char *p) {
  radio_read_reflection();
  return p;
}

const char *mbreflection(const char *p){
  mb_reflection();
  return p;
}

const char *reflection(const char *p) {
  radio_reflection();
  return p;
}


const char *bdumpi(const char *p) {
  bdump();
  return p;
}
const char *radio_fifo(const char *p) {
  read_fifo();
  return p;
}

const char *write_v(const char *p) {
  writeV();
  return p;
}

const char *read_v(const char *p) {
  readV();
  return p;
}

const char *radio_stat(const char *p) {
  radioStat();
  return p;
}

const char *Radio_TCal(const char *p){
  RadioTCal();
    return p;
}
const char *radio_trig(const char *p){
  radioTrigger();
    return p;
}
const char *forced_trig(const char *p) {
  forcedTrigger();
  return p;
}

const char *radio_dreset(const char *p) {
  radio_daq_reset();
return p;
}

const char *radio_dumpscaler(const char *p) {
  DumpScaler();
return p;
}
const char *radio_readScaler(const char *p) {
  radioScalerStat();
return p;
}
const char *radio_readDAC(const char *p) {
  radioDACStat();
return p;
}

const char *enableFBminY(const char *p) {
    hal_FB_enable_min();
    return p;
}

const char *radio_w(const char *p) {
  radio_write_record();
  return p;
}
const char *radio_w_loop(const char *p) {
  int i=0;
  while (i<100) {
    i++;
    radio_write_record();
    //sleepy(1000);
  }
  return p;
}

const char *radio_r(const char *p) {
  radio_read_record();
  return p;
}

const char *radio_r1(const char *p) {
  radio_read_1record();
  return p;
}
const char *radio_rs(const char *p) {
  radio_read_ok();
  return p;
}

const char *radio_cpp(const char *p) {
  radio_pp_counter();
  return p;
}



