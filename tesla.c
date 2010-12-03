// avrdude -c usbtiny -p atmega328p -U flash:w:avr/tesla/default/tesla.hex

#include <avr/io.h>
int main(void)
{
	#define minaudiowavelength 125 // =(16mhz/64)/2000hz
	#define maxpulselength 500 //10% of 50 hz
	#define duty 10 //pct duty cycle
	#define delaytime 10 //This has to do with preventing shorter imput wavelengths from tricking it into thinking the input is just always on
	unsigned int audioontime[3] = {0,0,0};// how long between instances of the input going high
	unsigned int dutyofftime[3] = {0,0,0};// what TCNT1 time we need to turn off at
	int dutycyclecount[3] = {0,0,0};// how many cycles it's been on so we can make sure it's at least one
	int audiopulse = 0;//I forget about what these do exactly(why you should comment your code when you write it!). Something to do with the input state.
	int audioon = 0;// has to do with the output state
	unsigned int dutypulselength;//how long the next on pulse will be
	unsigned int pinontime[3] = {0,0,0};//The wavelengths of inputs that we haven't skipped for being over the freq limit	
	int pinbyte;//byte containing the pin we're working on. Audio input and output are both the first 3 pins of their ports.
	int i;
	unsigned int steadyontime = 0;//keeps track of wavelength in steady mode
	DDRC = 0x00;
	DDRB = 0xff;
	PORTB = 0x00;
	TCCR1B |= ((1 << CS10) | (1 << CS11));
	while (1){
		while (PINC & 0b00001000){//audio modulator mode
			if (TCNT1 > 64000) {//prevent overflow 
				TCNT1 -= 54000;//can't go to zero cause we want to leave room for audioontimes that happened a little while ago
				for (i=0;i<3;i++){
					audioontime[i] = audioontime[i] < 54000 ? 0:audioontime[i]-54000;//we dont care about wavelengths more than 10000
					dutyofftime[i] -=54000;
					pinontime[i] = pinontime[i] < 54000 ? 0:pinontime[i]-54000;
				}
			}
			for (i=0;i<3;i++){
				pinbyte=1<<i;
				if ((~audioon) & PINC & pinbyte){ //audio input just went high
					if ((audiopulse & pinbyte) && TCNT1 > (audioontime[i]+delaytime)) {//Wavelength of last pulse is not too short to ignore
						audioon |= pinbyte;
						dutypulselength = (TCNT1 - pinontime[i])/duty;
						dutypulselength = dutypulselength>maxpulselength?maxpulselength:dutypulselength;
						dutyofftime[i] = TCNT1 + dutypulselength;
						if (TCNT1 > (pinontime[i] + minaudiowavelength)) {//OK to turn on based on max frequency limit
							PORTB |= pinbyte;
							pinontime[i] = TCNT1;
							dutycyclecount[i] = 1;
						}			
					}
					if ((~audiopulse) & pinbyte) {//mark the start of this input pulse
						audiopulse |= pinbyte;
						audioontime[i] = TCNT1;
					}

				}
				if ((PINB & pinbyte) && (TCNT1 > dutyofftime[i]) && (dutycyclecount[i] < 1)){//on time ran out, turn off
					PORTB &= ~pinbyte;
				}
				if (dutycyclecount[i]>0){
					dutycyclecount[i]--;
				}
			}
			audiopulse &= (PINC & 0b00000111);
			audioon	&= (PINC & 0b00000111);					
		}
		while (PINC & 0b00010000){//constant tone mode(currently a 100hz tone)
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
