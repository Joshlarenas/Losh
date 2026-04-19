/*
EEL 4742C - UCF

Code that prints a welcome message to the pixel display.
*/



/*
 * CODE EXPLAINATION BY: JOSHUA LARENAS & LARRY SAKUDA
 * PROJECT CHOICE: FLIGHT SIMULATOR USING BOTH THE ACCELOMETER AND JOYSTICK
 * PROJECT TASKS: ACCELOMETER INITIALIZATION (LARRY) / JOYSTICK INITIALIZATION (JOSHUA LARENAS) / PIXEL DISPLAY INITIALIZATION & PROJECT CONFIGURATION (BOTH LARRY AND JOSH)
 * RESOURCES: DATASHEETS, LCD IMAGE FILE GIVEN IN LAB MANUAL, LAB MANUEL
 *
 * SUMMARY: THE WAY OUR CODE IS SET UP FOLLOWS THE FILE AND FOLDER FORMAT OF THE GIVEN "EEL4742C LAB PIXEL DISPLAY" FROM THE LAB MANUAL.
 *          I USED THE FOLDER TO AVOID FILE TRANSFER MISTAKES AND JUST USED THE MAIN.C THAT WAS GIVEN AS A TEMPLATE FOR THE PIXEL DISPLAY.
 *          OUR LOGIC GOES LIKE THIS, WE HAVE 2 MODES THAT ALLOW FOR THE SIMULATION TO MOVE. THE FIRST/DEFAULT IS THE ACCELOMTER WHILE THE
 *          SECOND IS THE JOYSTICK, THESE MODES CAN SWITCH BY PRESSING THE BUTTON P1.1 OF THE MSP430 BOARD. WE HAVE APPLIED THRESHOLDS THAT ALLOW
 *          US TO MOVE THE PLANCE CAREFULLY, HOWEVER WHEN WE EXCEED OUR THRESHOLD IT GIVES US A WARNING FOLLOWED BY THE BUZZER AS A "REACTION"
 *          TO THE MISTAKE. WITH THAT SAID, WE DO HAVE 2 SETS OF THRESHOLDS FOR EACH MODE. THE FIST THRESHOLD OF EACH MODE IS FOR THE DEVICE TO
 *          "RECOGNIZE" IF THE USER IS GOING LEFT/RIGHT/UP/DOWN OR COMBINATIOS SUCH AS TOP LEFT/TOP RIGHT/ ETC. THE SECOND SETS OF THRESHOLDS IS
 *          TO JUDGE IF THE USER IS GOING TO FAR IN THE DIRECTION THEY ARE GOING.
 *
 * IN TERMS OF HELPER FUNCTIONS, WE HAVE CREATED 2 TO HELP ORGAINIZE THE MAIN FUNCTION.
 *
 *          PRINT DIRECTION FUNCTION: THE FIRST IS THE PRINT DIRECTION FUNCTION, WE USED THIS ORINGALLY FOR UART DURING TESTING HENCE THE NAME, HOWEVER NOW ITS USED TO
 *          JUDGE THE THRESHOLDS AND INPUTS OF THE USER (X AND Y AXIS) TO THEN KNOW WHICH IMAGE TO GENERATE. THE FUNCTION ITSELF USES THE THRESHOLDS FOR
 *          REFRENCE AND USES ITS OWN GLOABL VARIABLE CALLED LASTDIRECTION TO NOT GENERATE THE SAME IMAGE OVER AND OVER AGAIN. THIS PREVENTS SLOW
 *          RESULTS FOR WHICH WE HAVE DEALTH WITH PRIOR.
 *
 *          BUZZ: THIS SIMPLE FUNCTION IS JOR THE USE OF THE BUZZER AND ITS DURATION, IT SETS ITS WAVELENGTH TO HIGH FOR A DURATION GIVEN USING THE FOR LOOP
 *          THE SETS IT BACK TO LOW TO TURN IT BACK OFF. IN ADDITION, WE GAVE IT THE ABILITY TO OVERWRITE THE PREVIOUS IMAGE AND WRITE A WARNING IMAGE SINCE THE BUZZER'S
 *          PURPOSE IS TO INDICATE TO THE USER WHEN THEY ARE GOING OFF TRACK.
 *
 */
#include "msp430fr6989.h"
#include "Grlib/grlib/grlib.h"          // Graphics library (grlib)
#include "LcdDriver/lcd_driver.h"       // LCD driver
#include <stdio.h>
#include "logo.h"

#define redLED BIT0
#define greenLED BIT7
#define S1 BIT1
#define S2 BIT2
#define FLAGS       UCA1IFG
#define RXFLAG      UCRXIFG
#define TXFLAG      UCTXIFG
#define TXBUFFER    UCA1TXBUF
#define RXBUFFER    UCA1RXBUF
#define BUT1 BIT1 // Button S1 at P1.1

Graphics_Context g_sContext;        // Declare a graphic library context

// Accelerometer maps to the following
// P8.4 - X direction
// P8.5 - Y direction
// P8.6 - Z direction
#define XDIRIN  ADC12MCTL0
#define YDIRIN  ADC12MCTL1
#define ZDIRIN  ADC12MCTL2
#define XDIROUT ADC12MEM0
#define YDIROUT ADC12MEM1
#define ZDIROUT ADC12MEM2


// degree constants for buzzer
#define HIGH 2700
#define LOW 1300

// ADC thresholds (12-bit range: 0-4095, center ~2048)
#define THRESHOLD_LOW  1500
#define THRESHOLD_HIGH 2500

// -------------------------------------------------------
// OPT3001 Constants
// -------------------------------------------------------
#define OPT3001_ADDR    0x44    // ADDR pin tied to GND
#define MFR_ID_REG      0x7E    // Manufacturer ID  -> 0x5449
#define DEV_ID_REG      0x7F    // Device ID        -> 0x3001
#define CONFIG_REG      0x01    // Configuration register
#define RESULT_REG      0x00    // Result register

// RN=7, CT=0, M=3, ME=1 → 0x7604
#define CONFIG_VALUE    0x7604


// -------------------------------------------------------
// Function Prototypes
// -------------------------------------------------------
//void Initialize_UART(void);
void Initialize_ADC_Accel(void);
void Initialize_ADC_joystick(void);
void buzz(unsigned int duration_ms);
//void uart_write_char(unsigned char ch);
//void uart_write_string(char *str);
//void uart_write_hex(unsigned int value);
//void uart_write_uint16(unsigned int n);
void print_direction(unsigned int x, unsigned int y);



// ****************************************************************************
void main(void) {
    char mystring[20];

    WDTCTL = WDTPW | WDTHOLD;
       PM5CTL0 &= ~LOCKLPM5;

       unsigned int x_result, y_result;
       unsigned int state = 0;

       P1DIR &= ~(BUT1); // Direct pin as input
       P1REN |= (BUT1); // Enable built-in resistor
       P1OUT |= (BUT1); // Set resistor as pull-up

       //Initialize_UART();
       Initialize_ADC_Accel();

//       uart_write_string("\033[2J");
//       uart_write_string("\033[1;1H");
//       uart_write_string("=== Accelerometer ===\r\n");

    // Configure clock system

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Graphics functions
    Crystalfontz128x128_Init();         // Initialize the display

    // Set the screen orientation
    Crystalfontz128x128_SetOrientation(0);

    // Initialize the context
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);

    // Set background and foreground colors
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);

    // Set the default font for strings
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    //Clear the screen
    Graphics_clearDisplay(&g_sContext);
    ////////////////////////////////////////////////////////////////////////////////////////////

    // Draw the logo centered on screen (128x128 image, 128x128 display)
    Graphics_drawImage(&g_sContext, &StraightClimb4BPP_UNCOMP, 0, 0);

    WDTCTL = WDTPW | WDTHOLD;  // ADD THIS - disable watchdog again after display init

    while(1){
        if ((P1IN & BUT1) == 0) {
            __delay_cycles(200000);
            if ((P1IN & BUT1) == 0) {
                if (state == 0) {
                    state = 1;
                    Initialize_ADC_joystick();
//                    uart_write_string("\033[2J");
//                    uart_write_string("\033[1;1H");
                } else {
                    state = 0;
                    Initialize_ADC_Accel();
//                    uart_write_string("\033[2J");
//                    uart_write_string("\033[1;1H");
                }
                while ((P1IN & BUT1) == 0);
                __delay_cycles(200000);
            }
        }

        if (state == 0) {
            ADC12CTL0 |= ADC12SC;
            __delay_cycles(5000);

            unsigned int x = XDIROUT;
            unsigned int y = YDIROUT;
            unsigned int z = ZDIROUT;

            if (x <= 1200 || x >= 2800 || y <= 1200 || y >= 2800) {
                buzz(100);  // buzz for 100ms
            }
            print_direction(x,y);

            ADC12CTL0 &= ~ADC12SC;
        }
        else {
            ADC12CTL0 |= ADC12SC;
            while(ADC12CTL1 & ADC12BUSY) {}
            x_result = ADC12MEM0;
            y_result = ADC12MEM1;
            if (x_result <= 900 || x_result >= 3100 || y_result <= 900 || y_result >= 3100) {
                buzz(100);  // buzz for 100ms
            }

            print_direction(x_result, y_result);
        }
    }

}


void Initialize_ADC_Accel(void)
{
    // Fully reset everything first
    ADC12CTL0 = 0;
    ADC12CTL1 = 0;
    ADC12CTL2 = 0;
    ADC12CTL3 = 0;
    XDIRIN = 0;
    YDIRIN = 0;
    ZDIRIN = 0;

    P8SEL1 |= (BIT4 | BIT5 | BIT6);
    P8SEL0 |= (BIT4 | BIT5 | BIT6);

    ADC12CTL0 |= ADC12ON | ADC12SHT0_2;
    ADC12CTL1 |= ADC12SHP | ADC12SSEL_0 | ADC12CONSEQ_1;
    ADC12CTL2 |= ADC12RES_2;
    ADC12CTL3 |= ADC12CSTARTADD_0;

    XDIRIN |= ADC12INCH_7;
    YDIRIN |= ADC12INCH_6;
    ZDIRIN |= ADC12INCH_5 | ADC12EOS;

    ADC12CTL0 |= ADC12ENC;
}

void Initialize_ADC_joystick() {

    ADC12CTL0 = 0;
    ADC12CTL1 = 0;
    ADC12CTL2 = 0;
    ADC12CTL3 = 0;
    ADC12MCTL0 = 0;
    ADC12MCTL1 = 0;

    // X-axis: A10/P9.2
    P9SEL1 |= BIT2;
    P9SEL0 |= BIT2;
    // Y-axis: A4/P8.7
    P8SEL1 |= BIT7;
    P8SEL0 |= BIT7;

    ADC12CTL0 |= ADC12ON;
    ADC12CTL0 &= ~ADC12ENC;

    // CTL0: 32 cycles SHT, MSC
    ADC12CTL0 |= ADC12SHT0_3 | ADC12MSC;

    // CTL1: SC trigger, pulse sample, no divider, MODOSC, sequence-of-channels
    ADC12CTL1 |= ADC12SHS_0 | ADC12SHP | ADC12DIV_0 | ADC12SSEL_0 | ADC12CONSEQ_1;

    // CTL2: 12-bit, unsigned binary
    ADC12CTL2 |= ADC12RES_2;
    ADC12CTL2 &= ~ADC12DF;

    // CTL3: start at MEM0
    ADC12CTL3 &= ~ADC12CSTARTADD_0;

    // MCTL0: X-axis, A10, AVCC/AVSS
    ADC12MCTL0 |= ADC12VRSEL_0 | ADC12INCH_10;

    // MCTL1: Y-axis, A7, AVCC/AVSS, end of sequence
    ADC12MCTL1 |= ADC12VRSEL_0 | ADC12INCH_4 | ADC12EOS;

    ADC12CTL0 |= ADC12ENC;
    return;
}


static int lastDirection = -1;

void print_direction(unsigned int x, unsigned int y) {

    int left, right, up, down;

    left  = (x < THRESHOLD_LOW);
    right = (x > THRESHOLD_HIGH);
    up    = (y > THRESHOLD_HIGH);
    down  = (y < THRESHOLD_LOW);

    int current_direction = 0;

    if (left && up)         current_direction = 1;
    else if (right && up)   current_direction = 2;
    else if (left && down)  current_direction = 3;
    else if (right && down) current_direction = 4;
    else if (left)          current_direction = 5;
    else if (right)         current_direction = 6;
    else if (up)            current_direction = 7;
    else if (down)          current_direction = 8;
    else                    current_direction = 0;

    if (current_direction != lastDirection) {
        lastDirection = current_direction;

        if (left && up)
            Graphics_drawImage(&g_sContext, &Climbingleftbank4BPP_UNCOMP, 0, 0); //GOOD
        else if (right && up)
            Graphics_drawImage(&g_sContext, &Ascendingrightbank4BPP_UNCOMP, 0, 0); //GOOD
        else if (left && down)
            Graphics_drawImage(&g_sContext, &Descendingleftbank4BPP_UNCOMP, 0, 0); //GOOD
        else if (right && down)
            Graphics_drawImage(&g_sContext, &Descendingrightbank4BPP_UNCOMP, 0, 0); //GOOD
        else if (left)
            Graphics_drawImage(&g_sContext, &Levelleftbank4BPP_UNCOMP, 0, 0); //GOOD
        else if (right)
            Graphics_drawImage(&g_sContext, &Levelrightbank4BPP_UNCOMP, 0, 0); //GOOD
        else if (up)
            Graphics_drawImage(&g_sContext, &StraightClimb4BPP_UNCOMP, 0, 0); //GOOD
        else if (down)
            Graphics_drawImage(&g_sContext, &StraightDescent4BPP_UNCOMP, 0, 0); //GOOD
        else
            Graphics_drawImage(&g_sContext, &StraightLevel4BPP_UNCOMP, 0, 0); //GOOD
    }
}

void buzz(unsigned int duration_ms) {
    unsigned int i;
    for (i = 0; i < duration_ms; i++) {
        P2OUT |= BIT7;   // pin high
        __delay_cycles(500);
        P2OUT &= ~BIT7;  // pin low
        __delay_cycles(500);
    }
    Graphics_drawImage(&g_sContext, &Warning_clean4BPP_UNCOMP, 0, 0);
}








