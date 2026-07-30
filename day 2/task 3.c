#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define LCD_ADDRESS 0x27
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_LIGHT 0x08

void twi_init(void);
void twi_start(void);
void twi_write(uint8_t data);
void twi_stop(void);

void lcd_expander(uint8_t data);
void lcd_pulse(uint8_t data);
void lcd_nibble(uint8_t data);
void lcd_send(uint8_t data, uint8_t mode);
void lcd_command(uint8_t command);
void lcd_data(uint8_t data);
void lcd_init(void);
void lcd_position(uint8_t row, uint8_t column);
void lcd_string(const char *text);
void lcd_number(uint16_t number, uint8_t digits);
void lcd_clear_line(uint8_t row);

void show_screen(
uint16_t a,// 계산할 숫자 받기
uint16_t b,
uint8_t operatorNumber,//연산자 번호
uint8_t calculated//출력할지 결정하는 값
);

int main(void)
{
	uint16_t a = 1;//a의 값 저장하는 변수
	uint16_t b = 1;
	uint8_t operatorNumber = 0;
	uint8_t calculated = 0;

	DDRC &= ~((1 << PC0) | (1 << PC1));//스위치 입력설정
	PORTC |= (1 << PC0) | (1 << PC1);

	DDRD &= ~((1 << PD0) | (1 << PD1) |
	(1 << PD2) | (1 << PD3));

	PORTD |= (1 << PD0) | (1 << PD1) |
	(1 << PD2) | (1 << PD3);

	twi_init();
	lcd_init();

	lcd_position(0, 0);
	lcd_string("SJH");

	show_screen(a, b, operatorNumber, calculated);

	while (1)
	{
		if (!(PINC & (1 << PC0)))//스위치 눌림여부
		{
			_delay_ms(20);

			if (!(PINC & (1 << PC0)))
			{
				a++;

				if (a > 999)//999보다 크면 다시 1로 초기화
				{
					a = 1;
				}

				calculated = 0;//계싼결과 지우기 그리고 다시 채우기
				show_screen(a, b, operatorNumber, calculated);

				while (!(PINC & (1 << PC0)))
				{}
			}
		}

		if (!(PINC & (1 << PC1)))
		{
			_delay_ms(20);

			if (!(PINC & (1 << PC1)))
			{
				operatorNumber++;

				if (operatorNumber > 3)
				{
					operatorNumber = 0;//연산자도 5번쨰부터 다시 플러스로
				}

				calculated = 0;
				show_screen(a, b, operatorNumber, calculated);

				while (!(PINC & (1 << PC1)))//스위치 놓기
				{}
			}
		}

		if (!(PIND & (1 << PD2)))
		{
			_delay_ms(20);

			if (!(PIND & (1 << PD2)))
			{
				b++;

				if (b > 999)
				{
					b = 1;
				}

				calculated = 0;
				show_screen(a, b, operatorNumber, calculated);

				while (!(PIND & (1 << PD2)))
				{}
			}
		}

		if (!(PIND & (1 << PD3)))
		{
			_delay_ms(20);

			if (!(PIND & (1 << PD3)))
			{
				calculated = 1;
				show_screen(a, b, operatorNumber, calculated);

				while (!(PIND & (1 << PD3)))
				{}
			}
		}
	}
}

void show_screen(
uint16_t a,
uint16_t b,
uint8_t operatorNumber,
uint8_t calculated
)
{//변수저장
	int32_t result = 0;
	char operatorCharacter;

	if (operatorNumber == 0)//연산자 정의
	{
		operatorCharacter = '+';
		result = (int32_t)a + b;
	}
	else if (operatorNumber == 1)
	{
		operatorCharacter = '-';
		result = (int32_t)a - b;
	}
	else if (operatorNumber == 2)
	{
		operatorCharacter = '*';
		result = (int32_t)a * b;
	}
	else
	{
		operatorCharacter = '/';

		if (b != 0)
		{
			result = a / b;
		}
	}

	lcd_clear_line(1);
	lcd_position(1, 0);

	lcd_number(a, 3);
	lcd_data(' ');
	lcd_data(operatorCharacter);
	lcd_data(' ');
	lcd_number(b, 3);
	lcd_string(" = ");

	if (calculated == 0)//계산전 결과 숨기기
	{
		lcd_string("    ");
	}
	else if (operatorNumber == 3 && b == 0)//0으로 나누며 에러
	{
		lcd_string("ERR ");
	}
	else if (result > 9999 || result < -999)
	{
		lcd_string("OVER");//예외처리
	}
	else if (result < 0)//음수인경우
	{
		lcd_data('-');
		lcd_number((uint16_t)(-result), 3);
	}
	else
	{
		lcd_number((uint16_t)result, 4);
	}
}

void lcd_number(uint16_t number, uint8_t digits)//자릿수 맞춰서 출력하기
{
	uint16_t divisor = 1;
	uint8_t i;

	for (i = 1; i < digits; i++)
	{
		divisor *= 10;
	}

	for (i = 0; i < digits; i++)
	{
		lcd_data((number / divisor) + '0');
		number %= divisor;
		divisor /= 10;
	}
}

void lcd_clear_line(uint8_t row)
{
	uint8_t i;

	lcd_position(row, 0);

	for (i = 0; i < 16; i++)
	{
		lcd_data(' ');
	}

	lcd_position(row, 0);
}

void twi_init(void)
{
	TWSR = 0x00;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

void twi_start(void)
{
	TWCR = (1 << TWINT) |
	(1 << TWSTA) |
	(1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{
	}

	TWDR = LCD_ADDRESS << 1;

	TWCR = (1 << TWINT) |
	(1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{
	}
}

void twi_write(uint8_t data)
{
	TWDR = data;

	TWCR = (1 << TWINT) |
	(1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{
	}
}

void twi_stop(void)
{
	TWCR = (1 << TWINT) |
	(1 << TWSTO) |
	(1 << TWEN);

	_delay_us(10);
}

void lcd_expander(uint8_t data)
{
	twi_start();
	twi_write(data | LCD_LIGHT);
	twi_stop();
}

void lcd_pulse(uint8_t data)
{
	lcd_expander(data | LCD_EN);
	_delay_us(1);

	lcd_expander(data & ~LCD_EN);
	_delay_us(100);
}

void lcd_nibble(uint8_t data)
{
	lcd_expander(data);
	lcd_pulse(data);
}

void lcd_send(uint8_t data, uint8_t mode)
{
	uint8_t highData;
	uint8_t lowData;

	highData = data & 0xF0;
	lowData = (data << 4) & 0xF0;

	if (mode == 1)
	{
		highData |= LCD_RS;
		lowData |= LCD_RS;
	}

	lcd_nibble(highData);
	lcd_nibble(lowData);
}

void lcd_command(uint8_t command)
{
	lcd_send(command, 0);

	if (command == 0x01 || command == 0x02)
	{
		_delay_ms(2);
	}
}

void lcd_data(uint8_t data)
{
	lcd_send(data, 1);
}

void lcd_init(void)
{
	_delay_ms(100);

	lcd_expander(0x00);
	_delay_ms(10);

	lcd_nibble(0x30);
	_delay_ms(5);

	lcd_nibble(0x30);
	_delay_us(200);

	lcd_nibble(0x30);
	_delay_us(200);

	lcd_nibble(0x20);
	_delay_us(200);

	lcd_command(0x28);
	lcd_command(0x08);
	lcd_command(0x01);
	lcd_command(0x06);
	lcd_command(0x0C);

	_delay_ms(5);
}

void lcd_position(uint8_t row, uint8_t column)
{
	if (row == 0)
	{
		lcd_command(0x80 + column);
	}
	else
	{
		lcd_command(0xC0 + column);
	}
}

void lcd_string(const char *text)
{
	while (*text)
	{
		lcd_data(*text++);
	}
}
