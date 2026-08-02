# 3일차 과제 : 소프트웨어 UART 기반 문자열 송신 시스템

> **광운대학교 전자융합공학과**  
> **작성자:** 석주형  
> **제출일:** 2026년 8월 3일

---

## 1. 개요 (Overview)
본 과제는 ATmega128의 내장 UART 레지스터를 사용하지 않고, PD3 핀의 HIGH와 LOW 출력을 직접 제어하여 UART 통신 신호를 생성하는 것을 목표로 함. Timer1으로 비트 하나의 시간을 일정하게 유지하고, `HelloWorld!` 문자열을 9600bps 속도로 반복 전송하도록 구현함.

### 핵심 목표
* PD3 핀을 소프트웨어 UART 송신 핀으로 설정
* Timer1을 이용하여 9600bps 비트 시간 생성
* 시작 비트, 데이터 비트, 정지 비트를 직접 전송
* ASCII 16진수 코드를 이용하여 `HelloWorld!` 문자열 송신
* 문자열 전송 후 줄바꿈 및 약 0.1초 대기

---

## 2. 개발 환경 (Environment)

| 항목 | 내용 |
| :--- | :--- |
| **MCU** | ATmega128A (16MHz External Crystal) |
| **IDE / Compiler** | Microchip Studio / AVR-GCC |
| **Flasher Tool** | STK500 |
| **Terminal** | 시리얼 모니터 (9600bps) |
| **언어** | C Language |
| **주요 부품** | ROBIT 실습보드, USB-UART 어댑터 |

---

## 3. 하드웨어 구성 및 핀 맵 (Hardware Structure)

### Pin Configuration

```text
[ATmega128]                 [USB-UART 어댑터]
 PD3                  ----->   RXD
 GND                  <---->   GND
```

PD3은 데이터를 보내는 송신 핀으로 사용함. USB-UART 어댑터의 RXD 핀과 연결해야 하며, ATmega128 보드와 USB-UART 어댑터의 GND도 반드시 서로 연결해야 함.

UART 통신은 전압의 절대값이 아닌 두 장치가 공유하는 GND를 기준으로 HIGH와 LOW를 판단함. 따라서 PD3과 RXD만 연결하고 GND를 연결하지 않으면 신호 기준이 달라져 정상 수신되지 않을 수 있음.

### UART 신호 구성

UART는 송신하지 않을 때 HIGH 상태를 유지함. 1바이트를 보낼 때는 먼저 LOW 상태의 시작 비트를 보내고, 데이터 8비트를 낮은 비트부터 전송함. 마지막에는 HIGH 상태의 정지 비트를 보내 전송을 종료함.

```text
Idle        Start bit       Data 8bit                 Stop bit
HIGH   ->      LOW     ->   bit0 ~ bit7          ->     HIGH
```

---

## 4. 프로젝트 구조 (Directory Structure)
> 구현부(.c)와 보고서 파일만 구조에 표기함.

```text
├── Day 3/Software_UART/
│   ├── main.c        # Timer1, 비트 지연, 1바이트 송신
│   └── REPORT.md
```

프로젝트는 `main.c` 파일 하나로 구성함. `wait_bit()` 함수는 비트 시간을 만들고, `send_byte()` 함수는 UART 형식으로 1바이트를 전송하며, `main()` 함수는 초기 설정과 문자열 반복 송신을 담당함.

---

## 5. 핵심 코드 및 레지스터 설정 (Key Implementation)

### 비트 시간 생성 (main.c)

```c
#define BIT_TICKS 208

void wait_bit(void)
{
    TCNT1 = 0;

    while (TCNT1 < BIT_TICKS)
    {
    }
}
```

9600bps에서는 비트 하나를 약 104마이크로초 동안 유지해야 함.

```text
1 / 9600 = 약 104마이크로초
```

Timer1은 아래 설정에 의해 8분주로 동작함.

```c
TCCR1A = 0x00;
TCCR1B = (1 << CS11);
```

```text
16MHz / 8 = 2MHz
1 / 2MHz = 0.5마이크로초
104마이크로초 / 0.5마이크로초 = 208
```

따라서 `BIT_TICKS`를 208로 설정함. `wait_bit()` 함수는 `TCNT1`을 0으로 초기화하고 값이 208이 될 때까지 기다림. 이 과정으로 약 104마이크로초의 비트 시간이 생성됨.

`TCCR1A = 0x00`은 Timer1을 일반 타이머 방식으로 사용하기 위한 초기화 코드임. `TCCR1B = (1 << CS11)`은 `CS11` 비트를 1로 설정하여 Timer1의 분주비를 8로 설정함.

### 시작 비트, 데이터 비트, 정지 비트 전송

```c
PORTD &= ~(1 << TX_BIT);
wait_bit();

for (a = 0; a < 8; a++)
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

PORTD |= (1 << TX_BIT);
wait_bit();
```

첫 번째 `PORTD &= ~(1 << TX_BIT)`는 PD3을 LOW로 만들어 시작 비트를 전송함. 이후 반복문은 데이터의 0번 비트부터 7번 비트까지 확인하여, 비트가 1이면 HIGH를 보내고 비트가 0이면 LOW를 보냄.

```c
data & (1 << a)
```

위 연산은 `data`의 a번째 비트가 1인지 확인하는 코드임. UART는 가장 낮은 비트인 bit0부터 먼저 보내는 방식이므로 반복 변수 `a`를 0부터 7까지 증가시킴. 데이터 8비트를 모두 보낸 뒤에는 PD3을 HIGH로 만들어 정지 비트를 전송함.

### PD3 출력 설정 및 인터럽트 비활성화

```c
cli();

DDRD |= (1 << TX_BIT);
PORTD |= (1 << TX_BIT);
```

`DDRD`의 PD3 비트를 1로 설정하여 PD3을 출력으로 사용함. UART는 유휴 상태에서 HIGH를 유지하므로 `PORTD`의 PD3 비트도 처음에 1로 설정함.

인터럽트가 비트를 보내는 중간에 실행되면 `wait_bit()`의 실제 시간이 길어져 통신 속도가 달라질 수 있음. 이를 방지하기 위해 `cli()`를 사용하여 인터럽트를 비활성화함.

### 전체 코드

```c
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
    // Timer1을 더느리게 설정

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
```

---

## 6. 동작 설명 및 결과 (Results)

### 동작 시나리오
1. 전원 인가 시 인터럽트를 비활성화하고 PD3을 출력으로 설정함
2. Timer1을 8분주로 설정하여 2MHz로 동작시킴
3. `send_byte()` 함수가 시작 비트, 데이터 8비트, 정지 비트를 순서대로 전송함
4. `HelloWorld!`의 ASCII 코드들을 차례대로 전송함
5. `0x0D`, `0x0A`를 전송하여 줄바꿈을 수행함
6. 약 0.1초 대기한 뒤 같은 과정을 반복함

### 시리얼 모니터 설정

| 항목 | 설정 |
| :--- | :--- |
| Baud Rate | 9600bps |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

### 예상 출력 결과

```text
HelloWorld!
HelloWorld!
HelloWorld!
```

`0x48`은 `H`, `0x65`는 `e`, `0x6C`는 `l`과 같이 각 16진수 값은 ASCII 문자 코드임. `0x0D`는 커서를 줄 처음으로 이동시키는 CR 코드이고, `0x0A`는 다음 줄로 이동시키는 LF 코드임.

---

## 7. AI 툴 활용 명시 (AI Tools Declaration)
본 과제 작성 및 구현 과정에서 활용한 AI 도구의 사용 현황 및 목적은 다음과 같음.

| 도구명 (Tool) | 활용 영역 | 세부 사용 목적 및 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 이해 | - UART의 시작 비트, 데이터 비트, 정지 비트 구조 확인<br>- Timer1 분주와 비트 시간 계산 확인 |
| **ChatGPT** | 코드 검토 | - PD3 출력 설정과 비트 전송 순서 검토<br>- 9600bps에서 `BIT_TICKS` 값이 208인 이유 확인 |
| **ChatGPT** | 보고서 작성 | - 코드 동작 과정과 레지스터 설정을 보고서 형식으로 정리 |

### AI 활용 및 검증 원칙
1. **코드 검증:** 작성한 코드는 직접 컴파일하고 ATmega128 보드에 업로드하여 시리얼 모니터 출력으로 확인함.
2. **통신 설정 확인:** 시리얼 모니터의 통신 속도를 9600bps, 8N1로 설정하여 코드의 비트 시간과 일치하도록 확인함.
3. **학습 주도성:** AI는 UART 구조와 시간 계산의 이해를 돕는 용도로 활용하였으며, 코드 작성과 보드 연결 및 실행 확인은 직접 수행함.
