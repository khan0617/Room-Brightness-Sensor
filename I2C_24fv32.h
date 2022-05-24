/* 
 * File:   I2C_24fv32.h
 * Author: hamza
 *
 * Created on March 26, 2021, 8:20 AM
 */

#ifndef I2C_24FV32_H
#define	I2C_24FV32_H

#include <xc.h>

#ifdef	__cplusplus
extern "C" {
#endif

    void lcd_printChar(char Package);
    void lcd_printStr(const char s[], char row);
    void lcd_cmd(char Package);
    void lcd_setCursor(char x, char y);
    void lcd_init();
    void delay_ms(int ms);



#ifdef	__cplusplus
}
#endif

#endif	/* I2C_24FV32_H */

