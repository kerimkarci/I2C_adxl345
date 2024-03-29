/*
 * i2c.c
 *
 *  Created on: Jan 24, 2024
 *      Author: Kerim
 */

#include "stm32f4xx.h"

#define GPIOBEN			(1<<1)
#define I2C1EN			(1<<21)

#define I2C_100KHz		80 			//0B 0101 0000
#define SD_MODE_MAX_RISE_TIME	17
#define SR2_BUSY		(1<<1)
#define CR1_START		(1<<8)
#define SR1_SB			(1<<0)
#define SR1_ADDR		(1<<1)
#define SR1_TXE			(1<<7)
#define CR1_ACK			(1<<10)
#define CR1_STOP		(1<<9)
#define SR1_RXNE		(1<<6)
#define SR1_BTF			(1<<2)
/*
 * PB8---SCL
 * PB9---SDA
 * */


void I2C1_init(void)
{
	/*Enable clock access to GPIOB*/
	RCC->AHB1ENR |=GPIOBEN;
	/*Set PB8 and PB9 mode to alternate function*/
	GPIOB->MODER &= ~(1<<16);
	GPIOB->MODER |= (1<<17);

	GPIOB->MODER &= ~(1<<18);
	GPIOB->MODER |=  (1<<19);
	/*Set PB8 and PB9 output type to open drain*/

	GPIOB->OTYPER |= (1<<8);
	GPIOB->OTYPER |= (1<<9);

	/*Enable Pull-up for PB8 and PB9*/
	GPIOB->PUPDR |=(1<<16);
	GPIOB->PUPDR &=~(1<<17);

	GPIOB->PUPDR |= (1<<18);
	GPIOB->PUPDR &= ~(1<<19);


	/*Set PB8 and PB9 alternate function type to I2C (AF4)*/
	GPIOB->AFR[1] &=~(1U<<0);
	GPIOB->AFR[1] &=~(1U<<1);
	GPIOB->AFR[1] |= (1U<<2);
	GPIOB->AFR[1] &=~(1U<<3);

	GPIOB->AFR[1] &=~(1U<<4);
	GPIOB->AFR[1] &=~(1U<<5);
	GPIOB->AFR[1] |= (1U<<6);
	GPIOB->AFR[1] &=~(1U<<7);


	/*Enable clock access to I2C1*/
	RCC->APB1ENR |= I2C1EN;

	/*Enter reset mode*/
	I2C1->CR1 |= (1<<15);

	/*Come out of  reset mode*/
	I2C1->CR1 &= ~(1<<15);

	/*Set peripheral clock freq =16 MHz */
	I2C1->CR2 |= (1<<4);

	/*Set I2C to standard mode, 100KHz clock*/
	I2C1->CCR |= I2C_100KHz;

	/*Set rise time*/
	I2C1->TRISE |=SD_MODE_MAX_RISE_TIME;

	/*Enable I2C*/
	I2C1->CR1 |= (1<<0);

}

void I2C1_byteRead(char saddr, char maddr, char* data)
{
	volatile int tmp;

	/*sure I2C is not busy*/
	/*wait until bus not busy*/
	while(I2C1->SR2 & (SR2_BUSY)){}

	/*Generate start*/
	I2C1->CR1 |=CR1_START;

	/*Wait until start flag is set*/
	while(!(I2C1->SR1 & (SR1_SB))){}

	/*Transmit slave address + write*/
	I2C1->DR = saddr <<1;

	/*wait until addr flag is set*/
	while(!(I2C1->SR1 & (SR1_ADDR))){}

	/*Clear addr flag*/
	tmp = I2C1->SR2;

	/*Send memory address*/
	I2C1->DR = maddr;

	/*wait until transmitter empty*/
	while (!(I2C1->SR1 & SR1_TXE)){}

	/*Generate restart*/
	I2C1->CR1 |=CR1_START;

	/*Wait until start flag is set*/
	while(!(I2C1->SR1 & (SR1_SB))){}

	/*Transmit slave address +Read */
	I2C1->DR = saddr << 1 | 1;

	/*wait until addr flag is set*/
	while(!(I2C1->SR1 & (SR1_ADDR))){}

	/*Disable the Acknowledge*/
	I2C1->CR1 &= ~(CR1_ACK);

	/*Clear addr flag*/
	tmp = I2C1->SR2;

	/*Generate stop after data received*/
	I2C1->CR1 |= CR1_STOP;

	/*wait until RXNE flag is set*/
	while(!(I2C1->SR1 & SR1_RXNE));

	/*Read data from DR*/
	*data++ = I2C1->DR;

}


void I2C1_burstRead(char saddr, char maddr, int n, char* data) {

	volatile int tmp;

	 /* Wait until bus not busy */
	  while (I2C1->SR2 & (SR2_BUSY)){}

      /* Generate start */
    I2C1->CR1 |= CR1_START;

    /* Wait until start flag is set */
     while (!(I2C1->SR1 & SR1_SB)){}

     /* Transmit slave address + Write */
     I2C1->DR = saddr << 1;

     /* Wait until addr flag is set */
    while (!(I2C1->SR1 & SR1_ADDR)){}

    /* Clear addr flag */
     tmp = I2C1->SR2;

     /* Wait until transmitter empty */
    while (!(I2C1->SR1 & SR1_TXE)){}

    /*Send memory address */
    I2C1->DR = maddr;

    /*Wait until transmitter empty */
    while (!(I2C1->SR1 & SR1_TXE)){}

    /*Generate restart */
    I2C1->CR1 |= CR1_START;

    /* Wait until start flag is set */
    while (!(I2C1->SR1 & SR1_SB)){}

    /* Transmit slave address + Read */
    I2C1->DR = saddr << 1 | 1;

    /* Wait until addr flag is set */
    while (!(I2C1->SR1 & (SR1_ADDR))){}

    /* Clear addr flag */
    tmp = I2C1->SR2;

    /* Enable Acknowledge */
      I2C1->CR1 |=  CR1_ACK;

    while(n > 0U)
    {
    	/*if one byte*/
    	if(n == 1U)
    	{
    		/* Disable Acknowledge */
    	    I2C1->CR1 &= ~CR1_ACK;

    	    /* Generate Stop */
    	    I2C1->CR1 |= CR1_STOP;

 			/* Wait for RXNE flag set */
            while (!(I2C1->SR1 & SR1_RXNE)){}

            /* Read data from DR */
            *data++ = I2C1->DR;
            break;
    	}
    	else
    	{
       	 /* Wait until RXNE flag is set */
           while (!(I2C1->SR1 & SR1_RXNE)){}

           /* Read data from DR */
           (*data++) = I2C1->DR;

           n--;
    	}
    }

}


void I2C1_burstWrite(char saddr, char maddr, int n, char* data) {

	volatile int tmp;

	 /* Wait until bus not busy */
	 while (I2C1->SR2 & (SR2_BUSY)){}

     /* Generate start */
    I2C1->CR1 |= CR1_START;

    /* Wait until start flag is set */
    while (!(I2C1->SR1 & (SR1_SB))){}

    /* Transmit slave address */
    I2C1->DR = saddr << 1;

    /* Wait until addr flag is set */
    while (!(I2C1->SR1 & (SR1_ADDR))){}

    /* Clear addr flag */
    tmp = I2C1->SR2;

    /* Wait until data register empty */
    while (!(I2C1->SR1 & (SR1_TXE))){}

    /* Send memory address */
    I2C1->DR = maddr;

    for (int i = 0; i < n; i++) {

     /* Wait until data register empty */
        while (!(I2C1->SR1 & (SR1_TXE))){}

      /* Transmit memory address */
      I2C1->DR = *data++;
    }

    /* Wait until transfer finished */
    while (!(I2C1->SR1 & (SR1_BTF))){}

    /* Generate stop */
   I2C1->CR1 |= CR1_STOP;
}



