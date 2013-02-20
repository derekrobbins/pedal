#include <avr/io.h>
#include <avr/interrupt.h>

const BOUNCETIME = 20000;
const STOREDTAPS = 4;

volatile int quarter 	 = 15000;
volatile int triplet 	 = 1;
volatile int dbltime 	 = 1; 
volatile int dblstate 	 = 0;
volatile int triplestate = 0;
volatile int active 	 = 0;
volatile int counter 	 = 0;
volatile int remainder 	 = 0;

void debounce();

void main(void)
{
	/*
	Setup timer0 as debounbe timer. Timer is set to roughly 2ms.
	*/
	OCR0B	|= BOUNCETIME;				// Set compare register B to 2ms
	TIMSK0	|= (1 << OCIE1B);			// Fire interrupter when timer0 reaches the time set in compare register B
	TCCR0B 	|= (1 << WGM12);			// Set timer 0 to "Clear Timer on Compare" Mode
	TCCR0B 	|= (1 << CS10);				// Start timer

	/*
	Setup timer1 as main timer to control signal output. This timer is based
	around the quarter note tempo tapped in via pin 2 of port D
	*/
	OCR1A 	 = quarter / (4 * dbltime * triplet);	// Set CTC compare value
	TCCR1A 	|= (1 << COM1A0);			// Set timer 1 compare output channel A in toggle mode
	TCCR1B 	|= ((1 << CS10) | (1 << CS11));		// Start timer at frequency/64
	TCCR1B 	|= (1 << WGM12);			// Set timer 1 to "Clear Timer on Compare" Mode

	/*
	Set up I/O pins
	*/
	// Port B
	DDRB 	|= (1 << 1);				// Set pin 1 of port B as output
	PORTB 	|= (1 << 0);				// Set pin 0 of port B to on state
	// Port D
	DDRD 	|= 0b11100011;				// Set pins 2, 3, and 4 of port D as input, everything else to output
	PORTD	|= (0 << 7);				// Set pin 7 of port D to off state

	EIMSK	|= (1 << INT0) | (1 << INT1);		// Enable INT0 and INT 1 (pins 2 and 3 of port D)
	EICRA	|= (1 << ISC00) | (1 << ISC10);		// Trigger on rising edge of INT0 and state change of INT 1
	PCMSK2  |= (1 << PCINT20);			// Enable interuptor on PCINT20 (pin 4 of port D)
	PCICR	|= (1 << PCIE2); 

	sei();						// Globally enable interrupts

	while (1);
}

// Double time switch interrupter for pin 4 of port D
ISR(PCINT2_vect)
{
	int newstate = PIND & (1 << PD4);
	if (dblstate != newstate) {
		if (newstate) {
			dbltime = 2;
		} else {
			dbltime = 1;
		}
		dblstate = newstate;
		OCR1A 	 = quarter / (2 * dbltime * triplet);	// Set CTC compare value
		TCNT1 	 = 0;
	}
}

// Triplet switch interrupter for pin 3 of port D
ISR(INT1_vect)
{
	int newstate = PIND & (1 << PD3);
	if (triplestate != newstate) {
		if (newstate) {
			triplet = 3;
		} else {
			triplet = 1;
		}
		triplestate = newstate;
		OCR1A 	    = quarter / (2 * dbltime * triplet);	// Set CTC compare value
		TCNT1       = 0;
	}
}

// Tap tempo interrupter for pin 2 of port D
ISR(INT0_vect)
{
	debounce();
}

// Debounce interruptor, fires BOUNCETIME cycles after initial tempo interrupter
ISR(TIMER0_COMPB_vect)
{
	counter++;
	if (!active || counter < 5) return;			// Don't run anything if 
	active = 0;
	if (PIND & (1 << PD2)) {
		quarter = remainder + counter * (64 * BOUNCETIME) / 2;
		counter = 0;
		OCR1A 	= quarter / (2 * dbltime * triplet);	// Set timing of signal killer
		TCNT1 	= 0;					// reset timer
		PORTD  ^= (1 << 7);				// Toggle test LED
		PORTB  |= (0 << 1);				// Set initial state of signal killer
	}
}

// Switches be sloppy
void debounce()
{
	active    = 1;		// Set active so the interrupt for timer0 knows its good to go 
	remainder = TCNT0;	// To be added to quarter
	TCNT0     = 0;		// reset timer0's counter so it fires 2ms from now
}

