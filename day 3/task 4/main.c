#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define TX_BIT PD3 
//pd3으로 데이터 보낼 핀 설정
#define BIT_TICKS 104
//비트시간 설정
void wait_bit(void)//비트 하나를 보내는 시간만큼 기다리는 함수이다
{
    TCNT1 = 0;

    while (TCNT1 < BIT_TICKS)
    {}
}

void send_byte(uint8_t data)//8비트를 보내는 함수
{
    uint8_t a;

    PORTD &= ~(1 << TX_BIT);//tx핀이 하이 high 상태// 시작 후 low 상태로 유지
    wait_bit();

    for (a = 0; a < 8; a++)//낮은 비트부터 보
    {
        if (data & (1 << a))
        {
            PORTD |= (1 << TX_BIT);
        }
        else
        {
            PORTD &= ~(1 << TX_BIT);
        }

        wait_bit();
    }

    PORTD |= (1 << TX_BIT);//8비트 다보낸 뒤에 정지비트 보내기 정지 비트는 high이다
    wait_bit();
}

int main(void)
{
    cli();//인터럽트 끄는함수

    DDRD |= (1 << TX_BIT);//입출력 설정
    PORTD |= (1 << TX_BIT);

    TCCR1A = 0x00;//일반 터이머 방식으로 사용
    TCCR1B = (1 << CS11);

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

        send_byte(0x0D);// 줄바
        send_byte(0x0A);

        for (uint16_t a = 0; a < 1000; a++)
        {
            wait_bit();
        }
    }
}
