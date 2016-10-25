#include <msp430.h>

#define FALSE 0
#define TRUE 1

#define P1_0 BIT0
#define P1_1 BIT1
#define P1_3 BIT3
#define P1_4 BIT4
#define P1_5 BIT5
#define P1_6 BIT6

#define TRIGGER_PIN P1_5
#define PREFLASH_SELECTOR_PIN P1_3
#define LIGHT_SENSOR_PIN P1_4
#define TTL_SELECTOR_PIN P1_1
#define LED1 P1_0
#define LED2 P1_6

#define TTL_WAIT_COUNTER TA0R
#define TTL_WAIT_MIN 45000		//90ms/2
#define TTL_WAIT_MAX 55000		//110ms/2
#define TTL_WAIT_TARGET 49200	//98.4ms/2

#define PREFLASH_TIMEOUT 65535	//131ms

int preflash = 0;
int hasToTriggerFlash = 0;
int waitTimerRunning = FALSE;

inline void led1Off() {
	P1OUT &= ~LED1;
}

inline void led1On() {
	P1OUT |= LED1;
}

inline void led1Toggle() {
	P1OUT ^= LED1;
}

inline void led1Blink() {
	led1On();
	__delay_cycles(10000);
	led1Off();
}

inline void led2Off() {
	P1OUT &= ~LED2;
}

inline void led2On() {
	P1OUT |= LED2;
}

inline void led2Toggle() {
	P1OUT ^= LED2;
}

inline void led2Blink() {
	led2On();
	__delay_cycles(10000);
	led2Off();
}

inline void clearPort1Interrupt() {
	P1IFG &= ~LIGHT_SENSOR_PIN;
}

int main(void) {
    WDTCTL = WDTPW + WDTHOLD; // Stop Watchdog Timer
    P1SEL &= ~(TRIGGER_PIN + PREFLASH_SELECTOR_PIN + LIGHT_SENSOR_PIN + TTL_SELECTOR_PIN + LED2 + LED1);
    //output
    P1DIR |=  TRIGGER_PIN;				// Pin 1.0 as output
    P1DIR |= LED1;
    P1DIR |= LED2;
    //input
    P1DIR &=  ~LIGHT_SENSOR_PIN;
    P1DIR &=  ~PREFLASH_SELECTOR_PIN;
    P1OUT &=  ~TRIGGER_PIN;
    P1IE |= LIGHT_SENSOR_PIN;
    __enable_interrupt();
    while (TRUE) {
    	if (waitTimerRunning) {
    		led1Blink();
    		LPM1;
    	} else {
    		led2Blink();
    		LPM4;
    	}
    }
    return 0;
}

inline void triggerFlash() {
	P1OUT |= TRIGGER_PIN;
	__delay_cycles(10000);
	P1OUT &= ~TRIGGER_PIN;
	hasToTriggerFlash = FALSE;
}

inline void startWaitTimer() {
	waitTimerRunning = TRUE;
	TTL_WAIT_COUNTER = 0;
	TACTL = TASSEL_2 + MC_2 + TAIE + ID_1;
}

inline void stopWaitTimer() {
	waitTimerRunning = FALSE;
	TACTL = TASSEL_2 + MC_0 + TAIE + ID_1;
}

inline void handlePreflash() {
	startWaitTimer();
	preflash++;
	if (preflash > 1) {
		hasToTriggerFlash = 1;
		preflash = 0;
	}
}

inline void handleTTLSlave() {
	if (waitTimerRunning == FALSE) {
		startWaitTimer();
	} else if (waitTimerRunning == TRUE && TTL_WAIT_COUNTER > TTL_WAIT_MIN && TTL_WAIT_COUNTER <= TTL_WAIT_MAX) {
		stopWaitTimer();
		hasToTriggerFlash = TRUE;
	} else if (waitTimerRunning == TRUE && TTL_WAIT_COUNTER > TTL_WAIT_MAX) {
		stopWaitTimer();
	}
}

__attribute__((interrupt(PORT1_VECTOR))) void flashDetected(void) {
	if (P1IFG & LIGHT_SENSOR_PIN) {
        if (P1IN & PREFLASH_SELECTOR_PIN) {
			handlePreflash();
			LPM4_EXIT;
        } else if (P1IN & TTL_SELECTOR_PIN) {
			handleTTLSlave();
        	LPM4_EXIT;
        } else {
			hasToTriggerFlash = TRUE;  
        }
        if (hasToTriggerFlash) {
			triggerFlash();
        }
        clearPort1Interrupt();
    }
}

__attribute__((interrupt(TIMERA1_VECTOR))) void waitTimerTimeout(void) {
	switch(TAIV) {
		case TAIV_TAIFG: 
   			stopWaitTimer();
   			preflash = 0;
   			LPM4_EXIT;
        	break;
	}
}