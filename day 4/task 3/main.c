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
//adc값을 실제 거리로 바꾸기 위함
#define DIST_COEF_B        -0.769f

#define DIST_MIN_CM        15.0f   
#define DIST_MAX_CM        60.0f   
#define ADC_MIN_VALID      100      
#define ADC_MAX_VALID      900     

void UART0_init(unsigned long baud)//uart초기화
{
    unsigned int ubrr = (F_CPU / 16 / baud) - 1;

    UBRR0H = (unsigned char)(ubrr >> 8);//9600으로 계싼
    UBRR0L = (unsigned char)(ubrr);

    UCSR0B = (1 << RXEN0) | (1 << TXEN0);         
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);       
}

void UART0_transmit(unsigned char data)//문자열보내기
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

unsigned int ADC_read(unsigned char channel)//기준전압 설정 유지하면서 adc채녈만 바꾸기
{
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);  

    ADCSRA |= (1 << ADSC);                     
    while (ADCSRA & (1 << ADSC));            

    return ADC;                                 
}

float ADC_to_distance_cm(unsigned int adc_value)//adc값 거리로 바꾸기
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
    char buf[64];
    unsigned int adc_val;
    float distance_cm;

    UART0_init(BAUD_RATE);
    ADC_init();

    UART0_print("PSD Distance Measurement Start  \r\n");

    while (1)
    {
        adc_val = ADC_read(PSD_ADC_CHANNEL);
        distance_cm = ADC_to_distance_cm(adc_val);

        if (distance_cm < 0.0f)
        {
            sprintf(buf, " [ERROR] Invalid PSD reading\r\n", adc_val);// 예외처리
        }
        else
        {
            int whole = (int)distance_cm;
            int frac  = (int)((distance_cm - whole) * 10.0f);
            if (frac < 0) frac = -frac;

            sprintf(buf, "ADC:%4u  -> Distance: %d.%d cm\r\n", adc_val, whole, frac);
        }

        UART0_print(buf);

        _delay_ms(MEASURE_PERIOD_MS);
    }

    return 0;
}
