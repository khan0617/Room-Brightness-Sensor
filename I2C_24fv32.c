/* 
 * File:   khan_I2CLCD.h
 * Author: hamza
 * This library encompasses the functions needed to send commands to an LCD using I2C.
 * Functions include:
 * lcd_cmd(char package): send a package to the LCD to do something
 * lcd_printChar(char package): display package on the LCD
 * lcd_init(): initialize the LCD using its documented init sequence in the datasheet.
 * lcd_printStr(char s[], char row): Print a string starting in the row of specified
 *         (row == 0 means top row of LCD, row == 1 means bottom row of LCD).
 * lcd_setCursor(char x, char y): Set the cursor of the display (where it draws next)
 *              to the specified location.
 * 
 * 
 * Created on March 26, 2021, 8:20 AM
 */

#include <xc.h>
#include "I2C_24fv32.h"
#include <string.h>

#define CONTRAST 0b01110000 //default display contrast value
volatile int counter = 0; //keeps track of cursor position for shifting purposes.

void lcd_init(){
    I2C1CONbits.I2CEN = 0; //disable I2C1 before changing baud rate
    I2C1BRG = 157; //set baud rate w/ clock frequency = 100kHz & pic frequency = 16MHz
    
    IFS1bits.MI2C1IF = 0; //clear interrupt flag
    I2C1CONbits.I2CEN = 1; //re-enable I2C mode for I2C1 pins
    
    delay_ms(40);
    
    lcd_cmd(0b00111000); //function set
    lcd_cmd(0b00111001); //function set (2))
    lcd_cmd(0b00010100); //Internal OSC Frequency set
    lcd_cmd(CONTRAST); //contrast set
    lcd_cmd(0b01010110); //power control
    lcd_cmd(0b01101100); //Follower Control
    
    delay_ms(200); //power stabilization delay
    
    lcd_cmd(0b00111000); //function set
    lcd_cmd(0b00001100); //display ON/OFF control
    lcd_cmd(0b00000001); //clear display
    
    delay_ms(1);
}

void lcd_printChar(char Package){
    IFS1bits.MI2C1IF = 0;//reset interrupt flag for good measure
    
    I2C1CONbits.SEN = 1; //initiate start condition
    while(I2C1CONbits.SEN == 1); //wait for SEN to clear (when START bit is complete)
    
    IFS1bits.MI2C1IF = 0;
    I2C1TRN = 0b01111100; //send 8bit slave address
    while(IFS1bits.MI2C1IF == 0); //wait until IFS1bits.MI2C1IF == 1
    
    IFS1bits.MI2C1IF = 0; 
    
    I2C1TRN = 0b01000000; //8bits of control byte, w/ RS = 1 (since we are writing a packet to LCD)
    while(IFS1bits.MI2C1IF == 0); //wait until IFS1bits.MI2C1IF == 1
    
    IFS1bits.MI2C1IF = 0;
    I2C1TRN = Package;
    while(IFS1bits.MI2C1IF == 0);
    
    I2C1CONbits.PEN = 1; //initiate STOP bit
    while(I2C1CONbits.PEN == 1); //wait until STOP bit is sent and acknowledged by LCD screen. 
    counter++;
}

void lcd_setCursor(char x, char y){
    counter = y;
    char pos = x*0x40 + y; //using Formula on page 13 of Lab 5 manual: Package will set the cursor position accordingly
    char Package = 0b10000000 | pos; //This ensures that the MSB of package is always 1 (required to change cursor position)
    lcd_cmd(Package);
}

void lcd_cmd(char Package){
    //first, send the START bit
    IFS1bits.MI2C1IF = 0;//reset interrupt flag for good measure
    I2C1CONbits.SEN = 1;
    while(I2C1CONbits.SEN == 1);
    
    IFS1bits.MI2C1IF = 0;//reset interrupt flag for good measure
    I2C1TRN = 0b01111100; //slave address with R/nW bit from datasheet 
    while(IFS1bits.MI2C1IF == 0); //wait for MI2C1IF to == 1
    
    IFS1bits.MI2C1IF = 0; //clear interrupt flag
    I2C1TRN = 0b00000000; //send control byte to LCD screen
    while(IFS1bits.MI2C1IF == 0); //again wait for MI2C1IF to == 1
    
    IFS1bits.MI2C1IF = 0;
    I2C1TRN = Package; //send 8 bits consisting of data byte.
    while(IFS1bits.MI2C1IF == 0); 
    
    I2C1CONbits.PEN = 1; //send STOP bit to lcd screen
    while(I2C1CONbits.PEN == 1); //wait until PEN clears such that STOP bit is complete.
}

void lcd_printStr(const char s[], char row){
    int i;
    char controlByte;
    int length = strlen(s); //calculate the length of s
    
    IFS1bits.MI2C1IF = 0; //reset interrupt flag for good measure
    I2C1CONbits.SEN = 1; //initiate start condition
    while(I2C1CONbits.SEN == 1); //wait for SEN to clear (when START bit is complete)
     
    IFS1bits.MI2C1IF = 0;
    I2C1TRN = 0b01111100; //send 8bit slave address
    while(IFS1bits.MI2C1IF == 0); //wait until IFS3bits.MI2C1IF == 1
    
    for(i = 0; i < length; i++){ //implement printChar, but for Strings.
        IFS1bits.MI2C1IF = 0;
        if(counter >= 7){ //we're about to run off the right edge of the display, stop sending characters to LCD and reset cursor.
            controlByte = 0b01000000;
        }
        else{         
            if(i == (length - 1)){ //we're writing the last bit to the lcd now
                controlByte = 0b01000000;
            }
            else{ //not currently writing the last byte of the string
                controlByte = 0b11000000;
            }
        }     
        I2C1TRN = controlByte; //8bits of control byte, w/ RS = 1 (since we are writing a packet to LCD), unless it's the last bit.
        while(IFS1bits.MI2C1IF == 0); //wait until IFS1bits.MI2C1IF == 1
        IFS1bits.MI2C1IF = 0;
        I2C1TRN = s[i]; //write s[i] to the lcd screen
        while(IFS1bits.MI2C1IF == 0);
        
        counter++;
        
        if(counter >= 8){ //exit the for loop before sending more data that will run off the display
            break;
        }

    }
    
    I2C1CONbits.PEN = 1; //initiate STOP bit
    while(I2C1CONbits.PEN == 1); //wait until STOP bit is sent and acknowledged by LCD screen.   
//RECURSIVE call below
//    if(counter >= 8 && i != length - 1){ //if the cursor has reached the end of the LCD AND if we didn't successfully print the whole string:
//        i++; //need to increment i because technically we have already printed s[i]
//        int j;
//        int c = 0;
//        char copyOver[length - i]; //create a new array to copy the remaining characters of s[]
//        for(j = i; j < length; j++){ //copy over the leftover characters so we can call the printStr() again on remaining portion of the string
//            copyOver[c] = s[j];
//            c++;
//        }
//        counter = 0;
//        if(length >= 8){ //the string is too long to display on just one row of the lcd display, so put the rest of it on the other row.
//            row = 1 - row;
//        }
//        lcd_setCursor(row, 0); //reset the column position of the cursor and maintain our row position   
//        lcd_printStr(copyOver, row); //recursively call printStr on remainder of string.
//    }
//    else if(counter >= 8){ //i == length - 1, so we were able to write the string successfully. just reset counter and cursor position.
//        counter = 0;
//        lcd_setCursor(row, 0);
//    }
}  

