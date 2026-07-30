/*
 * day 1.c
 *
 * Author : sukjh
 */ 
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

volatile unsigned char count = 0;//volatile 인터럽트에서도 값이 바뀔 수 있다는 뜻
volatile unsigned char int2 = 0;// 인터럽트 일어났는지 확인하는 변수

int main(void)
{
	unsigned char led;
	unsigned char repeat;

	DDRA = 0xFF;// 모두 출력
	PORTA = 0xFF;// 모두 1출력

	DDRC &= ~((1 << PC0) | (1 << PC1));// 입력설정
	PORTC |= (1 << PC0) | (1 << PC1);// 풀업 저항

	DDRD &= ~((1 << PD2) | (1 << PD3));
	PORTD |= (1 << PD2) | (1 << PD3);

	EIMSK = (1 << INT2) | (1 << INT3);//외부 인터럽트 허용
	EICRA = (1 << ISC21) | (1 << ISC31);// 하이에서 로우로 인터럽트 조건 만들기

	sei();

	while (1)
	{
		if (!(PINC & (1 << PC0)))//스위치 눌렸는지 확인
		{
				for (repeat = 0; repeat < 2; repeat++)// 동작 두번 반복
				{
					for (led = 0x07; led <= 0xE0; led <<= 1)// 왼쪽방향으로 3개 이동
					{
						PORTA = ~led;//LED  반전해서 출력 LOW여서 반전해야함
						_delay_ms(200);

						if (led == 0xE0)// 만약 마지막 위치에 도착했다면 끝내기
						{
							break;
						}
					}
				}

				while (!(PINC & (1 << PC0))){}// 스위치 누르는 동안 기다리기
			}
		}
		else if (!(PINC & (1 << PC1)))
		{
			for (repeat = 0; repeat < 2; repeat++)
				{
					for (led = 0xE0; led >= 0x07; led >>= 1)
					{
						PORTA = ~led;
						_delay_ms(200);

						if (led == 0x07)
						{
							break;
						}
					}
				}

				while (!(PINC & (1 << PC1))){}
			}
		}
		else if (int2 == 1)// 인터럽트 발생했다면 INT2 1되기
		{
			int2 = 0;

			for (led = 0x01; led != 0; led <<= 1)// 오른쪽
			{
				PORTA = ~led;
				_delay_ms(200);
			}

			for (led = 0x40; led != 0; led >>= 1)
			{
				PORTA = ~led;
				_delay_ms(200);
			}
		}
		else
		{
			PORTA = ~count;// 반전해서  출력
			count++;

			_delay_ms(100);
		}
	}
}

ISR(INT2_vect)// 인터럽트 발생했다면 1
{
	int2 = 1;
}

ISR(INT3_vect)// 인터럽트 발생했다면 
{
	count = 0;// LED상태 값 초기화
	PORTA = 0xFF;// 끄기
}
