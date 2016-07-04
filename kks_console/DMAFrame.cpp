#ifndef __DMAFRAME
#define __DMAFRAME

#include <vector>

//Структура для хранения и работы со фреймом из DMA
//Размер - 16384 DMAWord = 65536 байт = 131072 временных отсчётов
#include "DMAWord.cpp"
#include "detections.cpp"

struct DMAFrame
{
	DMAFrame(char *buf) { memory = (DMAWord*)buf; };
	~DMAFrame() {delete memory;};
	
  int find_start_gen(void);//Ищет номер нулевого бита окна генерации
  unsigned int find_detect( unsigned int pos );//Ищет номер детектированного бита, начиная с позиции pos
  detections to_detections( peers who_am_i );//Возвращает структуру detections для Боба, сформированную из DMAFrame 
  //Актуально для Боба. Возвр. число отсчётов от последнего срабатывания детектора
  //до конца фрейма - необходимо для сшивания результата to_detections( ... ) со внешней переменной
  unsigned int ticks_left_to_end(std::vector<unsigned int> &v);	

	DMAWord *memory;
  size_t TICS = 16384*8;//Число временных отсчётов
	size_t WORDS = TICS/8; //Число DMAWord в одном фрейме
	size_t size_bytes = WORDS*4; //Размер memory в байтах
};

int DMAFrame::find_start_gen(void)
{
	size_t w;//Номер DMAWord
	for ( w = 0; w < WORDS; w++ ) if ( memory[w].calibr_state != 0 ) break;
	
	if ( w == WORDS )//Если в данном фрейме нет калибровки вообще
		return TICS;
		
  size_t bit;
	for ( bit = 0; bit < 8; bit++ ) if ( memory[w].calibr(bit) != 0 ) break;
	
	return w * 8 + bit - 1;
}

unsigned int DMAFrame::find_detect( unsigned int pos )
{
	size_t w;
	for ( w = pos/8; w < WORDS; w++ ) if ( memory[w].detections != 0 ) break;
	if ( w == WORDS ) return TICS;
	size_t bit;
	for ( bit = 0; bit < 8; bit++ ) if ( memory[w].detect(bit) != 0 ) break;
	
	return w*8 + bit;
}

detections DMAFrame::to_detections( peers who_am_i )
{
	detections ret;
	switch (who_am_i)
	{
		case peers::alice:
		for (size_t w = 0; w < WORDS; w++)
			for (size_t bit = 0; bit < 8; bit++)
			{
				uint8_t _bs = memory[w].bs(bit);
				ret.basis.push_back(_bs & 0b01);
				ret.key.push_back(_bs & 0b10);
				ret.special.push_back(0);
			}
		break;
		
    case peers::bob:
	  size_t last_detect;
		for (size_t w = 0; w < WORDS; w++)
		{
			if (memory[w].detections != 0)
			for (size_t bit = 0; bit < 8; bit++)
			if (memory[w].detect(bit)) 
			{
				uint8_t _bs = memory[w].bs(bit);
				ret.basis.push_back(_bs & 0b01);
				ret.key.push_back(_bs & 0b10);
				last_detect = w*8 + bit;
				ret.count.push_back(last_detect);
			}
		}
		//Один лишний элемент хранит число бит осталось от последнего срабатывания до конца фрейма
	  ret.count.push_back(ticks_left_to_end(ret.count));
    break;
    
    case gui:
    case telnet:
    case peers_size:
      break;
    default:;
	  }

  return ret;
}

unsigned int DMAFrame::ticks_left_to_end(std::vector<unsigned int> &v)
{
  unsigned int answer = TICS;
	for (auto i : v) answer -= i;
  return answer;
}
#endif
