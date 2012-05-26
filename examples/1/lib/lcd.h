#ifndef LCD_H
#define LCD_H

#include "global.h"
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "rprintf.h"

#define		LCD_PORT	PORTA
#define		LCD_DDR		DDRA
#define		LCD_RS		PA2
#define		LCD_E		  PA3
#define		LCD_DB4 	PA4
#define		LCD_DB5 	PA5
#define		LCD_DB6 	PA6
#define		LCD_DB7 	PA7

#define		LCD_CLEAR			    0x01
#define		LCD_HOME			    0x02
#define		LCD_CURSOR_OFF	  0x0C
#define		LCD_CURSOR_ON		  0x0E
#define		LCD_CURSOR_BNLINK	0x0F


#define	LCD_ROW		  2
#define	LCD_COLUMN	16

void lcdReset(void);
void lcdInit(void);

void lcd_print(char *str);
void lcd_putchar(unsigned char c);

void lcd_home(void);
void lcd_clear(void);
void lcd_gotoXY(char x, char y);
void lcd_create_char( int location, char *c );

void lcd_command(unsigned char command);

void lcdWriteByte(unsigned char byte);

#endif

