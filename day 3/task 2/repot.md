# 3일차 과제 2 : UART 기반 LED 제어 시스템

> **광운대학교 로봇학부**  
> **작성자:** 석주형  
> **제출일:** 2026년 8월 3일

---

## 1. 개요 (Overview)

본 과제는 ATmega128과 PC 사이의 UART0 통신을 이용하여 LED를 제어하는 것을 목표로 함. PC에서 문자 `0`~`9`를 수신하여 해당 LED를 켜거나 LED 위치를 이동시키고, 실행 결과를 다시 PC로 전송하도록 구현함. 또한 SW1을 누르면 LED 상태를 초기화하고 `RESET` 문자열을 전송하도록 구현함.

### 핵심 목표

* UART0 송신 및 수신 설정
* PC에서 입력한 `0`~`7`에 따른 LED 선택 점등
* `8`, `9` 입력에 따른 LED 위치 순환 이동
* SW1 입력으로 전체 LED 초기화 및 `RESET` 전송
* 범위를 벗어난 문자 입력을 무시하는 예외처리

---

## 2. 개발 환경 (Environment)

| 항목 | 내용 |
| :--- | :--- |
| **MCU** | ATmega128 (16MHz External Crystal) |
| **IDE / Compiler** | Microchip Studio / AVR-GCC |
| **Programmer** | STK500 계열 프로그래머 |
| **Terminal** | Serial Terminal |
| **언어** | C Language |
| **주요 부품** | ROBIT 실습보드, USB-UART 연결, LED 8개, SW1 |

---

## 3. 하드웨어 구성 및 핀 맵 (Hardware Structure)

### Pin Configuration

```text
[ATmega128]                 [Target Component]
 PA0 ~ PA7          ----->   LED 0 ~ LED 7
 PC0                <-----   SW1
 PE0 (RXD0)         <-----   USB-UART TX
 PE1 (TXD0)         ----->   USB-UART RX
 GND                <---->   USB-UART GND
```

### LED와 스위치 구성

LED는 PA0~PA7에 연결되어 있으며 Active-Low 방식으로 동작함. 따라서 해당 포트 비트에 `0`을 출력하면 LED가 켜지고, `1`을 출력하면 LED가 꺼짐. 프로그램 시작 시 `PORTA = 0xFF`로 설정하여 LED 전체를 끈 상태에서 시작함.

SW1은 PC0에 연결되어 있으며 내부 풀업 저항을 사용함. 따라서 평소 PC0에는 `1`이 입력되고, 버튼을 누르면 GND와 연결되어 `0`이 입력됨. 버튼 입력에는 20ms 지연을 넣어 채터링에 의해 한 번의 입력이 여러 번 인식되는 현상을 줄임.

### UART0 통신 설정

본 과제는 UART0의 PE0(RXD0), PE1(TXD0)을 사용함. 16MHz 클럭에서 `UBRR0 = 16`, `U2X0 = 1`로 설정하여 약 115200bps 통신 속도를 사용함. 데이터 형식은 데이터 8비트, 패리티 없음, 정지 비트 1개인 8N1 방식임.

```text
UCSR0A = 0x20  -> U2X0 = 1, 2배속 모드
UCSR0B = 0x18  -> RXEN0, TXEN0 활성화
UCSR0C = 0x06  -> 8-bit, No parity, 1 stop bit
UBRR0  = 16    -> 약 115200bps
```

---

## 4. 프로젝트 구조 (Directory Structure)

```text
├── Day 3/hw2/
│   ├── hw2.c       # UART 수신, LED 제어, 스위치 초기화
│   └── REPORT.md   # 과제 보고서
```

프로그램은 하나의 C 소스 파일로 구성함. UART 문자 전송 함수, 문자열 전송 함수, LED 점등 함수를 분리하여 메인 반복문에서 쉽게 사용할 수 있도록 구성함.

---

## 5. 핵심 코드 및 레지스터 설정 (Key Implementation)

### UART 문자 전송 함수

```c
void uart_putch(unsigned char data)
{
   while (!(UCSR0A & (1 << UDRE0)))
   {
   }

   UDR0 = data;
}
```

`UDRE0` 비트는 UART 송신 데이터 레지스터가 비어 있는지 나타냄. 이전 문자가 아직 전송 중이면 레지스터에 새 문자를 넣을 수 없으므로, `UDRE0`가 1이 될 때까지 기다린 후 `UDR0`에 문자 하나를 저장함. `UDR0`에 저장된 문자는 TXD0 핀을 통해 PC로 전송됨.

### 문자열 전송 함수

```c
void uart_string(const char *text)
{
   while (*text != '\0')
   {
      uart_putch(*text);
      text++;
   }
}
```

문자열은 마지막에 널 문자 `\0`을 포함하므로, 널 문자를 만날 때까지 문자 하나씩 `uart_putch()` 함수로 전송함. 예를 들어 `uart_string("RESET\r\n");`를 실행하면 PC 터미널에 `RESET`을 출력하고 다음 줄로 이동함.

### Active-Low LED 점등 함수

```c
void led_on(unsigned char number)
{
   PORTA = (unsigned char)~(1 << number);
}
```

`1 << number`는 선택한 LED 위치만 1인 값을 생성함. 이를 `~` 연산자로 반전하면 선택한 위치만 0이 되고 나머지 위치는 1이 됨. LED가 Active-Low 방식이므로 선택한 LED만 켜지고 나머지 LED는 꺼짐. 예를 들어 `led_on(3)`은 PA3만 0으로 출력하여 LED 3만 켬.

### SW1 초기화 처리

```c
if (!(PINC & (1 << PC0)) && switchOld == 1)
{
   _delay_ms(20);

   if (!(PINC & (1 << PC0)))
   {
      currentLed = NO_LED;
      PORTA = 0xFF;
      uart_string("RESET\r\n");

      switchOld = 0;
   }
}
```

PC0이 0이면 SW1이 눌린 상태임. 20ms 후에도 버튼이 눌려 있으면 정상 입력으로 판단하여 현재 LED 번호를 `NO_LED`로 초기화하고, `PORTA = 0xFF`로 LED 전체를 끔. 이후 `RESET` 문자열을 PC로 전송함. `switchOld`는 버튼을 계속 누르고 있을 때 초기화 동작이 반복되는 것을 방지하며, 버튼에서 손을 떼면 다시 1로 설정됨.

### UART 수신 및 LED 제어

```c
if (UCSR0A & (1 << RXC0))
{
   receivedData = UDR0;

   if (receivedData >= '0' && receivedData <= '7')
   {
      currentLed = receivedData - '0';
      led_on(currentLed);

      uart_putch(receivedData);
      uart_string(" LED ON\r\n");
   }
}
```

`RXC0` 비트가 1이면 PC에서 새 문자가 수신되었다는 뜻임. 수신 데이터는 `UDR0`에서 읽어 `receivedData`에 저장함. 입력값이 문자 `'0'`~`'7'`인 경우에만 해당 LED를 켬. UART로 들어오는 값은 숫자가 아니라 문자 코드이므로, `'0'`을 빼서 실제 LED 번호로 변환함. 예를 들어 문자 `'3'`을 받으면 `currentLed`에는 숫자 3이 저장되고 LED 3이 켜짐. 이외의 문자 입력은 해당 조건에 들어가지 않으므로 LED 상태를 바꾸지 않음.

### LED 왼쪽 이동

```c
else if (receivedData == '8')
{
   if (currentLed == NO_LED || currentLed == 7)
   {
      currentLed = 0;
   }
   else
   {
      currentLed++;
   }

   led_on(currentLed);
   uart_string("LEFT\r\n");
}
```

문자 `'8'`을 수신하면 LED 번호를 증가시켜 왼쪽 방향으로 이동함. 켜진 LED가 없거나 현재 LED가 7번이면 0번 LED로 이동하고, 그 외에는 번호를 1 증가시킴. 따라서 `0 → 1 → 2 → ... → 7 → 0` 순서로 반복됨. 이동 후에는 `LEFT` 문자열을 PC로 전송함.

### LED 오른쪽 이동

```c
else if (receivedData == '9')
{
   if (currentLed == NO_LED || currentLed == 0)
   {
      currentLed = 7;
   }
   else
   {
      currentLed--;
   }

   led_on(currentLed);
   uart_string("RIGHT\r\n");
}
```

문자 `'9'`를 수신하면 LED 번호를 감소시켜 오른쪽 방향으로 이동함. 켜진 LED가 없거나 현재 LED가 0번이면 7번 LED로 이동하고, 그 외에는 번호를 1 감소시킴. 따라서 `7 → 6 → 5 → ... → 0 → 7` 순서로 반복됨. 이동 후에는 `RIGHT` 문자열을 PC로 전송함.

---

## 6. 동작 설명 및 결과 (Results)

### 동작 시나리오

1. 전원을 인가하면 UART0, LED 포트, SW1 입력을 초기화함
2. PC 터미널에 `READY` 문자열을 출력함
3. PC에서 `0`~`7`을 입력하면 해당 번호 LED 하나가 켜짐
4. PC에서 `8`을 입력하면 LED가 왼쪽으로 한 칸 이동함
5. PC에서 `9`를 입력하면 LED가 오른쪽으로 한 칸 이동함
6. SW1을 누르면 LED 전체가 꺼지고 `RESET`을 출력함

### 입력과 동작 결과

| 입력 | LED 동작 | PC 출력 |
| :--- | :--- | :--- |
| `0`~`7` | 해당 번호 LED 점등 | `n LED ON` |
| `8` | LED 번호 증가, 7 다음은 0 | `LEFT` |
| `9` | LED 번호 감소, 0 다음은 7 | `RIGHT` |
| SW1 | LED 전체 OFF, 상태 초기화 | `RESET` |
| 그 외 문자 | 상태 유지 | 출력 없음 |

### 터미널 출력 예시

```text
READY
3 LED ON
LEFT
RIGHT
RESET
```

### 검증 과정에서 확인한 사항

**문자와 숫자의 차이**  
PC 터미널에서 입력한 `3`은 숫자 3이 아니라 문자 `'3'`으로 수신됨. 따라서 LED 번호로 사용하기 전에 `receivedData - '0'` 연산이 필요함. 이 변환을 하지 않으면 문자 코드값이 LED 번호로 사용되어 정상 동작하지 않음.

**스위치 반복 입력 방지**  
버튼을 계속 누른 상태에서 `RESET`이 반복 출력되는 문제가 발생할 수 있음. 이를 방지하기 위해 `switchOld`를 사용함. 버튼을 처음 누른 순간에만 `switchOld`를 0으로 바꾸고, 버튼에서 손을 뗐을 때만 다시 1로 바꿔 한 번의 누름을 한 번의 동작으로 처리함.

**범위 외 입력 처리**  
과제에서 정의한 입력은 `0`~`9`임. 이외의 문자가 들어오면 LED 번호를 계산하거나 포트에 잘못된 값을 출력하지 않도록 조건문에서 처리하지 않음. 따라서 잘못된 입력에도 기존 LED 상태가 유지됨.

### 동작 사진 / 영상

| 시연 영상 |
| :---: |
| 시연 영상 링크 또는 사진을 추가 |

---

## 7. AI 툴 활용 명시 (AI Tools Declaration)

본 과제에서는 ChatGPT를 UART 레지스터와 통신 흐름의 개념을 이해하고, 코드 동작을 검토하는 용도로 활용함.

| 도구명 (Tool) | 활용 영역 | 세부 사용 목적 및 내용 |
| :--- | :--- | :--- |
| **ChatGPT** | 개념 이해 | UART 송수신 과정, `UDRE0`과 `RXC0` 비트의 의미 확인 |
| **ChatGPT** | 개념 이해 | Active-Low LED 회로와 문자 `'0'`을 숫자 0으로 변환하는 원리 확인 |
| **ChatGPT** | 코드 검토 | 버튼 채터링 방지와 LED 순환 이동 조건의 동작 검토 |

### AI 활용 및 검증 원칙

1. **코드 검증:** 실제 ATmega128 보드와 시리얼 터미널을 연결하여 `0`~`9` 입력 및 SW1 입력 결과를 확인함.
2. **문제 진단:** LED의 Active-Low 특성과 UART 수신 문자 코드값을 레지스터 및 포트 출력값으로 확인함.
3. **학습 주도성:** UART 레지스터 설정과 LED 제어 흐름을 직접 이해하고, 실제 회로에서 동작을 확인하여 결과를 검증함.
