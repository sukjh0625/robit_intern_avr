#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#define LCD_ADDRESS 0x27
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_LIGHT 0x08

#define SW1 PC0
#define SW2 PC1

volatile int year = 2026;
volatile char month = 1;
volatile char day = 1;
volatile char hour = 0;
volatile char minute = 0;
volatile char sec = 0;
volatile char csec = 0;
volatile char msCnt = 0;
volatile char run = 0;

char lastDay(int y, char m)//예전과제 윤년처럼 알짜 넣어놓
{
	char d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
	{
		return 29;
	}

	return d[m - 1];
}

void twiInit(void)//i2c통신 설정
{
	TWSR = 0x00;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

void twiStart(void)//시작
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{
	}
}

void twiWrite(uint8_t data)//전송
{
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{
	}
}

void twiStop(void)// 정지
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

void lcdExpander(uint8_t data)//lcd에 데이터 보내는 부분
{
	twiStart();
	twiWrite(LCD_ADDRESS << 1);//전송형 주소로 바꾸기
	twiWrite(data | LCD_LIGHT);
	twiStop();
}

void lcdPulse(uint8_t data)
{
	lcdExpander(data | LCD_EN);
	_delay_us(1);

	lcdExpander(data & ~LCD_EN);
	_delay_us(50);
}

void lcdNibble(uint8_t data)
{
	lcdExpander(data);
	lcdPulse(data);
}

void lcdSend(uint8_t data, uint8_t mode)
{
	lcdNibble((data & 0xF0) | mode);
	lcdNibble(((data << 4) & 0xF0) | mode);
}

void lcdCommand(uint8_t command)
{
	lcdSend(command, 0);
}

void lcdData(uint8_t data)
{
	lcdSend(data, LCD_RS);
}

void lcdInit(void)//lcd초기화
{
	_delay_ms(50);

	lcdNibble(0x30);
	_delay_ms(5);

	lcdNibble(0x30);
	_delay_us(150);

	lcdNibble(0x30);
	lcdNibble(0x20);

	lcdCommand(0x28);
	lcdCommand(0x0C);
	lcdCommand(0x06);
	lcdCommand(0x01);

	_delay_ms(2);
}

void lcdClear(void)
{
	lcdCommand(0x01);
	_delay_ms(2);
}

void lcdPosition(char row, char column)
{
	if (row == 0)
	{
		lcdCommand(0x80 + column);
	}
	else
	{
		lcdCommand(0xC0 + column);
	}
}

void lcdString(char row, char column, char *text)// 문자열 출력
{
	lcdPosition(row, column);

	while (*text != '\0')
	{
		lcdData(*text);
		text++;
	}
}

ISR(TIMER0_OVF_vect)// 인터럽트로 시간 증
{
	TCNT0 = 6;

	if (!run)
	{
		return;
	}

	msCnt++;

	if (msCnt < 10)
	{
		return;
	}

	msCnt = 0;
	csec++;

	if (csec < 100)
	{
		return;
	}

	csec = 0;
	sec++;

	if (sec < 60)
	{
		return;
	}

	sec = 0;
	minute++;

	if (minute < 60)
	{
		return;
	}

	minute = 0;
	hour++;

	if (hour < 24)
	{
		return;
	}

	hour = 0;
	day++;

	if (day <= lastDay(year, month))
	{
		return;
	}

	day = 1;
	month++;

	if (month <= 12)
	{
		return;
	}

	month = 1;
	year++;
}

void timer0Init(void)
{
	TCCR0 = (1 << CS01) | (1 << CS00);
	TCNT0 = 6;
	TIMSK |= (1 << TOIE0);
}

void adcInit(void)//adc초기화// 이부분이 없어서 인식을 못했음
{
	ADMUX = (1 << REFS0);

	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);
}

int adcRead(void)// 가변저항 읽기
{
	ADCSRA |= (1 << ADSC);

	while (ADCSRA & (1 << ADSC))
	{
	}

	return ADCW;
}

char swPushed(char bit)
{
	if (PINC & (1 << bit))
	{
		return 0;
	}

	_delay_ms(20);

	if (PINC & (1 << bit))
	{
		return 0;
	}

	while (!(PINC & (1 << bit)))
	{
	}

	_delay_ms(20);

	return 1;
}

int setValue(char *name, int lo, int hi)// 가변저항으로 시간 설정
{
	int v;
	char buf[20];

	lcdClear();

	while (1)
	{
		v = lo + (long)adcRead() * (hi - lo + 1) / 1024;

		if (v > hi)
		{
			v = hi;
		}

		sprintf(buf, "%s INPUT      ", name);
		lcdString(0, 0, buf);

		sprintf(buf, "VALUE: %d      ", v);
		lcdString(1, 0, buf);

		if (swPushed(SW1))
		{
			return v;
		}
	}
}

int main(void)
{
	int y;
	char mo, d, h, mi, s, c;
	char prevDay = 0;
	char buf[20];

	DDRC &= ~((1 << SW1) | (1 << SW2));
	PORTC |= (1 << SW1) | (1 << SW2);

	DDRF &= ~(1 << PF0);
	PORTF &= ~(1 << PF0);

	adcInit();
	twiInit();
	lcdInit();
	timer0Init();

	sei();

	lcdClear();
	lcdString(0, 0, "SW1 TO FIX");
	lcdString(1, 0, "SET THE CLOCK");
	_delay_ms(1000);

	year = 2000 + setValue("YEAR", 0, 99);
	month = setValue("MONTH", 1, 12);
	day = setValue("DAY", 1, lastDay(year, month));
	hour = setValue("HOUR", 0, 23);
	minute = setValue("MIN", 0, 59);
	sec = setValue("SEC", 0, 59);

	csec = 0;

	lcdClear();
	lcdString(0, 0, "PUSH SW2");
	lcdString(1, 0, "TO START");

	while (!swPushed(SW2))
	{
	}

	lcdClear();
	run = 1;

	while (1)
	{
		cli();

		y = year;
		mo = month;
		d = day;
		h = hour;
		mi = minute;
		s = sec;
		c = csec;

		sei();

		if (d != prevDay)
		{
			sprintf(buf, "%02d%02d%02d", y % 100, mo, d);
			lcdString(0, 0, buf);
			prevDay = d;
		}

		sprintf(buf, "%02d:%02d:%02d.%02d", h, mi, s, c);
		lcdString(1, 0, buf);
	}
}
