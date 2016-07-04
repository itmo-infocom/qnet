//Одно слово DMA и методы работы с ним
#ifndef __DMAWORD
#define __DMAWORD

struct DMAWord
{
	DMAWord( unsigned int w )
	{
		detections 		= (w >> (16 + 8)) & 0xFF;
		calibr_state 	= (w >> 16) & 0xFF;
		basis_state		= w && 0xFFFF;
	}
	static uint8_t detect( int pos ) { return (detections >> pos) & 0b1; };
	static uint8_t calibr( int pos ) { return (calibr_state >> pos) & 0b1; };
	static uint8_t bs( int pos ) { return (basis_state >> pos) & 0b11; };
	
	uint8_t detections;
	uint8_t calibr_state;
	uint16_t basis_state;
}
#endif