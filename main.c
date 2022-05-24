/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device            :  PIC24FV32KA302
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.61
        MPLAB 	          :  MPLAB X v5.45
*/

/**
  Section: Included Files
*/
#include "mcc_generated_files/system.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#define MAX_CURRENT 5 //maximum current from photodiode as a constant. QUESTION: is this 4uA or 5uA? We computed gain based on 5uA.
volatile int overflow = 0;
volatile int tarePressedFlag = 0;
volatile long raw_data = 0;
volatile long tare_data = 0;

//Timer1 interrupt occurs every 100ms. Increment out overflow variable counter on interrupt.
void __attribute__((interrupt, auto_psv)) _T1Interrupt(){
    _T1IF = 0; //reset Timer1 interrupt flag.
    if(overflow < 5){ //make sure overflow can never be larger than 5 value.
        overflow++;
    }
}

void delay_ms(int ms){ //create a delay of about "ms" milliseconds
    while(ms-- > 0){
        asm("repeat #997");
        asm("nop");
    }
}

void timer_init(){ //initialize timer1's bits to start counting.
    T1CON = 0; //reset T1CON
    T1CONbits.TCKPS = 0b01; //set pre-scaler to 8
    TMR1 = 0; //reset TMR1 counter
    PR1 = 12500 - 1; //100ms period for timer1, where Tcy = 1/(10^6) seconds, or 1 usec.
    
    IFS0bits.T1IF = 0; //Clear the Timer1 interrupt status flag
    IEC0bits.T1IE = 1; //enable timer1 interrupts
    T1CONbits.TON = 1; //turn on Timer1
}

long readFromADC(){ //read voltage data from ADC and return it as a long int. only bottom 24 bits of return are actual data.
    long ADC_bits;
    long voltage_data = 0;
    long msb = 0;
    long lsb = 0;
    PORTBbits.RB15 = 0; //slave select is active LOW, ready to read from ADC
    msb = SPI1_Exchange16bit(0); //send garbage data to the ADC (0x0) and read its data in in 2 16 bit chunks.
    lsb = SPI1_Exchange16bit(0);
    PORTBbits.RB15 = 1; //slave select is idle HIGH, we're done reading from ADC
    lsb = lsb & 0x0000FFFF; //make sure 16 MSB of this variable are 0's
    ADC_bits = (msb << 16) | lsb; //concatenate MSB and LSB to have a full 32 bit number
    
    //EOC CHECKING CODE BELOW. Check if the was done with the conversion when we tried to read from it.
    if(msb & 0x00008000){ //if bit 16 is high, this means conversion was not finished in the ADC.
        voltage_data = -1;
    }
    else{
        voltage_data = (ADC_bits & 0x0FFFFFF0) >> 4;
    }
    //EOC CHECKING CODE ABOVE
    
    
    //CH0 AND CH1 CHECKING CODE BELOW!! NEED THIS IF INPUTS AREN'T TIED TOGETHER!
    //now check if the CH1/CH0 bit (bit #30) is ONE, meaning the ADC is sending data for channel 1 (which we are using).
//    if(msb & 0x00004000){
//        //for the voltage data, MSB = bit 27, LSB = bit 4.
//        voltage_data = (ADC_bits & 0x0FFFFFF0) >> 4;
//                    //corresponding hex bits to mask:31-28, 27-24, 23-20, 19-16, 15-12, 11-8, 7-4, 3-0
//                    //discard bits 31-28 as well as 3-0, since those are not the digital voltage data.
//    }
//    else{ //ADC sent data from ch0, which we don't care about. We'll return -1 to signify this.
//        voltage_data = -1;
//    }
    //CH0 AND CH1 CHECKING CODE ABOVE!!
    
    return voltage_data;
} //end readFromADC()

void writeToLCD(const char top[], const char bottom[]){
        //top[] is the string we want on the top row of the LCD.
        lcd_cmd(0b00000001); //send a CLEAR DISPLAY signal to the lcd 
        lcd_setCursor(0, 0); //reset cursor position to top left of LCD
        lcd_printStr(top, 0);
        lcd_setCursor(1, 0); //set the cursor to the bottom row so we can print the "bottom" string.
        lcd_printStr(bottom);
}

void writeToUart(int32_t data){
    //this function should send 24 chars over the UART communication protocol
    //each char is either '0' or '1', representing a corresponding bit of the raw ADC data
    int i;
    for(i = 23; i >= 0; i--){ //send the MSB of "data" first
        if((data>>i) % 2){
            UART1_Write('1');
        }
        else{
            UART1_Write('0');
        }
    }
    UART1_Write('\n');
    UART1_Write('\r');
}

int main(void) //"EE3102_proj" main function
{
    // initialize the device
    SYSTEM_Initialize();
    timer_init();
    lcd_init();
    lcd_setCursor(0, 0);
    
    //DEBUGGING: these 2 lines are simply for toggling an LCD upon button press. Not needed in final code.
//    TRISBbits.TRISB8 = 0; //RB8 is output
//    LATBbits.LATB8 = 0; //RB8 starts LOW
    
    delay_ms(1);

    //in the below while loop, we need to do multiple things, in this order:
    //1.)readFromADC to get raw voltage data using SPI
        //convert this raw data to a current
    //2.)format a string to print to the LCD, accounting for the "tare" button subtraction
        //Allow for tare_button to be reset as well.
    //3.) Print to LCD about 3-5 times per second
        //A writeToLCD function handles this.
    //4.) Load UART register to send data for debugging/resolution purposes
        //Send '1' or '0' as char values through UART.
        //This is done in the writeToUart function, which sends 24 individual chars, each a '0' or '1'.
    long adc_data = 0;
    float ratio = 0;
    float current = 0, tare_current = 0;
    
    while (1)
    {   
        char output[10]; //this is the buffer we will use to format ADC data to a string.
        
        //read data from ADC. If it's -1, read again so we get valid data
        adc_data = readFromADC();
        
        //if adc data is valid (meaning it's not -1), update our global variable, raw_data.
        if(adc_data != -1){
            raw_data = adc_data;
        }
        
        //calculate the value of the current being fed into the op amp, as well as the tare_current if applicable.
        ratio = raw_data / (pow(2, 24) - 1);
        current = ratio * MAX_CURRENT;
        tare_current = (tare_data / (pow(2, 24) - 1)) * MAX_CURRENT;
        float current_diff = current - tare_current;
        
        //format out string so we may print to the LCD
        if(current_diff < 0){
            sprintf(output, "%.5f", current_diff); 
        }
        else{
            sprintf(output, "+%.5f", current_diff); 
        }
        
        //write to the LCD as well as transmit our data over UART.
        writeToLCD(output, "Unit: uA");
        writeToUart(raw_data);
        
        //Reaching this section of code takes about 145ms from the start of the while loop (figured out using debugger/TMR1).
        //We want to write to the LCD about once every 200ms, so let's add a delay of 60ms using our delay function.
        delay_ms(60);
    }

    return 1;
}
/**
 End of File
*/

