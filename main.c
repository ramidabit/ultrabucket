/*
 * ECE 153B - Winter 2021
 *
 * Names: Rami Dabit, Kyle Kam
 * Section: Wednesday 7:00-9:50pm
 */

#include "LCD.h"
#include "L3GD20.h"
#include "UART.h"
#include "SPI.h"
#include "SysClock.h"
#include "SysTimer.h"
#include "stm32l476xx.h"
#include <stdio.h>

// Threshold for toggling functionality using gyroscope
int THRESHOLD = 300;

typedef struct {
	float x; 
	float y; 
	float z; 
} L3GD20_Data_t;

L3GD20_Data_t currentSpeed;

uint32_t volatile currentValue_1 = 0;
uint32_t volatile currentValue_2 = 0;
uint32_t volatile currentValue_3 = 0;
uint32_t volatile lastValue_1 = 0;
uint32_t volatile lastValue_2 = 0;
uint32_t volatile lastValue_3 = 0;
uint32_t volatile overflowCount_1 = 0;
uint32_t volatile overflowCount_2 = 0;
uint32_t volatile overflowCount_3 = 0;
uint32_t volatile timeInterval_1 = 0;
uint32_t volatile timeInterval_2 = 0;
uint32_t volatile timeInterval_3 = 0;

void Trigger_Setup() {
	// Enable GPIO port E clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	// Configure PE11 as alternative function (10)
	GPIOE->MODER &= ~GPIO_MODER_MODE11;
	GPIOE->MODER |= GPIO_MODER_MODE11_1;
	// Configure and select alternative function for PE11 (AF1)
	GPIOE->AFR[1] &= ~GPIO_AFRL_AFRL3;
	GPIOE->AFR[1] |= (0x1 << 3*4);
	// Configure PE11 as no pull-up, no pull-down (00)
	GPIOE->PUPDR &= ~GPIO_PUPDR_PUPD11;
	// Configure PE11 output type as push-pull (0)
	GPIOE->OTYPER &= ~GPIO_OTYPER_OT11;
	// Configure PE11 output speed as very high (11)
	GPIOE->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR11;

	// Enable TIM1 clock
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	// Set prescaler value to 15
	TIM1->PSC &= ~TIM_PSC_PSC;
	TIM1->PSC |= 0xF;
	// Enable preload for TIM1 CH 2
	TIM1->CR1 |= TIM_CR1_ARPE;
	// Set auto-reload value to 20
	TIM1->ARR &= ~TIM_ARR_ARR;
	TIM1->ARR |= 0x14;
	// Set capture and compare value to 10
	TIM1->CCR2 &= ~TIM_CCR2_CCR2;
	TIM1->CCR2 |= 0xA;
	// Configure channel to be used in output compare mode
	// Clear the output compare mode bits for channel 2
	TIM1->CCMR1 &= ~TIM_CCMR1_OC2M;
	// Set output control mode bits to PWM mode 1
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_1;
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_2;
	// Enable output compare preload
	TIM1->CCMR1 |= TIM_CCMR1_OC2PE;
	// Enable channel 2 output
	TIM1->CCER |= TIM_CCER_CC2E;
	// Enable main output and off-state selection
	TIM1->BDTR |= TIM_BDTR_MOE | TIM_BDTR_OSSR;
	// Enable update generation
	TIM1->EGR |= 0x1;
	// Enable update interrupt for timer 4
	TIM1->DIER |= TIM_DIER_UIE;
	// Clear update interrupt flag
	TIM1->SR &= ~TIM_SR_UIF;
	// Set direction such that timer counts up
	TIM1->CR1 &= ~TIM_CR1_DIR;
	// Enable the counter
	TIM1->CR1 |= 0x1;
}

void Ch1_Input_Capture_Setup() {
	// Enable GPIO port A clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	// Configure PA0 as alternative function (10)
	GPIOA->MODER &= ~GPIO_MODER_MODE0;
	GPIOA->MODER |= GPIO_MODER_MODE0_1;
	// Configure and select alternative function for PA0 (AF1)
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL0;
	GPIOA->AFR[0] |= 0x1;
	// Configure PA0 as no pull-up, no pull-down (00)
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;

	// Enable TIM2 clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	// Set prescaler value to 15
	TIM2->PSC &= ~TIM_PSC_PSC;
	TIM2->PSC |= 0xF;
	// Enable preload for timer 2
	TIM2->CR1 |= TIM_CR1_ARPE;
	// Set auto-reload value to max
	TIM2->ARR |= TIM_ARR_ARR;
	// Configure channel to be used in input capture mode
	// Clear capture and compare mode register
	TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;
	// Set to capture input on timer channel 1
	TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;
	// Enable input capture on both rising and falling edges
	TIM2->CCER |= TIM_CCER_CC1E;
	TIM2->CCER |= TIM_CCER_CC1P;
	TIM2->CCER |= TIM_CCER_CC1NP;
	// Enable interrupts for timer 2
	TIM2->DIER |= TIM_DIER_CC1IE;
	// Enable DMA requests for timer 2
	TIM2->DIER |= TIM_DIER_CC1DE;
	// Enable update interrupt for timer 2
	TIM2->DIER |= TIM_DIER_UIE;
	// Enable update generation
	TIM2->EGR |= 0x1;
	// Clear update interrupt flag
	TIM2->SR &= ~TIM_SR_UIF;
	// Set direction such that timer counts up
	TIM2->CR1 &= ~TIM_CR1_DIR;
	// Enable the counter
	TIM2->CR1 |= 0x1;

	// Enable TIM4_IRQn interrupt in NVIC and set priority to 2
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 2);
}

void Ch2_Input_Capture_Setup() {
	// Please note: Many steps are repeated in case Ch2 is used on its own

	// Enable GPIO port A clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	// Configure PA1 as alternative function (10)
	GPIOA->MODER &= ~GPIO_MODER_MODE1;
	GPIOA->MODER |= GPIO_MODER_MODE1_1;
	// Configure and select alternative function for PA1 (AF1)
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL1;
	GPIOA->AFR[0] |= (0x1 << 1*4);
	// Configure PA1 as no pull-up, no pull-down (00)
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD1;

	// Enable TIM2 clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	// Set prescaler value to 15
	TIM2->PSC &= ~TIM_PSC_PSC;
	TIM2->PSC |= 0xF;
	// Enable preload for timer 2
	TIM2->CR1 |= TIM_CR1_ARPE;
	// Set auto-reload value to max
	TIM2->ARR |= TIM_ARR_ARR;
	// Configure channel to be used in input capture mode
	// Clear capture and compare mode register
	TIM2->CCMR1 &= ~TIM_CCMR1_CC2S;
	// Set to capture input on timer channel 2
	TIM2->CCMR1 |= TIM_CCMR1_CC2S_0;
	// Enable input capture on both rising and falling edges
	TIM2->CCER |= TIM_CCER_CC2E;
	TIM2->CCER |= TIM_CCER_CC2P;
	TIM2->CCER |= TIM_CCER_CC2NP;
	// Enable interrupts for timer 2
	TIM2->DIER |= TIM_DIER_CC2IE;
	// Enable DMA requests for timer 2
	TIM2->DIER |= TIM_DIER_CC2DE;
	// Enable update interrupt for timer 2
	TIM2->DIER |= TIM_DIER_UIE;
	// Enable update generation
	TIM2->EGR |= 0x1;
	// Clear update interrupt flag
	TIM2->SR &= ~TIM_SR_UIF;
	// Set direction such that timer counts up
	TIM2->CR1 &= ~TIM_CR1_DIR;
	// Enable the counter
	TIM2->CR1 |= 0x1;

	// Enable TIM4_IRQn interrupt in NVIC and set priority to 2
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 2);
}

void Ch3_Input_Capture_Setup() {
	// Please note: Many steps are repeated in case Ch3 is used on its own

	// Enable GPIO port A clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	// Configure PA2 as alternative function (10)
	GPIOA->MODER &= ~GPIO_MODER_MODE2;
	GPIOA->MODER |= GPIO_MODER_MODE2_1;
	// Configure and select alternative function for PA2 (AF1)
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL2;
	GPIOA->AFR[0] |= (0x1 << 2*4);
	// Configure PA2 as no pull-up, no pull-down (00)
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD2;

	// Enable TIM2 clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	// Set prescaler value to 15
	TIM2->PSC &= ~TIM_PSC_PSC;
	TIM2->PSC |= 0xF;
	// Enable preload for timer 2
	TIM2->CR1 |= TIM_CR1_ARPE;
	// Set auto-reload value to max
	TIM2->ARR |= TIM_ARR_ARR;
	// Configure channel to be used in input capture mode
	// Clear capture and compare mode register
	TIM2->CCMR2 &= ~TIM_CCMR2_CC3S;
	// Set to capture input on timer channel 3
	TIM2->CCMR2 |= TIM_CCMR2_CC3S_0;
	// Enable input capture on both rising and falling edges
	TIM2->CCER |= TIM_CCER_CC3E;
	TIM2->CCER |= TIM_CCER_CC3P;
	TIM2->CCER |= TIM_CCER_CC3NP;
	// Enable interrupts for timer 2
	TIM2->DIER |= TIM_DIER_CC3IE;
	// Enable DMA requests for timer 2
	TIM2->DIER |= TIM_DIER_CC3DE;
	// Enable update interrupt for timer 2
	TIM2->DIER |= TIM_DIER_UIE;
	// Enable update generation
	TIM2->EGR |= 0x1;
	// Clear update interrupt flag
	TIM2->SR &= ~TIM_SR_UIF;
	// Set direction such that timer counts up
	TIM2->CR1 &= ~TIM_CR1_DIR;
	// Enable the counter
	TIM2->CR1 |= 0x1;

	// Enable TIM4_IRQn interrupt in NVIC and set priority to 2
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 2);
}

void TIM2_IRQHandler(void) {
	// currentValue = CCR_New
	// lastValue = CCR_Last
	// overflowCount = OC
	// timeInterval = CCR_New - CCR_Last + (1+ARR)*OC
	//  Note that we set ARR to the max: 1+(2^16-1) = 65536

	if(TIM2->SR & TIM_SR_UIF) { // UIF Interrupt
		TIM2->SR &= ~TIM_SR_UIF;
		overflowCount_1++;
	} else {
		if(TIM2->SR & TIM_SR_CC1IF) { // Channel 1
			TIM2->SR &= ~TIM_SR_CC1IF;

			if(GPIOA->IDR & GPIO_IDR_ID0) { // Echo pin high (rising edge)
				lastValue_1 = TIM2->CCR1 & TIM_CCR1_CCR1;
				
			} else { // Echo pin low (falling edge)
				currentValue_1 = TIM2->CCR1 & TIM_CCR1_CCR1;

				// Compute pulse width and reset overflow count
				timeInterval_1 = currentValue_1 - lastValue_1 + 65536*overflowCount_1;
				overflowCount_1 = 0;
			}
		}
		if(TIM2->SR & TIM_SR_CC2IF) { // Channel 2
			TIM2->SR &= ~TIM_SR_CC2IF;

			if(GPIOA->IDR & GPIO_IDR_ID1) { // Echo pin high (rising edge)
				lastValue_2 = TIM2->CCR2 & TIM_CCR2_CCR2;
				
			} else { // Echo pin low (falling edge)
				currentValue_2 = TIM2->CCR2 & TIM_CCR2_CCR2;

				// Compute pulse width and reset overflow count
				timeInterval_2 = currentValue_2 - lastValue_2 + 65536*overflowCount_2;
				overflowCount_2 = 0;
			}
		}
		if(TIM2->SR & TIM_SR_CC3IF) { // Channel 3
			TIM2->SR &= ~TIM_SR_CC3IF;

			if(GPIOA->IDR & GPIO_IDR_ID2) { // Echo pin high (rising edge)
				lastValue_3 = TIM2->CCR3 & TIM_CCR3_CCR3;
				
			} else { // Echo pin low (falling edge)
				currentValue_3 = TIM2->CCR3 & TIM_CCR3_CCR3;

				// Compute pulse width and reset overflow count
				timeInterval_3= currentValue_3 - lastValue_3 + 65536*overflowCount_3;
				overflowCount_3 = 0;
			}
		}
	}
}

void setStationary(int xStat, int yStat, int zStat) {
	// Calibrate X, Y, and Z by adjusting toward the opposite direction
	currentSpeed.x += -1*xStat;
	currentSpeed.y += -1*yStat;
	currentSpeed.x += -1*zStat;
};

void getSpeed(uint8_t highAddr, uint8_t lowAddr, uint8_t direction) {
	int16_t speed = 0;

	// Check current status of the gyroscope
	uint8_t temp = 0;
	GYRO_IO_Read(&temp, L3GD20_STATUS_REG_ADDR, sizeof(temp));
	// Make sure there is a new speed to read
	if(temp & direction) {
		temp = 0;			// Read in upper 8 bits
		GYRO_IO_Read(&temp, highAddr, sizeof(temp));
		speed |= temp << 8;
		temp = 0;			// Read in lower 8 bits
		GYRO_IO_Read(&temp, lowAddr, sizeof(temp));
		speed |= temp;
	}

	// If speed is negative, convert out of 2's complement
	if(speed & 0x80) speed = -1*(~speed + 0x1);
	// Convert to degrees per second
	speed = 0.07*speed;

	if(direction & 0x1) {
		currentSpeed.x = speed;
	} else if(direction & 0x2) {
		currentSpeed.y = speed;
	} else if(direction & 0x4) {
		currentSpeed.z = speed;
	}
}

void updateSpeed() {
	// Results are stored in currentSpeed global variable
	getSpeed(L3GD20_OUT_X_H_ADDR, L3GD20_OUT_X_L_ADDR, 0x1);
	getSpeed(L3GD20_OUT_Y_H_ADDR, L3GD20_OUT_Y_L_ADDR, 0x2);
	getSpeed(L3GD20_OUT_Z_H_ADDR, L3GD20_OUT_Z_L_ADDR, 0x4);
};

void enterLowPowerState() {
	// Disable the GPIO ports and timers for the ultrasonic sensors
	RCC->AHB2ENR &= ~RCC_AHB2ENR_GPIOEEN;
	RCC->AHB2ENR &= ~RCC_AHB2ENR_GPIOAEN;
	RCC->APB1ENR1 &= ~RCC_APB1ENR1_TIM2EN;
	RCC->APB2ENR &= ~RCC_APB2ENR_TIM1EN;
}

void exitLowPowerState() {
	// Enable the GPIO ports and timers for the ultrasonic sensors
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
}

int main(void){
	// Enable High Speed Internal Clock (HSI = 16 MHz)
	RCC->CR |= RCC_CR_HSION;
	// Select HSI as system clock source 
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_HSI;
	// Wait until HSI is system clock source and ready
	while ((RCC->CFGR & RCC_CFGR_SWS) == 0);
	while ((RCC->CR & RCC_CR_HSIRDY) == 0);

	System_Clock_Init();
	SysTick_Init();

	LCD_Initialization();
	LCD_Clear();

	UART2_Init();
	UART2_GPIO_Init();
	USART_Init(USART2);

	// Trigger Setup (The same trigger is used for all sensors)
	Trigger_Setup();
	// Input Capture Setup
	Ch1_Input_Capture_Setup();
	Ch2_Input_Capture_Setup();
	Ch3_Input_Capture_Setup();

	// Initialize Gyroscope
	GYRO_Init();

	// Initalize currentSpeed
	updateSpeed();

	bool active = 0;
	bool sensor1 = 0;
	bool sensor2 = 0;
	bool sensor3 = 0;
	char message[6];
	while(1) {
		// Note: Our X value is -1 when the board is stationary, so we must calibrate
		//  or adjust the velocity values based on the gyroscope's default readings
		setStationary(-1, 0, 0);

		// Use gyroscope reading to toggle functionality
		if(currentSpeed.x > THRESHOLD || currentSpeed.x < -1*THRESHOLD ||
			 currentSpeed.y > THRESHOLD || currentSpeed.y < -1*THRESHOLD) {
			active = !active;
		}

		if(active) {
			exitLowPowerState();

			// The HC-SR04 sensor can measure distances between 2cm and 400cm,
			//  but the user will soon hit something if they are within 100cm
			sensor1 = (timeInterval_1 < 58*100 && timeInterval_1 >= 58*2) ? 1 : 0;
			sensor2 = (timeInterval_2 < 58*100 && timeInterval_2 >= 58*2) ? 1 : 0;
			sensor3 = (timeInterval_3 < 58*100 && timeInterval_3 >= 58*2) ? 1 : 0;

			// Convert sensor measurements to a distance in cm, or "--" if range is invalid
			if(sensor1 && sensor2 && sensor3) {
				sprintf(message, "%2d%2d%2d", timeInterval_1/58, timeInterval_2/58, timeInterval_3/58);
			} else if(sensor1 && sensor2 && !sensor3) {
				sprintf(message, "%2d%2d%s", timeInterval_1/58, timeInterval_2/58, "--");
			} else if(sensor1 && !sensor2 && sensor3) {
				sprintf(message, "%2d%s%2d", timeInterval_1/58, "--", timeInterval_3/58);
			} else if(sensor1 && !sensor2 && !sensor3) {
				sprintf(message, "%2d%s", timeInterval_1/58, "----");
			} else if(!sensor1 && sensor2 && sensor3) {
				sprintf(message, "%s%2d%2d", "--", timeInterval_2/58, timeInterval_3/58);
			} else if(!sensor1 && sensor2 && !sensor3) {
				sprintf(message, "%s%2d%s", "--", timeInterval_2/58, "--");
			} else if(!sensor1 && !sensor2 && sensor3) {
				sprintf(message, "%s%2d", "----", timeInterval_3/58);
			} else {
				sprintf(message, "%s", "------");
			}
		} else {
			enterLowPowerState();
			sprintf(message, "%s", " OFF  ");
		}
		
		LCD_DisplayString((uint8_t *) message);

		// Connection to Termite available for testing and debugging
		printf("Current Angular Velocity\n   X: %.01f\n   Y: %.01f\n   Z: %.01f\n\n",
				currentSpeed.x, currentSpeed.y, currentSpeed.z);
		
		updateSpeed();
		delay(500); // Small delay between receiving measurements
	}
}