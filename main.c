#include <msp430.h>


#define FLAGS       UCA1IFG
#define RXFLAG      UCRXIFG
#define TXFLAG      UCTXIFG
#define TXBUFFER    UCA1TXBUF
#define RXBUFFER    UCA1RXBUF
#define BUT1 BIT1 // Button S1 at P1.1


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
void Initialize_UART(void);
void Initialize_I2C(void);
void Initialize_ADC_Accel(void);
void Initialize_ADC_joystick(void);
void Initialize_TimerB(void);
void uart_write_char(unsigned char ch);
void uart_write_string(char *str);
void uart_write_hex(unsigned int value);
void uart_write_uint16(unsigned int n);
int  i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int *data);
int  i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data);
unsigned int convert_to_lux_integer(unsigned int raw);
unsigned int convert_to_lux_decimal(unsigned int raw);
void print_direction(unsigned int x, unsigned int y);

// -------------------------------------------------------
// Main
// -------------------------------------------------------
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    unsigned int x_result, y_result;
    unsigned int state = 0;

    P1DIR &= ~(BUT1); // Direct pin as input
    P1REN |= (BUT1); // Enable built-in resistor
    P1OUT |= (BUT1); // Set resistor as pull-up

    Initialize_UART();
    Initialize_ADC_Accel();
    Initialize_TimerB();

    uart_write_string("\033[2J");
    uart_write_string("\033[1;1H");
    uart_write_string("=== Accelerometer ===\r\n");

    // --- Read loop ---
    while (1)
    {
        if ((P1IN & BUT1) == 0) {
            __delay_cycles(200000);
            if ((P1IN & BUT1) == 0) { // confirm still pressed
                if (state == 0) {
                    state = 1;
                    Initialize_ADC_joystick();
                    uart_write_string("\033[2J");    // clear screen
                    uart_write_string("\033[1;1H");  // move cursor to top left
                } else {
                    state = 0;
                    Initialize_ADC_Accel();
                    uart_write_string("\033[2J");    // clear screen
                    uart_write_string("\033[1;1H");  // move cursor to top left
                }

                while ((P1IN & BUT1) == 0);
                __delay_cycles(200000);  // debounce release
            }

        }

        if (state == 0) {
            ADC12CTL0 |= ADC12SC;



            __delay_cycles(50000);    // give ADC time to complete

            TB0CTL &= ~MC_1;

            unsigned int x = XDIROUT;
            unsigned int y = YDIROUT;
            unsigned int z = ZDIROUT;

            uart_write_string("X: ");
            uart_write_uint16(x);
            uart_write_string("\r\n");
            uart_write_string("Y: ");
            uart_write_uint16(y);
            uart_write_string("\r\n");
            uart_write_string("Z: ");
            uart_write_uint16(z);
            uart_write_string("\r\n");

            if (x <= LOW || x >= HIGH) {
                TB0CTL |= TBSSEL_2 | MC_1;   // SMCLK, up mode
            }

            if (y <= LOW || y >= HIGH) {
                TB0CTL |= TBSSEL_2 | MC_1;   // SMCLK, up mode
            }

            ADC12CTL0 &= ~ADC12SC;
            __delay_cycles(800000);
        } else {

            TB0CTL &= ~MC_1;

            // Trigger conversion
            ADC12CTL0 |= ADC12SC;

            // Wait for conversion to complete
            while(ADC12CTL1 & ADC12BUSY) {}

            // Read results
            x_result = ADC12MEM0;
            y_result = ADC12MEM1;

            // Print direction
            print_direction(x_result, y_result);


            if (x_result <= 900 || x_result >= 3100) {
                TB0CTL |= TBSSEL_2 | MC_1;   // SMCLK, up mode
            }

            if (y_result <= 900 || y_result >= 3100) {
                TB0CTL |= TBSSEL_2 | MC_1;   // SMCLK, up mode
            }

            __delay_cycles(50000);
        }
    }
    return 0;
}

void Initialize_TimerB(void) {
    P2SEL1 &= ~BIT7;   // clear SEL1
    P2SEL0 |=  BIT7;   // set SEL0

    P2DIR |= BIT7;

    TB0CCR0  = 1000;        // period  (1000Hz)
    TB0CCR6  = 500;         // duty cycle (50%)
    TB0CCTL6 = OUTMOD_7;    // PWM mode
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

// -------------------------------------------------------
// Initialize eUSCI_A1 as UART at 9600 baud
// Matches your working airport code exactly
// -------------------------------------------------------
void Initialize_UART(void)
{
    P3SEL1 &= ~(BIT4 | BIT5);
    P3SEL0 |=  (BIT4 | BIT5);

    UCA1CTLW0 = UCSWRST;
    UCA1CTLW0 |= UCSSEL_2;                     // SMCLK
    UCA1BRW    = 6;
    UCA1MCTLW  = UCBRF3 | UCBRS5 | UCOS16;
    UCA1CTLW0 &= ~UCSWRST;
}

// -------------------------------------------------------
// Initialize eUSCI_B1 as I2C Master at 125 KHz
// SDA: P4.0    SCL: P4.1
// -------------------------------------------------------
void Initialize_I2C(void)
{
    P4SEL1 |=  (BIT1 | BIT0);
    P4SEL0 &= ~(BIT1 | BIT0);

    UCB1CTLW0  = UCSWRST;
    UCB1CTLW0 |= UCMST | UCMODE_3 | UCSYNC | UCSSEL_3;
    UCB1BRW    = 8;                             // 1 MHz / 8 = 125 KHz
    UCB1CTLW0 &= ~UCSWRST;
}


void uart_write_char(unsigned char ch)
{
    while ((FLAGS & TXFLAG) == 0) {}
    TXBUFFER = ch;
}

void uart_write_string(char *str)
{
    if (*str == '\0')
        return;
    uart_write_char(*str);
    uart_write_string(str + 1);
}

void uart_write_hex(unsigned int value)
{
    const char hex_chars[] = "0123456789ABCDEF";
    int i;
    for (i = 3; i >= 0; i--)
        uart_write_char(hex_chars[(value >> (i * 4)) & 0xF]);
}

void uart_write_uint16(unsigned int n)
{
    if (n == 0)
    {
        uart_write_char('0');
        return;
    }
    char buf[5];
    int i = 0;
    while (n > 0)
    {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    // Print digits in reverse
    int j;
    for (j = i - 1; j >= 0; j--)
        uart_write_char(buf[j]);
}

void print_direction(unsigned int x, unsigned int y) {
    // Clear the line first
    uart_write_string("\033[2K\r");

    // Determine direction based on thresholds
    int left  = (x < THRESHOLD_LOW);
    int right = (x > THRESHOLD_HIGH);
    int up    = (y > THRESHOLD_HIGH);
    int down  = (y < THRESHOLD_LOW);

    // Corner combinations first
    if (left && up)
        uart_write_string("Top-Left");
    else if (right && up)
        uart_write_string("Top-Right");
    else if (left && down)
        uart_write_string("Bottom-Left");
    else if (right && down)
        uart_write_string("Bottom-Right");
    // Single directions
    else if (left)
        uart_write_string("Left");
    else if (right)
        uart_write_string("Right");
    else if (up)
        uart_write_string("Up");
    else if (down)
        uart_write_string("Down");
    else
        uart_write_string("Center");

    // Optionally also print raw values for debugging
    uart_write_string("  (X: ");
    uart_write_uint16(x);
    uart_write_string(", Y: ");
    uart_write_uint16(y);
    uart_write_string(")");

    uart_write_char('\n');
    uart_write_char('\r');
}


int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int *data)
{
    unsigned char byte1 = 0;
    unsigned char byte2 = 0;

    UCB1IFG &= ~(UCRXIFG0 | UCTXIFG0 | UCNACKIFG);

    UCB1I2CSA  = i2c_address;
    UCB1CTLW0 |= UCTR;
    UCB1CTLW0 |= UCTXSTT;

    while (!(UCB1IFG & UCTXIFG0))
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    UCB1TXBUF = i2c_reg;

    while (!(UCB1IFG & UCTXIFG0))
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    UCB1CTLW0 &= ~UCTR;
    UCB1CTLW0 |=  UCTXSTT;

    while (UCB1CTLW0 & UCTXSTT)
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    while (!(UCB1IFG & UCRXIFG0));
    byte1 = UCB1RXBUF;

    UCB1CTLW0 |= UCTXSTP;          // Schedule STOP before reading last byte

    while (!(UCB1IFG & UCRXIFG0));
    byte2 = UCB1RXBUF;

    while (UCB1CTLW0 & UCTXSTP);

    *data = ((unsigned int)byte1 << 8) | byte2;
    return 0;
}

// -------------------------------------------------------
// I2C Write: 2 bytes to internal register
// Returns 0 on success, -1 on NACK/error
// -------------------------------------------------------
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data)
{
    unsigned char byte1 = data >> 8;
    unsigned char byte2 = data & 0xFF;

    UCB1IFG &= ~(UCTXIFG0 | UCNACKIFG);

    UCB1I2CSA  = i2c_address;
    UCB1CTLW0 |= UCTR;
    UCB1CTLW0 |= UCTXSTT;

    while (!(UCB1IFG & UCTXIFG0))
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    UCB1TXBUF = i2c_reg;

    while (!(UCB1IFG & UCTXIFG0))
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    UCB1TXBUF = byte1;

    while (!(UCB1IFG & UCTXIFG0))
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    UCB1TXBUF = byte2;

    while (!(UCB1IFG & UCTXIFG0))
    {
        if (UCB1IFG & UCNACKIFG)
        {
            UCB1CTLW0 |= UCTXSTP;
            while (UCB1CTLW0 & UCTXSTP);
            UCB1IFG &= ~UCNACKIFG;
            return -1;
        }
    }

    UCB1CTLW0 |= UCTXSTP;
    while (UCB1CTLW0 & UCTXSTP);

    return 0;
}
