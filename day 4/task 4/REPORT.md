# 4일차 과제 3 : PSD 센서 기반 거리 측정 시스템

> **광운대학교 로봇게임단 수습단원 교육**  
> **학과:** 전자융합공학과  
> **작성자:** 석주형  
> **제출일:** 2026년 8월 3일

---

## 1. 개요 (Overview)

 PSD 거리 센서의 아날로그 출력 전압을 ATmega128의 ADC로 읽고, 보정식을 사용하여 거리(cm)로 환산한 뒤 UART0으로 PC에 출력하는 프로그램을 구현하는 것이다. 최근 8개의 ADC 값을 이동평균 필터로 처리하여 순간적인 측정값 흔들림을 줄였으며, 원본값(RAW)과 필터 적용값(FILTERED)을 함께 출력한다. 측정 범위를 벗어난 ADC 값과 계산 결과는 정상 거리로 출력하지 않고 오류로 처리하였다.

### 핵심 목표

* PF1(ADC1)에서 PSD 센서의 아날로그 출력값 읽기
* 10비트 ADC 값을 거리(cm)로 환산하기
* UART0으로 측정 결과를 PC 터미널에 출력하기
* 8개 샘플 이동평균 필터로 측정값 흔들림 줄이기
* 센서 미연결, 포화, 측정 범위 이탈에 대한 예외 처리

---

## 2. 개발 환경 (Environment)

| 항목 | 내용 |
| :--- | :--- |
| **MCU** | ATmega128A, 16 MHz External Crystal |
| **IDE / Compiler** | Microchip Studio / AVR-GCC |
| **Programmer** | STK500 호환 USB ISP |
| **Terminal** | Tera Term 또는 Serial 통신 프로그램 |
| **언어** | C Language |

---

## 3. 하드웨어 구성 및 핀 맵 (Hardware Structure)

### Pin Configuration

```text
[ATmega128]                 [Target Component]
 PF1 (ADC1)          <-----   PSD Vo (analog output)
 PE1 (TXD0)          ----->   USB-Serial adapter RX
 GND                 <---->   PSD GND, adapter GND
 +5V                 ----->   PSD Vcc
```

### PSD 센서 연결

PSD 센서는 전원(Vcc), 접지(GND), 아날로그 출력(Vo)의 3개 선을 사용한다. Vo는 거리 변화에 따라 달라지는 전압을 출력하며, 이 값을 PF1의 ADC1 채널로 입력한다. UART 출력이 정상적으로 보이려면 ATmega128과 USB-Serial 어댑터의 GND를 반드시 공통으로 연결해야 한다.

```text
[PSD 3-pin connector]
 Vo   -----> PF1 (ADC1)
 GND  -----> GND
 Vcc  -----> +5V
```

---

## 4. 프로젝트 구조 (Directory Structure)

빌드 시 `math.h`의 `powf()`를 사용하므로 AVR-GCC 환경에서 수학 라이브러리 링크 옵션이 필요한 경우 `-lm`을 추가한다.

```text
avr-gcc -mmcu=atmega128 -Os -Wall -std=gnu99 -o firmware.elf PSD_Distance.c -lm
avr-objcopy -O ihex -R .eeprom firmware.elf firmware.hex
```

---

## 5. 핵심 코드 및 레지스터 설정 (Key Implementation)

### UART0 초기화

```c
unsigned int ubrr = (F_CPU / 16 / baud) - 1;

UBRR0H = (unsigned char)(ubrr >> 8);
UBRR0L = (unsigned char)ubrr;
UCSR0B = (1 << RXEN0) | (1 << TXEN0);
UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
```

`F_CPU`가 16 MHz이고 통신 속도가 9600 bps일 때 UBRR 값은 약 103이다. `UCSR0B`에서 송신기(TXEN0)와 수신기(RXEN0)를 켜고, `UCSR0C`에서 데이터 비트를 8비트로 설정하였다. 따라서 터미널은 **9600 bps, 8N1**로 설정한다.

### ADC 초기화 및 값 읽기

```c
ADMUX = (1 << REFS0);
ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
```

`REFS0`을 1로 설정하여 AVCC(5V)를 ADC 기준 전압으로 사용한다. 분주비 128을 사용하므로 ADC 클록은 `16 MHz / 128 = 125 kHz`가 된다. 이는 ATmega128 ADC의 권장 동작 범위에 맞는 값이다.

```c
ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);
ADCSRA |= (1 << ADSC);
while (ADCSRA & (1 << ADSC));
return ADC;
```

입력 채널 번호 1을 ADMUX에 넣어 ADC1(PF1)을 선택한다. `ADSC`를 1로 설정하여 변환을 시작하고, 변환이 끝나 `ADSC`가 0이 될 때까지 기다린 후 10비트 ADC 결과(0~1023)를 반환한다.

### 거리 환산 방식

PSD 센서의 출력은 거리와 선형 관계가 아니므로 단순 비례식으로 변환할 수 없다. 이 프로그램에서는 실측 보정값을 기반으로 한 아래 거듭제곱 근사식을 사용한다.

```c
float distance = DIST_COEF_A * powf((float)adc_value, DIST_COEF_B);
```

```text
distance(cm) = 2670.4 × ADC^(-0.769)
```

ADC 값이 커질수록 계산된 거리는 작아진다. 이는 일반적인 PSD 센서가 가까운 물체에서 더 높은 출력 전압을 내는 특성과 일치한다.

### 이동평균 필터

```c
filter_sum -= filter_buf[filter_index];
filter_buf[filter_index] = new_sample;
filter_sum += new_sample;

return (unsigned int)(filter_sum / count);
```

`filter_buf` 배열에는 최근 ADC 측정값을 최대 8개 저장한다. 새 값이 들어오면 가장 오래된 값을 합계에서 빼고 새 값을 더한 뒤, 합계를 저장된 개수로 나누어 평균값을 구한다. 따라서 매번 8개 값을 전부 더하지 않아도 되며, 센서 노이즈로 인한 짧은 변화가 줄어든다.

초기에는 배열이 아직 8개로 채워지지 않았으므로 실제 저장된 값 개수만큼 나눈다. 8개가 모두 채워진 뒤에는 새 값이 가장 오래된 값을 순서대로 덮어쓰는 원형 방식으로 동작한다.

raw와 filtered를 비교해보면 raw값은 크게 변하는 반면 filtered값은 비교적 완만하게 변한다
### 예외 처리

```c
if (adc_value < ADC_MIN_VALID || adc_value > ADC_MAX_VALID)
{
    return -1.0f;
}

if (distance < DIST_MIN_CM || distance > DIST_MAX_CM)
{
    return -1.0f;
}
```

| 조건 | 판정 | 출력 |
| :--- | :--- | :--- |
| ADC < 100 | 센서 미연결, 원거리 또는 비정상 입력 가능성 | `Invalid PSD reading` |
| ADC > 900 | 너무 가까움 또는 ADC 포화 가능성 | `Invalid PSD reading` |
| 거리 < 15 cm 또는 > 60 cm | 보정 범위 밖 | `Invalid PSD reading` |
| 그 외 | 정상 측정 | `Distance: n.n cm` |

PSD 센서는 최소 측정 거리보다 더 가까운 구간에서 출력 특성이 단순하지 않을 수 있으므로, 계산 결과가 범위를 벗어나면 거리값으로 사용하지 않는다.

### 측정 주기

```c
UART0_print(buf);
_delay_ms(MEASURE_PERIOD_MS);
```

`MEASURE_PERIOD_MS`를 200으로 설정하여 약 200 ms마다 ADC를 한 번 읽고 결과를 출력한다. 너무 빠른 출력으로 터미널이 읽기 어려워지는 것을 막고, 거리 변화도 충분히 확인할 수 있는 주기이다. 필터는 이 주기마다 한 번 갱신된다.

---

## 6. 동작 설명 및 결과 (Results)

### 동작 시나리오

1. UART0을 9600 bps로 초기화한다.
2. ADC의 기준 전압을 AVCC로 설정하고 ADC를 활성화한다.
3. PF1(ADC1)의 PSD 출력 전압을 10비트 RAW 값으로 변환한다.
4. 최근 8개의 값으로 이동평균을 계산하여 FILTERED 값을 만든다.
5. FILTERED 값이 유효한지 검사한다.
6. 정상 값이면 보정식으로 거리(cm)를 계산한다.
7. 계산된 거리가 15~60 cm 범위인지 다시 검사한다.
8. RAW 값, FILTERED 값, 거리 또는 오류 메시지를 UART0으로 출력한다.
8. 200 ms 대기 후 반복한다.

### 터미널 출력 예시

```text
=== PSD Raw + Filtered Measurement Start ===
RAW: 356 | FILTERED: 356 | DISTANCE: 29.0cm
RAW: 351 | FILTERED: 353 | DISTANCE: 29.2cm
RAW:  42 | FILTERED:  42 | DISTANCE: [ERROR]
```

### 확인할 사항

* 값이 항상 `1023`이면 PSD 출력선이 5V에 연결되었거나 ADC 입력 배선을 먼저 확인한다.
* 값이 계속 `0`이면 GND 연결, PSD 전원, Vo 연결 상태를 확인한다.
* 터미널에 글자가 깨지면 통신 속도를 9600 bps로 맞추고 TXD0(PE1)과 어댑터 RX의 연결 및 공통 GND를 확인한다.
* 실제 거리가 계산값과 차이 난다면 `DIST_COEF_A`, `DIST_COEF_B`는 사용한 PSD 모델과 실측값에 맞춰 다시 보정해야 한다.

---

https://drive.google.com/file/d/1k7K8Zm1Lu82nYAfTUFuQT_lehkb1Tnhw/view?usp=sharing
