/*
 * day 1.c
 *
 * Created: 2026-07-30 오후 4:34:09
 * Author : sukjh
 */ 

#include <avr/io.h>//porta ddra pinc
#include <util/delay.h>//delay
#include <avr/interrupt.h>// 인터럽트

int count = 0;//led 상태  선언

int main(void)
{
	DDRA = 0xFF;//모든핀 출력
	PORTA = 0xFF;//모든 핀 출력

	DDRC = 0x00;//모든 핀 입력
	PORTC = (1 << PC0) | (1 << PC1);// 내부 풀업 저항 키기

	DDRD &= ~((1 << PD2) | (1 << PD3));//입력 설정
	PORTD |= (1 << PD2) | (1 << PD3);// 내부 풀업저항

	EIMSK = (1 << INT2) | (1 << INT3);//외부인터럽트 INT2 INT3 tkdyd
	EICRA = (1 << ISC21) | (1 << ISC31);// INT2와 INT3을 HIGH에서 LOW로 가도록 설정

	sei();// 인터럽트 허용

	while (1)
	{
		if (!(PINC & (1 << PC0)) && !(PINC & (1 << PC1)))//1번 2번 눌렸는지 확인
		{
			PORTA = 0x00;//LED전체 키기
		}
		else if (!(PINC & (1 << PC0)))
		{
			PORTA = 0x0F;//1번만 눌렸다면 4개 키기
		}
		else if (!(PINC & (1 << PC1)))
		{
			PORTA = 0xF0;//2번 눌렷다면 3래 키기
		}
		else
		{
			PORTA = count ? 0xFF : 0x00;
			count = !count;//껏다 켰다

			_delay_ms(500);// 아무것도 안눌렀다면 0.5초씩 깜빡거리기
		}
	}
}

ISR(INT2_vect)
{
	unsigned char led;

	for (led = 0x01; led != 0; led <<= 1)// LED한칸씩 왼쪽 이동
	{
		PORTA = ~led;//LED하나만 키기
		_delay_ms(500);
	}
}

ISR(INT3_vect)
{
	unsigned char led;

	for (led = 0x80; led != 0; led >>= 1)//LED한칸씩 오른쪽 이동
	{
		PORTA = ~led;
		_delay_ms(500);
	}
}
