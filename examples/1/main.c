#include "global.h"
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"
#include "lcd.h"
#include "gsm.h"

int main(void){
	int i;
	SMS sms;
	
	// Initial Serial port
	uartInit();
	uartSetBaudRate(9600);
	
	// Initial LCD
	lcdInit();
	
	// Initial GSM Module
	gsm_init(uartSendByte, uartGetByte);		// Initial gsm library
	gsm_text_sms();				// Initial SMS text-mode message
	

	
	while(1){
		i = gsm_check_new_sms( &sms );
		if( i != 0 ){
			gsm_del_sms( i );
			// We have new SMS
			lcd_clear(); lcd_print(sms.body);
			lcd_gotoXY(0, 1); lcd_print(sms.number);
			// Send reply
			gsm_send_sms( sms.number, "Your SMS Recieved!", 2 );
		}
		
		_delay_ms(2000);
	}
	
	return 0;
}

