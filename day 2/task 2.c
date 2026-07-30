#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>// 시간 지연 
#include <stdint.h>//크기가 정해진 자료형 사용 예를 들어 부허없는 8비트 16비트

#define LCD_ADDRESS 0x27
#define LCD_RS 0x01//레지스터 선택하는 비트
#define LCD_EN 0x04//enable 즉 데이터 저장하는거 가능하게 
#define LCD_LIGHT 0x08// 백라이트

void twi_init(void);//12c 초기화
void twi_start(void);// 12c 통신
void twi_write(uint8_t data);// 12c로 보내는 함수
void twi_stop(void);// 12c 통신

void lcd_expander(uint8_t data);//데이터 보냄
void lcd_pulse(uint8_t data);//신호 보냄
void lcd_nibble(uint8_t data);//lcd는 4비트 써서 4비트 보냄
void lcd_send(uint8_t data, uint8_t mode);
void lcd_command(uint8_t command);
void lcd_data(uint8_t data);
void lcd_init(void);//초기화 
void lcd_position(uint8_t row, uint8_t column);//lcd원하는 위치로 이동
void lcd_string(const char* text);//문자열 출력
void lcd_number(uint16_t number, uint8_t digits);//원하는 자리수로 출력

void adc_init(void);
uint16_t adc_read(void);

int main(void)
{
	uint16_t adcValue;//읽은 값 저장
	uint16_t voltage;//전압
	uint8_t ledPosition;//led 번호

	DDRA = 0xFF;
	PORTA = 0xFF;

	DDRF &= ~(1 << PF0);//pfo을 입력으로 설정 가변저항

	DDRD &= ~((1 << PD0) | (1 << PD1));
	PORTD |= (1 << PD0) | (1 << PD1);

	twi_init();// 초기화
	lcd_init();
	adc_init();

	lcd_position(0, 0);//lcd 커서 0.0으로 옮기기
	lcd_string("SJH");

	lcd_position(1, 0);
	lcd_string("0000");

	lcd_position(1, 11);
	lcd_string("0.0V");

	while (1)
	{
		adcValue = adc_read();// 가변저항 읽기

		ledPosition = adcValue / 128;// led번호로 변환

		if (ledPosition > 7)// led번호가 7보다 커도 7까지 밖에 없어서 7로 제한
		{
			ledPosition = 7;
		}

		PORTA = ~(1 << ledPosition);// 맞는 led하나만 키기

		voltage = ((uint32_t)adcValue * 5000UL) / 1023UL;//adc값 전압으로 계산 

		lcd_position(1, 0);// 커서이동
		lcd_number(adcValue, 4);

		lcd_position(1, 11);
		lcd_data((voltage / 1000) + '0');// 전압 정수 부분 출력
		lcd_data('.');
		lcd_data(((voltage % 1000) / 100) + '0');
		lcd_data('V');

		_delay_ms(100);// 너무 빠르게 바뀌지 않게
	}
}

void adc_init(void)
{
	ADMUX = (1 << REFS0);// 기준 전력을 5v로

	ADCSRA =
	(1 << ADEN) |//키기
	(1 << ADPS2) |//1번째 비트 1
	(1 << ADPS1) |//2번째 비트 1
	(1 << ADPS0);// 1
}

uint16_t adc_read(void)//adc값 읽기
{
	ADMUX = (ADMUX & 0xE0) | 0x00;// 입력 채널 선택

	ADCSRA |= (1 << ADSC);//adc변환

	while (ADCSRA & (1 << ADSC)){}// 변환 끝날때까지 기다리기

	return ADC;// 레지스터 값 반환
}

void twi_init(void)//초기화
{
	TWSR = 0x00;//레지스터 초기화
	TWBR = 72;//통신속도
	TWCR = (1 << TWEN);
}

void twi_start(void)
{
	TWCR =
	(1 << TWINT) |//동작 완료
	(1 << TWSTA) |
	(1 << TWEN);

	while (!(TWCR & (1 << TWINT))){}//스타트 신호가 전송될떄까지 기다리기

	TWDR = LCD_ADDRESS << 1;//주소 데에터 레지스터에 넣기

	TWCR =
	(1 << TWINT) |//주소 전송 시작
	(1 << TWEN);

	while (!(TWCR & (1 << TWINT))){}//기달리기
}

void twi_write(uint8_t data)//lcd로 데이터 보내기
{
	TWDR = data;

	TWCR =
	(1 << TWINT) |
	(1 << TWEN);

	while (!(TWCR & (1 << TWINT))){}
}

void twi_stop(void)// 통신 종료
{
	TWCR =
	(1 << TWINT) |
	(1 << TWSTO) |
	(1 << TWEN);

	_delay_us(10);
}

void lcd_expander(uint8_t data)// lcd의 확장기로 데이터 보내는 함수
{
	twi_start();
	twi_write(data | LCD_LIGHT);//lcd로 데이터 전송
	twi_stop();
}

void lcd_pulse(uint8_t data)//신호를 한번 발생시키는 함수
	{lcd_expander(data | LCD_EN);
	_delay_us(1);

	lcd_expander(data & ~LCD_EN);
	_delay_us(100);
}

void lcd_nibble(uint8_t data)//4비트를 보내는 함수
{
	lcd_expander(data);
	lcd_pulse(data);
}

void lcd_send(uint8_t data, uint8_t mode)// 문자나 명령을 보내는 함수
{
	uint8_t highData;// 상위 4비트 저장 변수이고
	uint8_t lowData;//하위 4비트 저장 변수이다

	highData = data & 0xF0;
	lowData = (data << 4) & 0xF0;

	if (mode == 1)// 문자 보냈는지 확인
	{
		highData |= LCD_RS;
		lowData |= LCD_RS;
	}

	lcd_nibble(highData);
	lcd_nibble(lowData);
}

void lcd_command(uint8_t command)//명령 보내는 함수
{
	lcd_send(command, 0);

	if (command == 0x01 || command == 0x02)// 명령어 확인
	{
		_delay_ms(2);
	}
}

void lcd_data(uint8_t data)// 출력
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

void lcd_position(uint8_t row, uint8_t column)// 커서 이동
{
	if (row == 0)
	{
		lcd_command(0x80 + column);//원하는 열록
	}
	else
	{
		lcd_command(0xC0 + column);//2행 원하는 열로
	}
}

void lcd_string(const char* text)//lcd출력
{
	while (*text != '\0')
	{
		lcd_data(*text);
		text++;
	}
}

void lcd_number(uint16_t number, uint8_t digits)
{
	uint16_t divider = 1;
	uint8_t count;

	for (count = 1; count < digits; count++)// 1432라면 1X1000 4X100 3X10이런 느낌
	{
		divider *= 10;
	}

	for (count = 0; count < digits; count++)
	{
		lcd_data((number / divider) + '0');
		number %= divider;
		divider /= 10;
	}
}
