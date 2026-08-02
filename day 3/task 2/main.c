    #define F_CPU 16000000UL

   #include <avr/io.h>
   #include <util/delay.h>

   #define NO_LED 255

   void uart_putch(unsigned char data)//문자 하나 보내는 함수
   {
      while (!(UCSR0A & (1 << UDRE0)))//비어있는지 확인
      { }
     UDR0 = data;
   }

   void uart_string(const char *text)//문자열 전체를 보낸
   {
      while (*text != '\0')
      {
         uart_putch(*text);
         text++;
      }
   }

   void led_on(unsigned char number)//넘에 해당되는 led키
   {
      PORTA = (unsigned char)~(1 << number);
   }

   int main(void)
   {
      unsigned char receivedData;
      unsigned char currentLed = NO_LED;
      unsigned char switchOld = 1;

      DDRA = 0xFF;//회로에 맞게 설정
      PORTA = 0xFF;

      DDRC &= ~(1 << PC0);
      PORTC |= (1 << PC0);

      UBRR0L = 16;//통신속도 정하기
      UBRR0H = 0;

      UCSR0A = 0x20;//송신 수신 둘다 키기
      UCSR0B = 0x18;
      UCSR0C = 0x06;// 비트설

      DDRE = 0x02;//TXD0를 출력으로 설정해 컴퓨터로 데이터를 보냄

      uart_string("READY\r\n");

      while (1)
      {
         if (!(PINC & (1 << PC0)) && switchOld == 1)//pc0눌렀을때
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

         if (PINC & (1 << PC0))
         {
            switchOld = 1;
         }

         if (UCSR0A & (1 << RXC0))
         {
            receivedData = UDR0;

            if (receivedData >= '0' && receivedData <= '7')
            {
               currentLed = receivedData - '0';//0부터 7까지 해당 번호 led키기
               led_on(currentLed);

               uart_putch(receivedData);
               uart_string(" LED ON\r\n");
            }
            else if (receivedData == '8')//8받으면 한칸씩 이동시키는 코드
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
         }
      }
   }
