#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BAUD_RATE          9600
#define MEASURE_PERIOD_MS  200      
#define PSD_ADC_CHANNEL    1       

#define DIST_COEF_A        2670.4f
#define DIST_COEF_B        -0.769f

#define DIST_MIN_CM        15.0f
#define DIST_MAX_CM        60.0f
#define ADC_MIN_VALID      100
#define ADC_MAX_VALID      900

#define FILTER_WINDOW_SIZE 8

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

void ADC_init(void)
{
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

unsigned int ADC_read(unsigned char channel)
{
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));

    return ADC;
}

unsigned int filter_buf[FILTER_WINDOW_SIZE];
unsigned char filter_index = 0;
unsigned char filter_filled = 0;   
unsigned long filter_sum = 0;

unsigned int filter_update(unsigned int new_sample)
{
    filter_sum -= filter_buf[filter_index];
    filter_buf[filter_index] = new_sample;
    filter_sum += new_sample;

    filter_index++;
    if (filter_index >= FILTER_WINDOW_SIZE)
    {
        filter_index = 0;
        filter_filled = 1;
    }

    unsigned char count = filter_filled ? FILTER_WINDOW_SIZE : filter_index;
    if (count == 0) count = 1;  

    return (unsigned int)(filter_sum / count);
}

float ADC_to_distance_cm(unsigned int adc_value)
{
    if (adc_value < ADC_MIN_VALID || adc_value > ADC_MAX_VALID)
    {
        return -1.0f;
    }

    float distance = DIST_COEF_A * powf((float)adc_value, DIST_COEF_B);

    if (distance < DIST_MIN_CM || distance > DIST_MAX_CM)
    {
        return -1.0f;
    }

    return distance;
}

int main(void)
{
    char buf[80];
    unsigned int raw_val;
    unsigned int filtered_val;
    float distance_cm;

    UART0_init(BAUD_RATE);
    ADC_init();

    for (unsigned char i = 0; i < FILTER_WINDOW_SIZE; i++)
    {
        filter_buf[i] = 0;
    }

    UART0_print("=== PSD Raw + Filtered Measurement Start ===\r\n");

    while (1)
    {
        raw_val      = ADC_read(PSD_ADC_CHANNEL);
        filtered_val = filter_update(raw_val);

        distance_cm = ADC_to_distance_cm(filtered_val);

        if (distance_cm < 0.0f)
        {
            sprintf(buf, "RAW: %u | FILTERED: %u | DISTANCE: [ERROR]\r\n",
                    raw_val, filtered_val);
        }
        else
        {
            int whole = (int)distance_cm;
            int frac  = (int)((distance_cm - whole) * 10.0f);
            if (frac < 0) frac = -frac;

            sprintf(buf, "RAW: %u | FILTERED: %u | DISTANCE: %d.%dcm\r\n",
                    raw_val, filtered_val, whole, frac);
        }

        UART0_print(buf);

        _delay_ms(MEASURE_PERIOD_MS);
    }

    return 0;
}
