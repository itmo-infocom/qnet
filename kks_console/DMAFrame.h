#ifndef DMAFRAME_CPP
#define DMAFRAME_CPP

#include <vector>
#include <unistd.h>

//Структура для хранения и работы со фреймом из DMA
//Размер - 16384 DMAWord = 65536 байт = 131072 временных отсчётов
#include "DMAWord.cpp"
#include "detections.cpp"

struct DMAFrame
{
	DMAFrame(char *buf);
	
  detections to_detections( bool bob );//Возвращает структуру detections, сформированную из DMAFrame 

	unsigned int bs_at(size_t pos);//Возвращает базисное состояние на позиции pos

	unsigned int raw_at(size_t pos);

  static const std::size_t TICS = 16384*8;//Число временных отсчётов
	static const std::size_t WORDS = TICS/8; //Число DMAWord в одном фрейме
	static const std::size_t size_bytes = WORDS*4; //Размер memory в байтах
private:
	std::vector<DMAWord> memory;
  int find_start_gen(void);//Ищет номер нулевого бита окна генерации
  unsigned int find_detect( unsigned int pos );//Ищет номер детектированного бита, начиная с позиции pos
	unsigned int ticks_left_to_end(std::vector<unsigned int> &v);	
};

DMAFrame::DMAFrame(char *buf)
{
	//memory.reserve(WORDS);
	for (size_t i = 0; i < WORDS; i++)
	memory.push_back((unsigned int)*(buf + i*sizeof(unsigned int)));
	delete buf;
}

int DMAFrame::find_start_gen(void)
{
	std::size_t w;//Номер DMAWord
	for ( w = 0; w < WORDS; w++ ) if ( memory[w].calibr_state != 0 ) break;
	
	if ( w == WORDS )//Если в данном фрейме нет калибровки вообще
		return TICS;
		
  std::size_t bit;
	for ( bit = 0; bit < 8; bit++ ) if ( memory[w].calibr(bit) != 0 ) break;
	
	return w * 8 + bit - 1;
};

unsigned int DMAFrame::find_detect( unsigned int pos )
{
	std::size_t w;
	for ( w = pos/8; w < WORDS; w++ ) if ( memory[w].detections != 0 ) break;
	if ( w == WORDS ) return TICS;
	std::size_t bit;
	for ( bit = 0; bit < 8; bit++ ) if ( memory[w].detect(bit) != 0 ) break;
	
	return w*8 + bit;
};

detections DMAFrame::to_detections( bool bob )
{
	detections ret;
	if (bob)
	{
		std::size_t last_detect;
		for (std::size_t w = 0; w < WORDS; w++)
		{
			if (memory[w].detections != 0)
			for (std::size_t bit = 0; bit < 8; bit++)
			if (memory[w].detect(bit)) 
			{
				unsigned int _bs = memory[w].bs(bit);
				ret.basis.push_back(_bs & 0b01);
				ret.key.push_back(_bs & 0b10);
				last_detect = w*8 + bit;
				ret.count.push_back(last_detect);
			}
		}
		//Один лишний элемент - сколкьо бит осталось от последнего срабатывания до конца фрейма. Нужно для сшивки отдельных фреймов
		ret.count.push_back(ticks_left_to_end(ret.count));
	} else 
	{
		for (std::size_t w = 0; w < WORDS; w++)
			for (std::size_t bit = 0; bit < 8; bit++)
			{
				unsigned int _bs = memory[w].bs(bit);
				ret.basis.push_back(_bs & 0b01);
				ret.key.push_back(_bs & 0b10);
			}
	}

  return ret;
};

unsigned int DMAFrame::ticks_left_to_end(std::vector<unsigned int> &v)
{
  unsigned int answer = TICS;
	for (auto i : v) answer -= i;
  return answer;
};

unsigned int DMAFrame::bs_at(size_t pos)
{
	return (memory[pos/8].bs(pos % 8));
}

uint32_t DMAFrame::raw_at(size_t pos)
{
	return memory[pos].raw;
}
#endif
