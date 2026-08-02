#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define TX_BIT PD3
// PD3으로 데이터를 보낼 핀 설정
#define BIT_TICKS 208
//9600 통신에서 비트하나를 유지해야하는 시간이 104인데 만들기 위해 설정한 값

void wait_bit(void) // 비트 하나를 보내는 시간만큼 기다리는 함수이다
{
    TCNT1 = 0;

    while (TCNT1 < BIT_TICKS)
    {}
    // 타이머 값이 208이 될 때까지 기다린다
}

void send_byte(uint8_t data)
// 8비트 데이터를 보내는 함수
{
    uint8_t a;
    // 비트 번호를 확인할 반복 변수

    PORTD &= ~(1 << TX_BIT);
    // 시작 비트는 LOW 상태로 보낸다

    wait_bit();

    for (a = 0; a < 8; a++)
    // 낮은 비트부터 8개 비트를 하나씩 보낸다
    {
        if (data & (1 << a))
        {
            PORTD |= (1 << TX_BIT); // 비트가 1이면 HIGH 전송
        }
        else
        {
            PORTD &= ~(1 << TX_BIT); // 비트가 0이면 LOW 전송
        }

        wait_bit();
    }

    PORTD |= (1 << TX_BIT);//정지비트 high 상태

    wait_bit();
}

int main(void)
{
    cli();// 통신 중 시간 오차가 생기지 않도록 인터럽트 끄게하기

    DDRD |= (1 << TX_BIT);

    PORTD |= (1 << TX_BIT); // 통신하지 않을 때 HIGH 상태

    TCCR1A = 0x00;

    TCCR1B = (1 << CS11);
    // Timer1을 더느리게 설

    while (1)
    {
        send_byte(0x48);  // H
        send_byte(0x65);  // e
        send_byte(0x6C);  // l
        send_byte(0x6C);  // l
        send_byte(0x6F);  // o
        send_byte(0x57);  // W
        send_byte(0x6F);  // o
        send_byte(0x72);  // r
        send_byte(0x6C);  // l
        send_byte(0x64);  // d
        send_byte(0x21);  // !

        send_byte(0x0D);

        send_byte(0x0A);

        for (uint16_t a = 0; a < 1000; a++)
        {
            wait_bit();
        }
        // HelloWorld!를 보낸 뒤 약 0.1초 기다린다
    }
}
