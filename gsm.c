//*****************************************************************************
//
// File Name	: 'gsm.c'
// Title		: GSM functions
// Author		: Hamid Rostami, hamid.r1988@gmail.com
// Version		: 1.0
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//****************************************************************************

#include "gsm.h"

int (*gsm_get_byte)(void);
void (*gsm_send_byte)(unsigned char c);

void gsm_init(void (*sendByte_func)(unsigned char c), int (*getByte_func)(void)){
	gsm_send_byte = sendByte_func;
	gsm_get_byte = getByte_func;
	

	//_delay_ms(5000);
	

	// Send dummy commands for matchnig baudrate in autobaud modules
	gsm_command("AT");
	gsm_command("AT");
	gsm_command("AT");
	
	//Turn off command echos
	gsm_command("ATE0");
	// Text SMS Mode
	gsm_command("AT+CMGF=1");
	// Enable Unstructured supplementary service data
	gsm_command("AT+CUSD=1");
	// set Preferred SMS Message Storage to SIM Card
	gsm_command("AT+CPMS=\"SM\"");
	
	// Enable delivery report
	gsm_command("AT+CNMI=2,1,0,1,0");
	gsm_command("AT+CSMP=49,255,0,0");
}

// ##################################################################################
// #               SMS FUNCTIONS
// ##################################################################################

void gsm_text_sms(void){
	gsm_command("AT+CMGF=1");
}

//----------------------------------
int gsm_check_new_sms( char *index ){
	int i, new_sms=0;
	char line[300], token[50];
	
	
	//uartFlushReceiveBuffer();
	
	gsm_command("AT+CMGL");
	_delay_ms(500);
	
	// Make a finite loop
	for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
			if( instr(line, "REC UNREAD") == 0 ){
				// CMGL output format :
				// +CMGL: 3,"REC UNREAD","+989372391694","","2011/08/27 09:12:04+18"
				
				read_token( line, token, 0, "," );
				read_token( token, token, 1, ":" );
				*index = atoi( token );		// store unread SMS index
				index++;
				new_sms++; 
				i = 0;		// Refresh loop counter
			}
		}
	}
	
	return new_sms;
}

//----------------------------------
int gsm_send_sms(char *number, char *text){
	int i;
	char line[20], token[10];
	

	rprintfInit(gsm_send_byte);
	rprintf("AT+CMGS=\"");
	rprintfStr(number);
	rprintf("\"\n");
	rprintfStr(text);
	gsm_send_byte(26);
	
	for(i=0; i<50; i++){
		if( gsm_readline(line) == TRUE ){
			//rprintfInit(uart1SendByte); rprintfStr(line); rprintfCRLF();
			// Check for error
			if( instr(line, "ERROR") == 0 ) return -1;
			// check +CMGS for refnum
			if( instr(line, "+CMGS") == 0 ){
				if( read_token(line, token, 1, ":") == 0 ) return atoi(token);
				return -1;
			}
		}
		_delay_ms(100);
	}
	
	return -1;
}

//-------------------------------------
int gsm_send_sms2( char *number, char *text, int try_num ){
  int i, refnum;
  
  for( i=0; i<try_num; i++ ){
    // Send SMS and store refnumber
    refnum = gsm_send_sms(number, text);
    //rprintfInit(uart1SendByte); rprintf("refnum: %d\n", refnum);
    if( refnum == -1 ) continue;
    // Check delivery report
    if( gsm_check_delivery(refnum) == TRUE ) return TRUE;
  }
  
  return FALSE;
}

//----------------------------------
int gsm_check_delivery( int refnum ){
	int i;
	char line[100], token[50];
	
	for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
		  rprintfInit(uart1SendByte); rprintfStr(line); rprintfCRLF();
			if( instr(line, "+CDS 6") == 0 )
				rprintfInit(uart1SendByte); rprintfStr(line); rprintfCRLF();
				// read second field of +CDS line as delivery ref number
				if( read_token(line, token, 1, ",") != 0 ) continue;
				// Compare delivery refnum with given refnum
				if( atoi(token) != refnum ) continue;
				// check delivery report value (for Operators that that contain , in date line Irancell)
				// Example: +CDS: 6,46,"+989372391694",145,"11/10/13,11:22:45+14","11/10/13,11:22:50+14",0
				if( read_token(line, token, 8, ",") == 0 ){
					if( atoi(token) == 0 ) return TRUE;
				}
				// check delivery report value (for Operators that that contain , in date like MCI)
				// Example: +CDS: 6,26,"+099372391694",129,"2011/10/13 13:08:35+14","2011/10/13 13:08:39+14",0
				if( read_token(line, token, 6, ",") == 0 ){
					if( atoi(token) == 0 ) return TRUE;
				}
		}
		_delay_ms(200);
	}

	return FALSE;
}


//----------------------------------
int gsm_read_sms( int index, SMS *sms ){
	int i;
	char line[200], token[50];
	
	//uartFlushReceiveBuffer(gsm_nUart);
	//gsm_rprintf_init();
	
	rprintfInit(gsm_send_byte);
	rprintf("AT+CMGR=%d\n", index);
	_delay_ms(500);
	
	// Make a finite loop
	for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
			if( instr(line, "+CMGR") == 0 ){
				// CMGR output format :
				// +CMGR: "REC READ","+989372391694","","2011/08/27 09:10:06+18"
				// Message Body
				
				if( strstr(line, "REC READ") != NULL )
					sms->stat = SMS_READ;
				else if( strstr(line, "REC UNREAD") != NULL )
					sms->stat = SMS_UNREAD;
				
				// store sms Number in sms->number
				read_token( line, token, 1, "," );
				remove_char( token, '"' );
				strcpy(sms->number, token );
				
				// store sms Date in sms->date
				read_token( line, token, 3, "," );
				remove_char( token, '"' );
				strcpy(sms->date, token );
				
				gsm_readline(line);
				strcpy(sms->body, line);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

//----------------------------------
int gsm_SIM_mem_used( int *total_mem ){
	int i, used, total;
	char line[100], token[50], *p;
	
	gsm_command("AT+CPMS?");
	
	// Make a finite loop
	for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
			//lcd_clear(); lcd_print(line); _delay_ms(1000);
			if( instr(line, "+CPMS") == 0 ){
				// +CPMS output format :
				// +CPMS: "SM",used,total,"SM",used,total,"SM",used,total
				
				p = strstr(line, "SM");		// Find SIM storage location
				if( read_token( p, token, 1, "," ) != 0 ) return -1;
				used = atoi(token);
				
				if( read_token( p, token, 2, "," ) != 0 ) return -1;
				total = atoi(token);
				if( total == 0 ) return -1;
				
				*total_mem = total;
				return used * 100 / total;		// Return used memory in percent
			}
		}
	}
	
	return -1;
}

//----------------------------------
int gsm_del_sms( int index ){
	rprintfInit(gsm_send_byte);
	rprintf("AT+CMGD=%d\n", index);
	_delay_ms(500);
	
	return TRUE;
}

// ##################################################################################
// #               GSM utility FUNCTIONS                                            #
// ##################################################################################

int gsm_usd_command( char *cmd, char *response ){
	char line[300];
	int i;
	
	gsm_command( cmd );
	_delay_ms(5000);
	
	// Make a finite loop
	for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
			if( instr(line, "+CUSD") == 0 ){
				strcpy(response, line);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

//-----------------------------------
void gsm_debug( char *command, void(*sendByte_func)(unsigned char c) ){
  char line[200];
  int i;
  
  gsm_command( command );
  _delay_ms(1000);
  
  rprintfInit(sendByte_func);
  for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
			rprintfStr(line);
			rprintfCRLF();
		}
	}
}

// ##################################################################################
// #               RTC FUNCTIONS                                                    #
// ##################################################################################
int gsm_set_rtc( char *time ){
	char buffer[50];
	
	rprintfInit(gsm_send_byte);
	
	sprintf(buffer, "AT+CCLK=%s\n", time);
	rprintfStr(buffer);
	_delay_ms(500);
  return TRUE;
}

//-----------------------------------
int gsm_get_rtc_str( char *time ){
  char line[200];
  char buffer[50];
  int i;
  
  gsm_command( "AT+CCLK?" );
  
  for(i=0; i<200; i++){
		if( gsm_readline(line) == TRUE ){
		  if( instr(line, "+CCLK") != 0 ) continue;
      if( read_token(line, buffer, 1, " ") != 0 ) continue;
      //delete_char( buffer, '"' );
      strcpy( time, buffer );
      sei();
      return TRUE;
		}
	}
  
  return FALSE;
}

//----------------------------------------------------------------------------------
int gsm_get_rtc( GSM_RTC *rtc ){
	char time[30], buffer[30], token[30];
	
	
	// Get RTC in string format
	if( gsm_get_rtc_str(time) == TRUE ){
		// Split Date
		if( read_token(time, buffer, 0, ",") != 0 ) return FALSE;
		// Convert RTC date to digit values
		
		if( read_token(buffer, token, 0, "/") == 0 )
			rtc->year	=	atoi(token+1);				// token+1 is for skiping " character
		else
			return FALSE;
 
		if( read_token(buffer, token, 1, "/") == 0 ) rtc->month	=	atoi(token); else return FALSE;
		if( read_token(buffer, token, 2, "/") == 0 ) rtc->day		=	atoi(token); else return FALSE;
		// Split Time
		if( read_token(time, buffer, 1, ",") != 0 ) return FALSE;
		if( read_token(buffer, token, 0, ":") == 0 ) rtc->hour	=	atoi(token); else return FALSE;
		if( read_token(buffer, token, 1, ":") == 0 ) rtc->minute=	atoi(token); else return FALSE;
		if( read_token(buffer, token, 2, ":") == 0 ) rtc->second=	atoi(token); else return FALSE;
		return TRUE;
	}
	
	rprintfStr("Oops"); rprintfCRLF();
	return FALSE;
}

//----------------------------------------------------------------------------------
int gsm_str2rtc( char *time_str, GSM_RTC *time ){
  char token[5], date_str[15], clock_str[15];
  
  if( read_token( time_str, date_str, 0, " " ) != 0 ) return FALSE;
  if( read_token( time_str, clock_str, 1, " ") != 0 ) return FALSE;
  
  // Convert Date string to numbers
  read_token( date_str, token, 0, "/" );
  time->year = atoi( token );
  
  read_token( date_str, token, 1, "/" );
  time->month = atoi( token );
  
  read_token( date_str, token, 2, "/" );
  time->day = atoi( token );
  
  // Convert Clock string to numbers
  read_token( clock_str, token, 0, ":" );
  time->hour = atoi( token );
  
  read_token( clock_str, token, 1, ":" );
  time->minute = atoi( token );
  
  read_token( clock_str, token, 2, ":" );
  time->second = atoi( token );
  
  return TRUE;
}

// ##################################################################################
// #               LOW LEVEL FUNCTIONS                                              #
// ##################################################################################

int gsm_command(char *command ){
	//gsm_rprintf_init();
	rprintfInit(gsm_send_byte);
	
	rprintfStr( command );
	rprintfChar('\n');
	_delay_ms(500);
	return TRUE;
}

//----------------------------------
int gsm_readline( char *str ){
	int i;
	static char last_char = 0;
	char *p = str;			// backup from str pointer
	
	*str = 0;
	if( IS_ASCII(last_char) ){
		*str = last_char;	// Recover last unsaved character
		str++;
		*str = 0;					// Terminate string with null
	}
	
	while(1){
		i = gsm_get_byte();
		
		// Check any byte available 
		if( i != -1 ){
			// Check EOL characters
			if( i == '\n' || i == '\r' ){
				// Waste any other EOF characters in current line
				do{
					i = gsm_get_byte();
				}while( i == '\n' || i == '\r' );
				
				// store last char
				last_char = i;
				break;		// @while
			}
			
			// store current ASCII charcater
			if( IS_ASCII(i) ){
				*str = i;	str++;
				*str = 0;					// Terminate string with null
			}
		}else{
			last_char = 0;
			return FALSE;
		}
	}
	
	str = p;					// Recover str pointer
	// Check str lenght
	if( strlen(str) > 0 )
		return TRUE;
	else
		return FALSE;
}


