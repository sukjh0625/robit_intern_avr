# ATmega128 UART 통신 및 레지스터 보고서

## 1. UART의 정의

UART(Universal Asynchronous Receiver/Transmitter)는 병렬 데이터를 직렬 데이터로 변환하여 송신하고, 수신한 직렬 데이터를 다시 병렬 데이터로 변환하는 통신 방식이다. 별도의 클럭선을 사용하지 않으므로 송신 장치와 수신 장치가 Baud Rate, 데이터 비트 수, 패리티 비트, 스톱 비트 수를 동일하게 설정해야 한다. ATmega128에는 USART0과 USART1이 있으며, 비동기 방식에서는 일반적인 UART로 사용할 수 있다.

ATmega128의 UART는 전이중 통신을 지원하므로 송신과 수신을 동시에 할 수 있다. 기본적인 UART 프레임은 Start Bit 1개, Data Bit 8개, Parity Bit 없음, Stop Bit 1개로 구성하며, 이를 `8N1`이라고 한다.

```text
대기 상태(HIGH) -> Start Bit(LOW) -> Data Bit 0~7 -> Stop Bit(HIGH)
```

UART 데이터는 가장 낮은 비트인 LSB부터 전송된다. 예를 들어 문자 `H`는 ASCII 값 `0x48`, 10진수로 `72`, 2진수로 `01001000`이다. 실제 전송 순서는 LSB부터이므로 `0, 0, 0, 1, 0, 0, 1, 0` 순서가 된다.

## 2. ATmega128 UART 핀

| 구분 | 수신 핀 | 송신 핀 | 외부 클럭 핀 |
|---|---|---|---|
| USART0 | PE0(RXD0) | PE1(TXD0) | PE2(XCK0) |
| USART1 | PD2(RXD1) | PD3(TXD1) | PD5(XCK1) |

송신 장치의 TXD는 수신 장치의 RXD에 연결하고, 수신 장치의 TXD는 송신 장치의 RXD에 연결한다. 두 장치는 반드시 GND도 공통으로 연결해야 한다.

## 3. UART에서 사용하는 레지스터

레지스터 이름에 들어가는 `n`은 사용할 USART 번호를 뜻한다. USART0을 사용하면 `UCSR0A`, `UCSR0B`처럼 `n`에 0을 넣고, USART1을 사용하면 `UCSR1A`, `UCSR1B`처럼 `n`에 1을 넣는다.

### 3.1 UDRn - USART Data Register

`UDRn`은 실제 송수신 데이터를 저장하는 8비트 레지스터이다. 이 레지스터에 값을 쓰면 데이터가 송신되고, 이 레지스터를 읽으면 수신된 데이터를 가져올 수 있다.

```c
UDR0 = data;
```

위 코드는 `data`의 값을 USART0으로 송신한다.

```c
data = UDR0;
```

위 코드는 USART0으로 수신된 데이터를 읽는다. 송신할 때는 `UCSR0A`의 `UDRE0`가 1인지 먼저 확인해야 한다. `UDRE0`가 0일 때 `UDR0`에 데이터를 쓰면 해당 데이터가 무시될 수 있다.

### 3.2 UCSRnA - USART Control and Status Register A

`UCSRnA`는 UART의 송수신 상태와 오류 상태를 확인하고, 2배속 모드 등을 설정하는 레지스터이다.

| 비트 | 이름 | 정의 |
|---:|---|---|
| 7 | RXCn | 수신 버퍼에 읽지 않은 데이터가 있으면 1 |
| 6 | TXCn | 송신 프레임이 완전히 전송되고 송신 버퍼도 비어 있으면 1 |
| 5 | UDREn | 송신 버퍼가 비어 새로운 데이터를 쓸 수 있으면 1 |
| 4 | FEn | 수신된 프레임의 Stop Bit가 올바르지 않으면 1 |
| 3 | DORn | 기존 수신 데이터를 처리하기 전에 새 데이터가 들어와 오버런이 발생하면 1 |
| 2 | UPEn | 수신 데이터에서 패리티 오류가 발생하면 1 |
| 1 | U2Xn | 비동기 통신의 Baud Rate 분주값을 16에서 8로 바꾸는 2배속 설정 |
| 0 | MPCMn | 멀티프로세서 통신 모드 활성화 |

일반 속도 비동기 통신에서는 다음과 같이 설정할 수 있다.

```c
UCSR0A = 0x00;
```

2배속 비동기 통신을 사용하려면 다음과 같이 `U2X0`을 1로 설정한다.

```c
UCSR0A = (1 << U2X0);
```

### 3.3 UCSRnB - USART Control and Status Register B

`UCSRnB`는 UART 송수신 기능과 UART 인터럽트를 활성화하고 데이터 비트 수를 설정하는 레지스터이다.

| 비트 | 이름 | 정의 |
|---:|---|---|
| 7 | RXCIEn | 수신 완료 인터럽트 활성화 |
| 6 | TXCIEn | 송신 완료 인터럽트 활성화 |
| 5 | UDRIEn | 송신 데이터 레지스터 준비 인터럽트 활성화 |
| 4 | RXENn | 수신 기능 활성화 |
| 3 | TXENn | 송신 기능 활성화 |
| 2 | UCSZn2 | `UCSRnC`의 `UCSZn1:0`과 함께 데이터 비트 수 설정 |
| 1 | RXB8n | 9비트 수신 모드에서 수신한 9번째 비트 |
| 0 | TXB8n | 9비트 송신 모드에서 송신할 9번째 비트 |

USART0의 송신과 수신을 모두 사용하고 인터럽트는 사용하지 않는 경우 다음과 같이 설정한다.

```c
UCSR0B = (1 << RXEN0) | (1 << TXEN0);
```

이 설정을 2진수와 16진수로 나타내면 다음과 같다.

```text
UCSR0B = 0b00011000 = 0x18
```

### 3.4 UCSRnC - USART Control and Status Register C

`UCSRnC`는 비동기 또는 동기 모드, 패리티, 스톱 비트 수, 데이터 비트 수와 같은 UART 프레임 형식을 설정하는 레지스터이다.

| 비트 | 이름 | 정의 |
|---:|---|---|
| 7 | Reserved | 예약 비트이므로 0으로 설정 |
| 6 | UMSELn | 0: 비동기 모드, 1: 동기 모드 |
| 5~4 | UPMn1:0 | `00`: 패리티 없음, `10`: 짝수 패리티, `11`: 홀수 패리티 |
| 3 | USBSn | 0: Stop Bit 1개, 1: Stop Bit 2개 |
| 2~1 | UCSZn1:0 | `UCSZn2`와 함께 데이터 비트 수 설정 |
| 0 | UCPOLn | 동기 모드에서 사용하는 클럭 극성 설정 |

데이터 비트 수는 `UCSZn2:0` 조합으로 정한다.

| UCSZn2 | UCSZn1 | UCSZn0 | 데이터 비트 수 |
|---:|---:|---:|---:|
| 0 | 0 | 0 | 5비트 |
| 0 | 0 | 1 | 6비트 |
| 0 | 1 | 0 | 7비트 |
| 0 | 1 | 1 | 8비트 |
| 1 | 1 | 1 | 9비트 |

가장 일반적인 비동기, 패리티 없음, Stop Bit 1개, Data Bit 8개의 `8N1`은 다음과 같이 설정한다.

```c
UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
```

이를 2진수와 16진수로 나타내면 다음과 같다.

```text
UCSR0C = 0b00000110 = 0x06
```

### 3.5 UBRRnH, UBRRnL - USART Baud Rate Register

`UBRRnH`와 `UBRRnL`은 UART의 통신 속도인 Baud Rate를 설정하는 레지스터이다. 전체 UBRR 값은 12비트이며, 상위 4비트는 `UBRRnH`, 하위 8비트는 `UBRRnL`에 저장한다.

일반 속도 비동기 모드에서 UBRR 값은 다음 식으로 계산한다.

```text
UBRR = F_CPU / (16 × Baud Rate) - 1
```

2배속 비동기 모드에서 UBRR 값은 다음 식으로 계산한다.

```text
UBRR = F_CPU / (8 × Baud Rate) - 1
```

16 MHz에서 일반 속도로 9600 bps를 사용할 경우 다음과 같이 계산한다.

```text
UBRR = 16,000,000 / (16 × 9,600) - 1
     = 103.166...
     ≒ 103
```

따라서 다음과 같이 설정한다.

```c
UBRR0H = 0;
UBRR0L = 103;
```

16 MHz에서 자주 사용하는 설정값은 다음과 같다.

| Baud Rate | U2X0 | UBRR 값 | 실제 Baud Rate | 오차율 |
|---:|---:|---:|---:|---:|
| 9600 | 0 | 103 | 9615.38 | 약 +0.16% |
| 57600 | 0 | 16 | 58823.53 | 약 +2.12% |
| 57600 | 1 | 34 | 57142.86 | 약 -0.79% |

57600 bps에서는 일반 속도의 UBRR 16도 사용할 수 있지만, 통신 오차를 줄이려면 `U2X0=1`, `UBRR0=34` 설정이 더 적합하다. 송수신 중에 UBRR 값을 변경하면 현재 전송 중인 데이터가 손상될 수 있으므로 초기화할 때 설정해야 한다.

## 4. UART0 레지스터 설정

다음은 ATmega128, 16 MHz 환경에서 USART0을 9600 bps, 8N1, 송수신 모드로 설정하는 코드이다.

```c
#define F_CPU 16000000UL
#define BAUD_RATE 9600UL
#define UBRR_VALUE ((F_CPU / (16UL * BAUD_RATE)) - 1)

#include <avr/io.h>

void UART0_init(void)
{
    UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
    UBRR0L = (unsigned char)UBRR_VALUE;

    UCSR0A = 0x00;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
```

각 설정의 의미는 다음과 같다.

| 코드 | 설정 내용 |
|---|---|
| `UBRR0H`, `UBRR0L` | 16 MHz에서 9600 bps가 되도록 UBRR 값을 103으로 설정 |
| `UCSR0A = 0x00` | 일반 속도 비동기 통신 사용 |
| `RXEN0 = 1` | USART0 수신 활성화 |
| `TXEN0 = 1` | USART0 송신 활성화 |
| `UCSZ01 = 1`, `UCSZ00 = 1` | 데이터 비트 8개 설정 |
| 나머지 `UCSR0C` 비트 0 | 비동기, 패리티 없음, Stop Bit 1개 설정 |

## 5. UART 송신 및 수신 함수

### 5.1 1바이트 송신

```c
void UART0_transmit(unsigned char data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}
```

`UDRE0`가 1이면 송신 버퍼가 비어 있다는 뜻이다. 따라서 `while`문은 송신 버퍼가 준비될 때까지 기다리고, 준비가 끝나면 `UDR0`에 데이터를 넣어 송신한다.

### 5.2 1바이트 수신

```c
unsigned char UART0_receive(void)
{
    while (!(UCSR0A & (1 << RXC0)))
    {
    }

    return UDR0;
}
```

`RXC0`가 1이면 수신 버퍼에 읽지 않은 데이터가 있다는 뜻이다. 따라서 `while`문은 데이터가 도착할 때까지 기다리고, 데이터가 도착하면 `UDR0`을 읽어 반환한다.

### 5.3 문자열 송신

```c
void UART0_string(const char *text)
{
    while (*text != '\0')
    {
        UART0_transmit(*text);
        text++;
    }
}
```

문자열의 끝을 나타내는 널 문자 `\0`이 나올 때까지 문자를 한 글자씩 송신한다.

## 6. 전체 예제 코드

다음 코드는 1초마다 `HelloWorld!` 문자열을 USART0으로 전송한다.

```c
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#define BAUD_RATE 9600UL
#define UBRR_VALUE ((F_CPU / (16UL * BAUD_RATE)) - 1)

void UART0_init(void)
{
    UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
    UBRR0L = (unsigned char)UBRR_VALUE;

    UCSR0A = 0x00;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART0_transmit(unsigned char data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

void UART0_string(const char *text)
{
    while (*text != '\0')
    {
        UART0_transmit(*text);
        text++;
    }
}

int main(void)
{
    UART0_init();

    while (1)
    {
        UART0_string("HelloWorld!\r\n");
        _delay_ms(1000);
    }
}
```

## 7. 레지스터 설정 정리

USART0을 16 MHz, 9600 bps, 8N1, 송수신 방식으로 사용할 때의 최종 설정값은 다음과 같다.

| 레지스터 | 설정값 | 의미 |
|---|---:|---|
| `UBRR0H` | `0x00` | UBRR 값 103의 상위 비트 |
| `UBRR0L` | `0x67` | UBRR 값 103의 하위 8비트 |
| `UCSR0A` | `0x00` | 일반 속도 모드 |
| `UCSR0B` | `0x18` | 송신과 수신 활성화 |
| `UCSR0C` | `0x06` | 비동기, 패리티 없음, Stop Bit 1개, Data Bit 8개 |
| `UDR0` | 송수신 데이터 | 쓰면 송신, 읽으면 수신 |

## 8. 결론

ATmega128의 UART 통신을 사용하려면 먼저 `UBRRnH`와 `UBRRnL`로 Baud Rate를 정하고, `UCSRnB`로 송수신 기능을 활성화하며, `UCSRnC`로 데이터 프레임 형식을 설정해야 한다. 송신할 때는 `UDREn`을 확인한 뒤 `UDRn`에 데이터를 쓰고, 수신할 때는 `RXCn`을 확인한 뒤 `UDRn`을 읽는다. 송신 장치와 수신 장치의 Baud Rate와 8N1 같은 통신 조건이 다르면 문자가 깨지거나 데이터가 수신되지 않으므로 양쪽 설정을 동일하게 맞추는 것이 중요하다.

## 9. 참고 자료

1. **ATmega128 Datasheet**, USART 항목, USART 레지스터 설명(pp. 171-194).
2. **2026 로봇게임단 수습단원교육 MCU 3일차 UART**, UART 프레임, 레지스터 및 응용 내용.

