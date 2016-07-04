#ifndef __DMAFRAME
#define __DMAFRAME

//Структура для хранения и работы со фреймом из DMA
//Размер - 16384 DMAWord = 65536 байт = 131072 временных отсчётов
#include "DMAWord.cpp"
#include "detections.cpp"

struct DMAFrame
{
	DMAFrame(char *buf) { memory = buf; };
	~DMAFrame() {delete memory;};
	
	static int find_start_gen(void);//Ищет номер нулевого бита окна генерации
	static unsigned int find_detect( unsigned int pos );//Ищет номер детектированного бита, начиная с позиции pos
	
	DMAWord *memory;
	enum { 
		SIZE = 16384*8,//Число временных отсчётов
		WORDS = SIZE/8 //Число DMAWord в одном фрейме
		size_bytes = WORDS*4 //Размер memory в байтах
	}
}

static int DMAFrame::find_start_gen(void)
{
	short int w;//Номер DMAWord
	for ( w = 0; w < WORDS; w++ ) if ( DMAWord[w].calibration_state != 0 ) break;
	
	if ( w == WORDS )//Если в данном фрейме нет калибровки вообще
		return SIZE;
		
	short int bit;
	for ( bit = 0; bit < 8; bit++ ) if ( DMAWord[w].calibr(bit) != 0 ) break;
	
	return w * 8 + bit - 1;
}

static int DMAFrame::find_detect( unsigned int pos )
{
	short int w;
	for ( w = pos/8; w < WORDS; w++ ) if ( DMAWord[w].detections != 0 ) break;
	if ( w == WORDS ) return SIZE;
	short int bit;
	for ( bit = 0; bit < 8; bit++ ) if ( DMAWord[w].detect(bit) != 0 ) break;
	
	return w*8 + bit;
}

detections DMAFrame::to_detections( peers who_am_i )
{
	detections ret;
	switch (who_am_i)
	{
		case peers::alice:
		for (int w = 0; w < WORDS; w++)
			for (int bit = 0; bit < 8; bit++)
			{
				uint8_t _bs = memory[w].bs(bit);
				ret.basis.push_back(_bs & 0b01);
				ret.key.push_back(_bs) & 0b10);
				ret.special.push_back(0);
			}
		break;
		case peers::bob:
		unsigned int last_detect;
		for (int w = 0; w < WORDS; w++)
		{
			if (memory[w].detections != 0)
			for (int bit = 0; bit < 8; bit++)
			if (memory[w].detect(bit)) 
			{
				uint8_t _bs = memory[w].bs(bit);
				ret.basis.push_back(_bs & 0b01);
				ret.key.push_back(_bs & 0b10);
				last_detect = w*8 + bit
				ret.count.push_back(last_detect);
			}
		}
		ret.count.push_back(ticks_left_to_end(ret));//Один лишний элемент хранит число бит осталось от последнего срабатывания до конца фрейма
		break;
		default: break;
	}
	}
}

unsigned int DMAFRAME::ticks_left_to_end(vector<unsigned int> &v)
{
	unsigned int answer = SIZE;
	for (auto i : v) answer -= *i;
}
#endif