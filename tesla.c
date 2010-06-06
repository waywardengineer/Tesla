// avrdude -c usbtiny -p atmega328p -U flash:w:avr/tesla/default/tesla.hex




#include <avr/io.h>
int main(void)
{
	#define minaudiowavelength 125 // =(16mhz/64)/2000hz
	#define maxpulselength 500 //10% of 50 hz
	#define duty 9 //pct duty cycle
	#define delaytime 10
	unsigned int audioontime[3] = {0,0,0};
	unsigned int dutyofftime[3] = {0,0,0};
	int dutycyclecount[3] = {0,0,0};
	int audiopulse = 0;
	int audioon = 0;
	unsigned int dutypulselength;
	unsigned int pinontime[3] = {0,0,0};	
	int pinbyte;
	int i;
	unsigned int steadyontime = 0;
	DDRC = 0x00;
	DDRB = 0xff;
	PORTB = 0x00;
	TCCR1B |= ((1 << CS10) | (1 << CS11));
	while (1){
		while (PINC & 0b00001000){
			if (TCNT1 > 64000) {//prevent overflow 
				TCNT1 -= 54000;
				for (i=0;i<3;i++){
					audioontime[i] = audioontime[i] < 54000 ? 0:audioontime[i]-54000;
					dutyofftime[i] -=54000;
					pinontime[i] = pinontime[i] < 54000 ? 0:pinontime[i]-54000;
				}
			}
			for (i=0;i<3;i++){
				pinbyte=1<<i;
				if ((~audioon) & PINC & pinbyte){ //audio turning on
					if ((audiopulse & pinbyte) && TCNT1 > (audioontime[i]+delaytime)) {
						audioon |= pinbyte;
						dutypulselength = (TCNT1 - pinontime[i])/10;
						dutypulselength = dutypulselength>maxpulselength?maxpulselength:dutypulselength;
						dutyofftime[i] = TCNT1 + dutypulselength;
						if (TCNT1 > (pinontime[i] + minaudiowavelength)) {
							PORTB |= pinbyte;
							pinontime[i] = TCNT1;
							dutycyclecount[i] = 1;
						}			
					}
					if ((~audiopulse) & pinbyte) {
						audiopulse |= pinbyte;
						audioontime[i] = TCNT1;
					}

				}
				if ((PINB & pinbyte) && (TCNT1 > dutyofftime[i]) && (dutycyclecount[i] < 1)){
					PORTB &= ~pinbyte;
				}
				if (dutycyclecount[i]>0){
					dutycyclecount[i]--;
				}
			}
			audiopulse &= (PINC & 0b00000111);
			audioon	&= (PINC & 0b00000111);					
		}
		while (PINC & 0b00010000){
			if (TCNT1 > 62000) {//prevent overflow 
				TCNT1 -= 62000;
				steadyontime -= 62000;
				dutyofftime[0] -= 62000;
			}
			if (TCNT1 > steadyontime) {
				steadyontime = TCNT1+2500;
				dutyofftime[0] = TCNT1+250;
				PORTB |= 0b00000111;
			}
			if (TCNT1 > dutyofftime[0]) {
				PORTB &= 0b11111000;
			}
			
		}
	}
	return 1;
}
