# 3일차 과제 : UART 기반 서보모터 각도 제어

> **광운대학교 로봇게임단 수습단원 교육**  
> **학과:** 전자융합공학과  
> **작성자:** 석주형  
> **제출일:** 2026년 8월 3일

---

## 1. 개요 (Overview)

 PC 터미널에서 UART0으로 서보모터의 목표 각도(0~180)를 입력하면, ATmega128이 입력값을 받아 PWM 신호를 만들고 서보모터를 해당 각도로 이동시키는 프로그램을 구현하는 것이다. 시작 시 서보를 원점인 90도로 이동시키며, 허용 범위를 벗어난 값은 모터를 움직이지 않고 경고 메시지를 출력하도록 예외 처리하였다.

### 핵심 목표

* UART0으로 목표 각도 문자열을 입력받기
* 입력 문자열을 정수 각도로 변환하기
* Timer1의 PWM 출력으로 서보모터 제어하기
* 0~180도 범위 밖의 입력값 예외 처리하기
* 현재 이동한 각도를 UART0으로 출력하기

---

## 2. 개발 환경 (Environment)

| 항목 | 내용 |
| :--- | :--- |
| **MCU** | ATmega128A, 16 MHz External Crystal |
| **IDE / Compiler** | Microchip Studio / AVR-GCC |
| **Programmer** | STK500 호환 USB ISP |
| **Terminal** | Tera Term 또는 Serial 통신 프로그램 |
| **언어** | C Language |

터미널 통신 설정은 **9600 bps, 8 data bits, no parity, 1 stop bit (8N1)**로 설정한다.

---

## 3. 하드웨어 구성 및 핀 맵 (Hardware Structure)

### Pin Configuration

```text
[ATmega128]                 [Target Component]
 PE0 (RXD0)          <-----   USB-Serial adapter TX
 PE1 (TXD0)          ----->   USB-Serial adapter RX
 PB7 (OC1C)          ----->   Servo signal
 +5V                 ----->   Servo Vcc
 GND                 <---->   Servo GND, adapter GND
```

### 연결 시 주의사항

서보모터는 신호선, 전원선, GND선이 필요하다. PB7은 Timer1의 비교 출력 채널 C인 `OC1C` 핀이므로 서보의 신호선에 연결한다. 또한 ATmega128, USB-Serial 어댑터, 서보모터의 GND는 반드시 공통으로 연결해야 한다. 전류가 부족하면 서보가 떨리거나 보드가 재시작될 수 있으므로, 서보의 전원은 충분한 전류를 공급할 수 있어야 한다.

---

## 4. 핵심 코드 및 레지스터 설정 (Key Implementation)

### UART0 초기화

```c
unsigned int ubrr = (F_CPU / 16 / baud) - 1;

UBRR0H = (unsigned char)(ubrr >> 8);
UBRR0L = (unsigned char)(ubrr);
UCSR0B = (1 << RXEN0) | (1 << TXEN0);
UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
```

16 MHz에서 9600 bps를 사용하면 UBRR 값은 103이다. `RXEN0`과 `TXEN0`으로 UART0 수신과 송신을 켜고, `UCSZ01`, `UCSZ00`을 1로 설정하여 8비트 데이터를 사용한다.

### 한 줄 입력 및 삭제 처리

```c
if (c == '\r' || c == '\n')
{
    UART0_print("\r\n");
    break;
}

if ((c == 0x08 || c == 0x7F) && idx > 0)
{
    idx--;
    UART0_print("\b \b");
    continue;
}
```

Enter를 누르면 입력을 끝내고 문자열 끝에 `\0`을 넣는다. Backspace(0x08) 또는 Delete(0x7F)를 입력하면 한 글자를 지우도록 처리하였다. `RX_BUF_SIZE`가 16이므로 너무 긴 입력이 배열을 넘지 않도록 최대 15글자까지만 저장한다.

### Timer1 PWM 생성

```c
TCCR1A = (1 << COM1C1) | (1 << WGM11);
TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

ICR1 = PWM_TOP;
```

`WGM13:WGM10 = 1110`으로 Timer1 Fast PWM Mode 14를 사용한다. 분주비는 8이므로 Timer1의 한 카운트 시간은 다음과 같다.

```text
16 MHz / 8 = 2 MHz
1 count = 0.5 us
ICR1 = 39999 -> 40000 count x 0.5 us = 20 ms
```

따라서 PWM 주기는 약 20 ms(50 Hz)가 되며, 일반적인 서보모터 제어 주기와 같다. `COM1C1`을 1로 설정하여 PB7(OC1C)에서 PWM 파형이 출력된다.

### 각도를 펄스 폭으로 변환

```c
unsigned int ticks = SERVO_MIN_TICKS +
    (unsigned long)(SERVO_MAX_TICKS - SERVO_MIN_TICKS) * angle /
    (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE);

OCR1C = ticks;
```

서보는 PWM의 듀티비보다 High 펄스 폭으로 위치를 판단한다. 이 코드에서는 0도에 1 ms, 180도에 2 ms 펄스를 사용한다. 한 카운트가 0.5 us이므로 1 ms는 2000, 2 ms는 4000 카운트이다.

| 각도 | OCR1C 값 | High 펄스 폭 |
| :---: | :---: | :---: |
| 0도 | 2000 | 1.0 ms |
| 90도 | 3000 | 1.5 ms |
| 180도 | 4000 | 2.0 ms |

입력 각도는 선형 비례식으로 `OCR1C` 값으로 변환한다. `unsigned long`으로 곱셈하여 계산 과정에서 값이 넘치는 것을 방지하였다.

### 입력 범위 예외 처리

```c
if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE)
{
    UART0_print("[WARNING] Angle out of range (0~180). Servo not moved.\r\n");
    continue;
}
```

`atoi()`로 바꾼 결과가 0~180을 벗어나면 PWM 비교값을 바꾸지 않는다. 따라서 잘못된 각도 입력이 들어와도 서보는 이전 위치를 유지한다. 빈 줄을 입력한 경우도 별도로 검사하여 동작하지 않는다.

---

## 5. 동작 설명 및 결과 (Results)

### 동작 시나리오

1. UART0을 9600 bps로 초기화한다.
2. Timer1 Fast PWM을 설정하고 PB7(OC1C)을 PWM 출력으로 만든다.
3. 서보모터를 원점인 90도로 이동한다.
4. 터미널에서 각도(0~180)를 입력받는다.
5. 빈 입력과 범위 밖 입력을 검사한다.
6. 정상 입력이면 각도를 1~2 ms 펄스 폭으로 바꾸고 `OCR1C`에 저장한다.
7. 서보를 이동시킨 뒤 이동한 각도를 터미널에 출력한다.

### 터미널 출력 예시

```text
=== Servo Control Ready (origin: 90 deg) ===
Enter target angle (0~180) and press Enter:
> 0
Moved to 0 degrees.
> 90
Moved to 90 degrees.
> 180
Moved to 180 degrees.
> 200
[WARNING] Angle out of range (0~180). Servo not moved.
```

### 확인할 사항

* 터미널 글자가 깨지면 9600 bps와 PE0/PE1 연결을 확인한다.
* 서보가 전혀 움직이지 않으면 신호선이 PB7(OC1C)에 연결되었는지 확인한다.
* 서보가 떨리거나 보드가 꺼지면 서보 전원 공급의 전류 용량과 GND 공통 연결을 확인한다.
* 서보마다 실제 동작 가능한 펄스 범위가 조금씩 다르므로 끝 각도에서 과도하게 소리가 나면 `SERVO_MIN_TICKS`, `SERVO_MAX_TICKS` 값을 줄여 조정한다.

---

## 6. ai사용

보고서 틀을 잡는데 도움을 받음. 시리얼 입력값의 범위 검사와 코드 오류를 점검하는 데 참고하였다. 또한 서보모터 각도에 따른 OCR1C 값 계산 방법을 확인하는데 도움을 받았따.
