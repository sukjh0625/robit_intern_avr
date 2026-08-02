#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define BAUD_RATE          9600

#define SERVO_MIN_ANGLE    0
#define SERVO_MAX_ANGLE    180
#define ORIGIN_ANGLE       90    

#define PWM_TOP            39999 
#define SERVO_MIN_TICKS    2000   
#define SERVO_MAX_TICKS    4000   

#define RX_BUF_SIZE        16

void UART0_init(unsigned long baud)
{
    unsigned int ubrr = (F_CPU / 16 / baud) - 1;

    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)(ubrr);

    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART0_transmit(unsigned char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void UART0_print(const char *str)
{
    while (*str)
    {
        UART0_transmit(*str++);
    }
}

unsigned char UART0_receive(void)
{
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void UART0_read_line(char *buf, unsigned char max_len)
{
    unsigned char idx = 0;

    while (idx < max_len - 1)
    {
        unsigned char c = UART0_receive();

        if (c == '\r' || c == '\n')
        {
            UART0_print("\r\n");
            break;
        }

        if ((c == 0x08 || c == 0x7F) && idx > 0)
        {
            idx--;
            UART0_print("\b \b");
            continue;
        }

        buf[idx++] = (char)c;
        UART0_transmit(c);  // echo
    }

    buf[idx] = '\0';
}

void Servo_PWM_init(void)
{
    DDRB |= (1 << PB7);  

    TCCR1A = (1 << COM1C1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); 

    ICR1 = PWM_TOP;
}

void Servo_set_angle(int angle)
{
    unsigned int ticks = SERVO_MIN_TICKS +
        (unsigned long)(SERVO_MAX_TICKS - SERVO_MIN_TICKS) * angle / (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE);

    OCR1C = ticks;
}
int main(void)
{
    char rx_buf[RX_BUF_SIZE];

    UART0_init(BAUD_RATE);
    Servo_PWM_init();

    Servo_set_angle(ORIGIN_ANGLE);
    _delay_ms(500);  

    UART0_print(" Servo Control Ready (origin: 90 deg) ===\r\n");
    UART0_print("Enter target angle and press Enter:\r\n");

    while (1)
    {
        UART0_print("> ");
        UART0_read_line(rx_buf, RX_BUF_SIZE);

        if (rx_buf[0] == '\0')
        {
            continue;  
        }

        int angle = atoi(rx_buf);

        if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE)
        {
            UART0_print("[WARNING] Angle out of range (0~180). Servo not moved.\r\n");
            continue;
        }

        Servo_set_angle(angle);

        char msg[48];
        sprintf(msg, "Moved to %d degrees.\r\n", angle);
        UART0_print(msg);
    }

    return 0;
}
