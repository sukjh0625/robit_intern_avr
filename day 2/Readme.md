ADC이란?

아날로그 값을 디지털 숫자로 바꾸는 장치이다.

ATMEGA에서는 10비트이기 때문에 결과가 0부터 1023 까지 나온다. 따라서 1024단계의 전압을 구분하는 것이다. 예를들어 ADC값이 2.5V라면 ADC값이 약 512 이다.


ADC입력핀

ADC입력핀으로는 PFO부터 PF7까지 쓰인다. 각자의 핀마다 채널이 있다. 


SINGLE ENDED입력 

3VRK 들어오고 GND가 0이라면 ADC는 3V를 측정한다
 

Sample and Hold

Sample and Hold회로가 있는데 ADC가 변환하는동안 입력 전압이 변하면 측정하기 힘들다 그래서 변환 시작할때 전압을 잠깐 저장하고 그 저장된 전압을 기준으로 디지털 값을 계산한다.

AVCC와 AREF

AVCC는 전원을 공급하는 핀이다 일반적으로 5V에 연결한다

ARFF는 전압을 계산할 떄 사용하는 기준전압 핀이다. 기준전압으로는 외부 ARFF전압 AVCC 내부 2.56V 중 하나를 선택할 수 있다


ADC내부 구조
1. 기준전압선택 
2. ADC입력채널 ADC0~ADC7중 측정할 핀을 하나 고르고 MUX4~MUX0비트를 이용한다
3. 단일 입력과 차동 입력중 하나를 사용한다
4. ADC활성화하고 변환한다
5. PRESCULAR CPU는 16메가헤레츠로 작동하지만 ADC는 느리므로 CPU클럭을 나눠서 ADC에 전달해야한다
6. 선택된 입력전압은 SAMPLE&HOLD회로에 저장된다
7. 변환결과를 ADCL ADCH에 저장한다 UNIT8_T UNIT16_T를 이용한다

노이즈 

ADC값이 실제 전압은 거의 같은데 계속 조금씩 바뀌는 현상

해결방법
ADC선 짧게 연결
ARFF에 콘덴서 연결
AVCC전원 안정화
여러번 측정해서 평균내기
변환중 디지털 출력 바꾸지 않기

최소 감지 전압
ADC가 감지할 수 있는 가장 작은 전압 변화 단위는 1LSB이다

과제 2 https://drive.google.com/file/d/1hEbJroXGBGcaOtHVkSFnHujRlhCc5oNv/view?usp=sharing
과제 3 https://drive.google.com/file/d/1ADQO4VIBSb8wijyMJI_YJwib00B-1CM6/view?usp=sharing
