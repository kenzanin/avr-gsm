//*****************************************************************************
//
// File Name	: 'gsm.h'
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


#ifndef GSM_H
#define GSM_H

#include "global.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <my-string.h>
#include <util/delay.h>
#include "rprintf.h"
#include "uart2.h"
#include "lcd.h"


// Structers	
typedef struct struct_sms{
	char body[50];
	char number[15];
	char date[25];
	char stat;
}SMS;

typedef struct struct_time{
	unsigned char hour, minute, second;
	unsigned char year, month, day;
}GSM_RTC;

#define		SMS_UNREAD	0
#define		SMS_READ		1

// Functions ------------

// Initial GSM library
void gsm_init( void(*sendByte_func)(unsigned char c), int(*getByte_func)(void) );

void gsm_debug( char *command, void(*sendByte_func)(unsigned char c) );

// Send AT command to module
int gsm_command(char *command );

// Going GSM module in text mode (at+cmgf=1)
void gsm_text_sms(void);

// Send sms to specific number, return RefNum of SMS
int gsm_send_sms(char *number, char *text);

// send sms and check delivery report, if not delivered try again
int gsm_send_sms2( char *number, char *text, int try_num );

// Cehck SMS delivery report, return TRUE of FALSE
int gsm_check_delivery( int refnum );

// Read specific sms
int gsm_read_sms( int index, SMS *sms );

// Check new sms and return number of new SMSes, also fill new sms indexes in *index pointer
int gsm_check_new_sms( char *index );

// Delete sms
int gsm_del_sms( int index );

// Return SIM memory used in percent
int gsm_SIM_mem_used( int *total_mem );

// Return Signl quality in percent
int gsm_signal_quality(void);

// ---------------------------- RTC Functions ----------------------
// Set Reale Time Clock to GSM Module
// time format: "YY/MM/DD,HH:MM:SS+00"
// NOTE: " included in response
int gsm_set_rtc( char *time );

// get Reale Time Clock
// NOTE: return time is in above format and contain " is response
int gsm_get_rtc_str( char *time );

// Get rtc in digit format
int gsm_get_rtc( GSM_RTC *rtc );

// Convert a string to GSM_RTC format
int gsm_str2rtc( char *time_str, GSM_RTC *time );

// ---------------------------- Other Functions ----------------------
// Send "Unstructured supplementary service" command and return response
// for example irancell get credit command is *141*1#
int gsm_usd_command( char *cmd, char *response );

// Read a line from uart buffer
int gsm_readline( char *str );

// Initial rprintf function for send bytes to uart
void gsm_rprintf_init( void );

#endif

