# 4일차 과제 : I2C LCD 기반 디지털 시계

> **광운대학교 로봇학부**  
> **작성자:** 석주형  
> **제출일:** 2026년 8월 3일

---

## 1. 개요 (Overview)

본 과제는 ATmega128의 Timer0 오버플로우 인터럽트로 시간을 증가시키고, 가변저항의 ADC 값을 이용해 초기 날짜와 시간을 설정한 뒤 I2C LCD에 표시하는 디지털 시계 구현을 목표로 함. SW1으로 설정값을 확정하고 SW2로 시계를 시작하도록 구성함. 또한 월별 마지막 날짜와 윤년을 처리해 달력 날짜가 정상적으로 넘어가도록 구현함.

### 핵심 목표

* Timer0 오버플로우 인터럽트를 이용한 0.01초 단위 시간 생성
* PF0(ADC0)에 연결한 가변저항으로 날짜 및 시간 설정
* I2C LCD(주소 `0x27`)에 날짜와 시간 표시
* SW1으로 설정값 확정, SW2로 시계 시작
* 윤년 및 월별 마지막 날짜를 반영한 날짜 증가 처리

---

## 2. 개발 환경 (Environment)

| 항목 | 내용 |
| :--- | :--- |
| **MCU** | ATmega128 (16MHz External Crystal) |
| **IDE / Compiler** | Microchip Studio / AVR-GCC |
| **Programmer** | STK500 계열 프로그래머 |
| **언어** | C Language |
| **LCD** | 16×2 I2C LCD, PCF8574 Backpack, Address `0x27` |
| **입력 부품** | 가변저항 1개, 택트 스위치 2개 |

---

## 3. 하드웨어 구성 및 핀 맵 (Hardware Structure)

### Pin Configuration

```text
[ATmega128]                 [Target Component]
 PD0 (SCL)          <---->   I2C LCD SCL
 PD1 (SDA)          <---->   I2C LCD SDA
 PF0 (ADC0)         <-----   Potentiometer output
 PC0                <-----   SW1: Confirm setting
 PC1                <-----   SW2: Start clock
 5V / GND           <---->   LCD, potentiometer, switches
```

### LCD와 스위치 구성

LCD는 I2C 방식으로 연결하여 PD0(SCL), PD1(SDA) 두 개의 신호선만 사용함. LCD 백팩의 I2C 주소는 `0x27`이며, 코드에서 `LCD_LIGHT` 비트를 함께 전송하여 백라이트가 켜진 상태를 유지함.

SW1과 SW2는 PC0, PC1에 연결하고 내부 풀업 저항을 사용함. 따라서 평소에는 `1`이고 스위치를 누르면 GND에 연결되어 `0`이 입력되는 Active-Low 방식임. PF0은 가변저항의 가운데 단자와 연결되어 ADC0 입력으로 사용함.

---

## 4. 프로젝트 구조 (Directory Structure)

```text
├── Day 4/clock/
│   ├── clock.c     # LCD, ADC, Timer0 인터럽트, 시계 동작
│   └── REPORT.md   # 과제 보고서
```

프로그램은 하나의 C 소스 파일로 구성함. I2C 통신과 LCD 출력 함수를 분리하고, 시간 설정 함수와 Timer0 인터럽트 함수를 별도로 작성하여 메인 함수의 흐름을 단순하게 구성함.

---

## 5. 핵심 코드 및 레지스터 설정 (Key Implementation)

### 월별 마지막 날짜 및 윤년 처리

```c
char lastDay(int y, char m)
{
    char d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
    {
        return 29;
    }

    return d[m - 1];
}
```

`d` 배열에는 각 월의 기본 마지막 날짜를 저장함. 2월인 경우에는 윤년 조건을 검사하여 29일을 반환함. 이 함수의 결과를 일(day) 설정의 최댓값과 날짜 증가 과정에 모두 사용하므로 2월 30일, 4월 31일처럼 존재하지 않는 날짜가 만들어지지 않음.

### I2C 초기화와 LCD 데이터 전송

```c
void twiInit(void)
{
    TWSR = 0x00;
    TWBR = 72;
    TWCR = (1 << TWEN);
}
```

`TWSR = 0x00`은 분주비를 1로 설정하고, `TWBR = 72`는 16MHz에서 약 100kHz I2C 통신 속도를 설정함. `TWEN`은 ATmega128의 TWI(I2C) 기능을 활성화하는 비트임.

```c
void lcdExpander(uint8_t data)
{
    twiStart();
    twiWrite(LCD_ADDRESS << 1);
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
```

`lcdExpander()`는 I2C 시작 신호를 보낸 뒤 LCD 주소와 데이터 1바이트를 전송함. `LCD_ADDRESS << 1`은 7비트 I2C 주소를 쓰기 전송용 값으로 변환하는 과정임. `lcdPulse()`는 Enable 비트를 잠깐 1로 만들었다가 0으로 내려 LCD가 현재 데이터를 읽도록 하는 함수임.

### LCD 초기화 및 문자열 출력

```c
void lcdInit(void)
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
```

전원 인가 직후 LCD 상태를 맞추기 위해 `0x30`을 여러 번 전송함. 이후 `0x20`으로 4비트 모드를 설정함. `0x28`은 2줄 표시, `0x0C`는 화면 표시 ON, `0x06`은 글자 입력 방향 설정, `0x01`은 전체 화면 지우기 명령임.

```c
void lcdString(char row, char column, char *text)
{
    lcdPosition(row, column);

    while (*text != '\0')
    {
        lcdData(*text);
        text++;
    }
}
```

`lcdPosition()`으로 출력 위치를 정한 뒤, 문자열의 끝 문자 `\0`을 만날 때까지 한 글자씩 LCD로 전송함. 예를 들어 `lcdString(0, 0, "PUSH SW2");`를 실행하면 첫째 줄 첫 칸에 문구가 표시됨.

### Timer0 시간 생성

```c
void timer0Init(void)
{
    TCCR0 = (1 << CS01) | (1 << CS00);
    TCNT0 = 6;
    TIMSK |= (1 << TOIE0);
}
```

`CS01`, `CS00`을 1로 설정하여 Timer0 분주비를 64로 설정함. 16MHz 클럭에서 타이머 1틱은 4us이고, `TCNT0 = 6`부터 256까지 250틱이 지나면 오버플로우하므로 약 1ms마다 인터럽트가 발생함. `TOIE0`은 Timer0 오버플로우 인터럽트를 허용하는 비트임.

```c
ISR(TIMER0_OVF_vect)
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
}
```

Timer0가 오버플로우할 때마다 인터럽트 함수가 실행됨. `run`이 0이면 아직 시계를 시작하지 않은 상태이므로 시간을 증가시키지 않음. 약 1ms마다 `msCnt`를 증가시키고 10번이 쌓이면 `csec`를 1 증가시켜 0.01초 단위를 생성함.

```c
csec = 0;
sec++;

if (sec < 60)
{
    return;
}

sec = 0;
minute++;
```

`csec`가 100이면 1초가 지나므로 0으로 초기화하고 `sec`를 증가시킴. 같은 방식으로 초가 60이면 분을, 분이 60이면 시를 증가시킴. 시가 24가 되면 날짜를 하루 증가시키고, `lastDay()` 결과를 넘으면 다음 달 1일로 변경함. 12월이 끝난 경우에는 1월로 바꾸고 연도를 증가시킴.

### ADC 초기화와 가변저항 값 읽기

```c
void adcInit(void)
{
    ADMUX = (1 << REFS0);

    ADCSRA = (1 << ADEN) |
              (1 << ADPS2) |
              (1 << ADPS1) |
              (1 << ADPS0);
}

int adcRead(void)
{
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADCW;
}
```

`REFS0`은 ADC 기준전압을 AVCC(5V)로 설정함. MUX 비트를 별도로 설정하지 않았으므로 기본 채널인 ADC0, 즉 PF0을 사용함. `ADEN`은 ADC를 활성화하고, `ADPS2~0`은 ADC 클럭을 128분주하여 안정적인 변환 속도를 만들기 위한 설정임. `adcRead()`는 `ADSC`로 변환을 시작하고 변환 완료 후 10비트 결과 `ADCW`를 반환함.

### SW1 입력과 가변저항 값 확정

```c
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
```

스위치는 Active-Low 방식이므로 해당 비트가 0일 때 눌린 상태임. 20ms 후 다시 검사하여 채터링으로 인한 잘못된 입력을 줄임. 이후 버튼을 놓을 때까지 기다리므로 버튼을 계속 누르고 있어도 한 번의 입력으로만 처리됨.

```c
v = lo + (long)adcRead() * (hi - lo + 1) / 1024;

if (v > hi)
{
    v = hi;
}
```

ADC 결과는 0~1023이므로 위 식으로 필요한 범위 `lo~hi`로 변환함. 예를 들어 월 설정은 1~12, 시 설정은 0~23 범위가 됨. 최댓값 보정은 ADC 값이 1023일 때 계산 결과가 `hi + 1`이 되는 것을 방지함.

### 메인 함수의 설정 및 동작 흐름

```c
DDRC &= ~((1 << SW1) | (1 << SW2));
PORTC |= (1 << SW1) | (1 << SW2);

DDRF &= ~(1 << PF0);
PORTF &= ~(1 << PF0);

adcInit();
twiInit();
lcdInit();
timer0Init();
sei();
```

PC0, PC1을 입력으로 설정하고 내부 풀업을 켬. PF0은 ADC 입력으로 설정함. 이후 ADC, TWI, LCD, Timer0를 초기화한 뒤 `sei()`로 전역 인터럽트를 허용함.

```c
year = 2000 + setValue("YEAR", 0, 99);
month = setValue("MONTH", 1, 12);
day = setValue("DAY", 1, lastDay(year, month));
hour = setValue("HOUR", 0, 23);
minute = setValue("MIN", 0, 59);
sec = setValue("SEC", 0, 59);
```

가변저항으로 연도, 월, 일, 시, 분, 초를 차례로 설정함. 각 단계에서 SW1을 누르면 현재 값이 확정됨. 일 설정의 최댓값에는 `lastDay(year, month)`를 사용하므로 앞에서 정한 연도와 월에 맞는 일수만 선택할 수 있음.

```c
while (!swPushed(SW2))
{
}

lcdClear();
run = 1;
```

모든 설정이 끝나면 LCD에 `PUSH SW2 / TO START`를 표시함. SW2를 누르면 `run`이 1이 되고, 그때부터 Timer0 인터럽트가 시간을 증가시킴.

```c
cli();
y = year;
mo = month;
d = day;
h = hour;
mi = minute;
s = sec;
c = csec;
sei();
```

시간 변수는 인터럽트 함수에서 계속 바뀜. 따라서 LCD에 표시할 지역 변수로 복사하는 동안 `cli()`로 인터럽트를 잠시 막고, 복사가 끝난 뒤 `sei()`로 다시 허용함. 이 과정으로 날짜와 시간 값이 바뀌는 중간에 서로 섞여 표시되는 문제를 방지함.

---

## 6. 동작 설명 및 결과 (Results)

### 동작 시나리오

1. 전원을 인가하면 ADC, I2C LCD, Timer0와 스위치 입력을 초기화함
2. LCD에 `SW1 TO FIX / SET THE CLOCK`를 표시함
3. 가변저항으로 YEAR, MONTH, DAY, HOUR, MIN, SEC를 차례대로 정하고 SW1으로 확정함
4. LCD에 `PUSH SW2 / TO START`를 표시함
5. SW2를 누르면 Timer0 인터럽트가 시간을 증가시킴
6. LCD 첫째 줄에는 날짜, 둘째 줄에는 시·분·초·1/100초를 표시함

### LCD 표시 형식

| LCD 위치 | 출력 형식 | 예시 |
| :--- | :--- | :--- |
| 첫째 줄 | `YYMMDD` | `260803` |
| 둘째 줄 | `HH:MM:SS.CC` | `21:15:08.37` |

`%02d` 형식 지정자를 사용하여 값이 한 자리여도 앞에 `0`을 붙여 항상 두 자리로 표시함.

### 예외처리

| 처리 항목 | 코드 동작 | 방지하는 문제 |
| :--- | :--- | :--- |
| 윤년 처리 | `lastDay()`에서 4년·100년·400년 조건 검사 | 윤년의 2월 29일 누락 |
| 월별 일수 제한 | 일 설정 최댓값에 `lastDay(year, month)` 사용 | 2월 30일, 4월 31일 등 잘못된 날짜 |
| 날짜 넘김 | 날짜가 마지막 일을 넘으면 다음 달 1일로 변경 | 날짜가 해당 월 범위를 벗어남 |
| ADC 범위 보정 | `if (v > hi) v = hi;` | 최대 ADC 값에서 설정값이 최댓값을 넘음 |
| 스위치 채터링 | 20ms 지연 후 재검사 | 버튼 한 번이 여러 번 입력됨 |
| 길게 누름 방지 | 버튼을 놓을 때까지 대기 | 버튼을 계속 누를 때 반복 확정됨 |
| 공유 변수 보호 | `cli()` 후 시간값 복사, `sei()`로 복구 | 인터럽트 갱신 중 시간 표시가 섞임 |

현재 코드는 LCD I2C 통신이나 ADC 변환이 완료될 때까지 `while`문에서 기다리는 방식임. 따라서 LCD 연결 불량 또는 ADC 하드웨어 이상이 발생했을 때 빠져나오는 통신 타임아웃 예외처리는 포함되어 있지 않음.

---

## 7. AI 툴 활용 명시 (AI Tools Declaration)

본 과제 작성 및 구현 과정에서 AI 도구를 다음 목적으로 활용함.

| 도구명 (Tool) | 활용 영역 | 세부 사용 목적 및 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 이해 | Timer0 분주비와 오버플로우 주기 계산, ADC 값 범위 변환 원리 확인 |
| **ChatGPT** | 코드 검토 | I2C LCD 초기화 순서, 날짜 증가 및 윤년 조건 검토 |
| **ChatGPT** | 보고서 작성 | 코드 단락별 역할과 예외처리 항목을 보고서 형식으로 정리 |

### AI 활용 및 검증 원칙

1. 코드의 핀 연결, 레지스터 설정, 시간 계산 결과는 ATmega128 데이터시트와 실제 회로 연결 기준으로 확인함.
2. 날짜와 시간 설정 범위는 코드의 `lastDay()` 및 `setValue()` 동작을 기준으로 직접 검토함.
3. AI는 개념 이해와 코드 구조 검토를 위한 보조 도구로 사용하였으며, 최종 회로 연결과 동작 확인은 직접 수행함.
