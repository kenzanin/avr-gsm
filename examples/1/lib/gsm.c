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
	
	/*
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
	*/
}

// ##################################################################################
// #               SMS FUNCTIONS																								#####
// ##################################################################################

void gsm_text_sms(void){
	gsm_command("AT+CMGF=1");
}

//----------------------------------
int gsm_check_new_sms( SMS *sms ){
	int i, location;
	static char index[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	char *index_p = index;
	char line[300], token[50];
	
	// Check index for new sms
	for(i=0; i<15; i++){
		if( index[i] != 0 ){
			gsm_read_sms( index[i], sms );
			location = index[i];
			index[i] = 0;
			return location;
		}
	}

	
	//gsm_flush_buffer();
	gsm_command("AT+CMGL");
	_delay_ms(1000);
	
	// Make a finite loop
	for(i=0; i<200; i++){
		if( gsm_readline(line, sizeof(line)) == TRUE ){
			if( strstr(line, "REC UNREAD") != NULL ){
				// CMGL output format :
				// +CMGL: 3,"REC UNREAD","+989372391694","","2011/08/27 09:12:04+18"
				gsm_read_token( line, token, sizeof(token), 0, "," );
				gsm_read_token( token,token, sizeof(token), 1, ":" );
				// store unread SMS index
				*index_p = atoi( token );
				index_p++;
				// Reset loop counter
				i = 0;
			}
		}
	}
	
	// Check index for new sms
	for(i=0; i<15; i++){
		if( index[i] != 0 ){
			//lcd_clear(); rprintfInit(lcd_putchar); rprintf("Rindex=%d", index[i]); _delay_ms(1000);
			gsm_read_sms( index[i], sms );
			location = index[i];
			index[i] = 0;
			return location;
		}
	}
	
	// return 0 if no new sms available
	return 0;
}

//----------------------------------
int gsm_read_sms( int index, SMS *sms ){
	int i;
	char line[200], token[200];
	
	//uartFlushReceiveBuffer(gsm_nUart);
	//gsm_rprintf_init();
	
	gsm_flush_buffer();
	rprintfInit(gsm_send_byte);
	rprintf("AT+CMGR=%d\n", index);
	_delay_ms(1000);
	
	// Make a finite loop
	for(i=0; i<200; i++){
		if( gsm_readline(line, sizeof(line)) == TRUE ){
			if( strstr(line, "+CMGR") != NULL ){
				// CMGR output format :
				// +CMGR: "REC READ","+989372391694","","2011/08/27 09:10:06+18"
				// Message Body
				
				if( strstr(line, "REC READ") != NULL )
					sms->stat = SMS_READ;
				else if( strstr(line, "REC UNREAD") != NULL )
					sms->stat = SMS_UNREAD;
				
				// store sms Number in sms->number
				gsm_read_token( line, token, sizeof(token), 1, "," );
				// Remove " from start and end of token
				gsm_remove_char(token, token, '"');
				strcpy(sms->number, token );
				
				// store sms Date in sms->date
				gsm_read_token( line, token, sizeof(token), 3, "," );
				gsm_remove_char(token, token, '"');
				strcpy(sms->date, token );
				
				while( gsm_readline(line, sizeof(line)) == FALSE );
				strcpy(sms->body, line);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

//----------------------------------
int gsm_send_sms( char *number, char *text, int try_num ){
	int try, i;
	int refnum=0;
	char line[100], token[50];
	
	for( try=0; try<=try_num; try++ ){
		//lcd_clear(); rprintfInit(lcd_putchar); rprintf("Try%d", try); _delay_ms(1000);
		// Send sms ------------------
		gsm_flush_buffer();	rprintfInit(gsm_send_byte);
		rprintf("AT+CMGS=\"");	rprintfStr(number);	rprintf("\"\n"); rprintfStr(text); gsm_send_byte(26);
		_delay_ms(3000);
		
		// wait for getting SMS refnum
		for(i=0; i<50; i++){
			// wait for new line
			_delay_ms(100);
			if( gsm_readline(line, sizeof(line)) != TRUE ) continue;
			// send SMS response:
			// AT+CMGS="09372391694"
			// > SMS Body...
			// +CMGS: 157
		
			// check for error
			if( strstr(line, "ERROR") != NULL ) return FALSE;
			// check +CMGS for refnum
			if( strstr(line, "+CMGS") != NULL ){
				gsm_read_token(line, token, sizeof(token), 1, ":");
				refnum = atoi(token);
				if( try_num == 0 )
					return refnum;
				else
					break;
			}
		}

		// check refnum
		if( refnum <= 0 ) return FALSE;
		
		// Check delivery phase ---------------
    for(i=0; i<40; i++){
			// Wait for new line
			_delay_ms(500);
			if( gsm_readline(line, sizeof(line)) != TRUE ) continue;
		
			// check +CDS in new line
			if( strstr(line, "+CDS: 6") == NULL ) continue;
		
			// read second field of +CDS line as delivery ref number
			if( gsm_read_token(line, token, sizeof(token), 1, ",") != 0 ) continue;
		
			// Compare delivery refnum with given refnum
			if( atoi(token) != refnum ) continue;
		
			// check delivery report value (for Operators that that contain , in date like Irancell)
			// Example: +CDS: 6,46,"+989372391694",145,"11/10/13,11:22:45+14","11/10/13,11:22:50+14",0
			if( gsm_read_token(line, token, sizeof(token), 8, ",") == 0 )
				if( atoi(token) == 0 ) return TRUE;
			
			// check delivery report value (for Operators that that contain / in date like MCI)
			// Example: +CDS: 6,26,"+099372391694",129,"2011/10/13 13:08:35+14","2011/10/13 13:08:39+14",0
			if( gsm_read_token(line, token, sizeof(token), 6, ",") == 0 )
				if( atoi(token) == 0 ) return TRUE;
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
		if( gsm_readline(line, sizeof(line)) == TRUE ){
			if( strstr(line, "+CPMS") != NULL ){
				// +CPMS output format :
				// +CPMS: "SM",used,total,"SM",used,total,"SM",used,total
				
				p = strstr(line, "SM");		// Find SIM storage location
				if( gsm_read_token( p, token, sizeof(token), 1, "," ) != 0 ) return -1;
				used = atoi(token);
				
				if( gsm_read_token( p, token, sizeof(token), 2, "," ) != 0 ) return -1;
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
	//lcd_clear(); lcd_print("del-1"); _delay_ms(10); lcd_clear();
	rprintfInit(gsm_send_byte);
	//lcd_clear(); lcd_print("del-2"); _delay_ms(10); lcd_clear();
	rprintf("AT+CMGD=%d\n", index);
	//lcd_clear(); lcd_print("del-3"); _delay_ms(10); lcd_clear();
	_delay_ms(500);
	
	return TRUE;
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
		if( gsm_readline(line, sizeof(line)) == TRUE ){
		  if( strstr(line, "+CCLK") == NULL ) continue;
      if( gsm_read_token(line, buffer, sizeof(buffer), 1, " ") != 0 ) continue;
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
		if( gsm_read_token(time, buffer, sizeof(buffer), 0, ",") != 0 ) return FALSE;
		// Convert RTC date to digit values
		
		if( gsm_read_token(buffer, token, sizeof(token), 0, "/") == 0 )
			rtc->year	=	atoi(token+1);				// token+1 is for skiping " character
		else
			return FALSE;
 
		if( gsm_read_token(buffer, token, sizeof(token), 1, "/") == 0 ) rtc->month	=	atoi(token); else return FALSE;
		if( gsm_read_token(buffer, token, sizeof(token), 2, "/") == 0 ) rtc->day		=	atoi(token); else return FALSE;
		// Split Time
		if( gsm_read_token(time, buffer, sizeof(token), 1, ",") != 0 ) return FALSE;
		if( gsm_read_token(buffer, token,sizeof(token), 0, ":") == 0 ) rtc->hour	=	atoi(token); else return FALSE;
		if( gsm_read_token(buffer, token,sizeof(token), 1, ":") == 0 ) rtc->minute=	atoi(token); else return FALSE;
		if( gsm_read_token(buffer, token,sizeof(token), 2, ":") == 0 ) rtc->second=	atoi(token); else return FALSE;
		return TRUE;
	}
	
	rprintfStr("Oops"); rprintfCRLF();
	return FALSE;
}

//----------------------------------------------------------------------------------
int gsm_str2rtc( char *time_str, GSM_RTC *time ){
  char token[5], date_str[15], clock_str[15];
  
  if( gsm_read_token( time_str, date_str, sizeof(date_str),  0, " " ) != 0 ) return FALSE;
  if( gsm_read_token( time_str, clock_str,sizeof(clock_str), 1, " ") != 0 ) return FALSE;
  
  // Convert Date string to numbers
  gsm_read_token( date_str, token, sizeof(token), 0, "/" );
  time->year = atoi( token );
  
  gsm_read_token( date_str, token, sizeof(token), 1, "/" );
  time->month = atoi( token );
  
  gsm_read_token( date_str, token, sizeof(token), 2, "/" );
  time->day = atoi( token );
  
  // Convert Clock string to numbers
  gsm_read_token( clock_str, token, sizeof(token), 0, ":" );
  time->hour = atoi( token );
  
  gsm_read_token( clock_str, token, sizeof(token), 1, ":" );
  time->minute = atoi( token );
  
  gsm_read_token( clock_str, token, sizeof(token), 2, ":" );
  time->second = atoi( token );
  
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
		if( gsm_readline(line, sizeof(line)) == TRUE ){
			if( strstr(line, "+CUSD") != NULL ){
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
		if( gsm_readline(line, sizeof(line)) == TRUE ){
			rprintfStr(line);
			rprintfCRLF();
		}
	}
}

//-----------------------------------
int gsm_read_token( char *src, char *dest, char lenght, char nToken, char *delimiter ){
  int i;
  char *temp_p = malloc( strlen(src) );
  char *token;
  
  // make backup from src and store in temp_p space
  strcpy( temp_p, src );
  token = strtok( temp_p, delimiter );
  
  // seek to specified token
  for(i=1; i<=nToken; i++)
    token = strtok( NULL, delimiter );
  
  // return specified token if availabe
  // AND check lenght of this token
  if( token != NULL && strlen(token) < lenght){
    strcpy(dest, token);
    free( temp_p );
    return 0;
  }
  
  // Terminate destination with null and return error code
  *dest = 0;
  free( temp_p );
  return 1;
}

//-----------------------------------
void gsm_remove_char( char *dest, char *src, char c ){
	char *buffer = malloc( strlen(src) );
	char *p = buffer;
	
	printf("%s\n", src);
	
	for( ; *src != 0; src++ ){
		if( *src != c ){
			*p = *src;
			p++;
		}
	}
	
	*p = 0;
	strcpy(dest, buffer);
	free( buffer );
}

//-----------------------------------
int gsm_command(char *command ){
	//gsm_rprintf_init();
	rprintfInit(gsm_send_byte);
	
	rprintfStr( command );
	rprintfChar('\n');
	_delay_ms(1000);
	return TRUE;
}

//-----------------------------------
void gsm_flush_buffer( void ){
	int i;
	// Read uart buffer until no byte available
	do{
		i = gsm_get_byte();
	}while( i != -1 );
}

// ##################################################################################
// #               LOW LEVEL FUNCTIONS                                              #
// ##################################################################################
int gsm_readline( char *str, int lenght ){
	int i;
	int count=0;
	// make a backup from str pointer
	char *p = str;
	
	// Terminate string with null
	*str = 0;
	
	while(1){
		// Read serial input buffer
		i = gsm_get_byte();
		// break from while if no byte available
		if( i == -1 ) break;
		
		// check for ascii characters
		if( i >= 0x20 && i <= 0x7F ){
			if( count < lenght ){
				*str = i;
				str++; count++;
				*str = 0;
			}
		// check carriage return and line feed characters
		}else if( i == '\r' || i == '\n' ){
			break;
		}
	}
	
	// Recover str pointer for check lenght of recieved line
	str = p;
	if( strlen(str) > 0 )
		return TRUE;
	else
		return FALSE;
}

/*
//----------------------------------
int gsm_readline( char *str ){
	int i;
	static char last_char = 0;
	char *p = str;			// backup from str pointer
	
	*str = 0;
	//if( IS_ASCII(last_char) ){
	if( last_char >= 0x20 && last_char <= 0x7F ){
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
			//if( IS_ASCII(i) ){
			if( i >= 0x20 && i <= 0x7F ){
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
*/
