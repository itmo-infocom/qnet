//Класс, реализующий безопасный интерфейс для работы с платой

#ifndef BOARD_IF_CPP
#define BOARD_IF_CPP

#include <string>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <errno.h>//Для использования errno
#include <string.h>//Ради strerror

#include "./driverAnB/src/lib/AnBDefs.h"
#include "./driverAnB/src/lib/driver.h"
#include "./driverAnB/src/lib/driver.cpp"
//#include "DMAFrame.h"
#include "detections.cpp"

namespace board_if
{

using namespace AnB;
using namespace std;

    class board_if
    {
        const double frequency = 1e7;//Частота работы ПЛИС - необходимо для выбора размера буфера
        bool type;//Тип устройства
        bool DMAStatus;//Текущий статус DMA
        device_list devices;//Список всех подключённых устройства
        AnBDriverAPI *top, *bottom;//Верхняя и нижняя платы
       
        unsigned short int num_next_frame = 0;//Номер следующего фрейма, подлежащего считыванию. Необходим для обнаружения переполнения буфера.

        struct DMAFrame
        {
            DMAFrame(char *buf)
            {
                p = (unsigned int *)buf;
                count = p[0] & 0xFFFF;
            };
            unsigned short int count;
            //Возвращает базисное состояние с позиции bs_at в пределах данного фрейма
            uint8_t bs_at(unsigned int pos) {return (p[pos/8] >> ((pos%8)*2)) & 0b11;};
        private:
            unsigned int *p;
        } *curr_frame;
        bool curr_frame_initialized = false;
    public:
        //Структура для обработки исключений
        struct except{
            private:
            struct error errstruct;
            public:
            string errstr;
            except(string e) //e - имя метода, вызвавшего ошибку
            {
                errstr = e; e += ": ";
                AnBDriverAPI::GlobalError(errstruct);

                e += AnBDriverAPI::ErrorStr(errstruct);
            };
        };

        board_if() throw(except);
        ~board_if();

        //Возвращает базисные состояния на позициях count, начиная с нулевого отсчёта нулевого фрейма. Калибровки и метки ABCD не учитываются
        detections get_detect(vector<unsigned int> count);

        //Записывает таблицу случайных чисел из вектора
        void TableRNG(vector<int> table);

        //Устанавливает или считывает статус DMA
        bool StatusDMA() {return DMAStatus;};
        void SetDMA(bool status);

        //Записывает во внутренную память метода накопленные за t миллисекунд фреймы DMA
        void StoreDMA(double t, int argc, char** argv);

        //Устанавливает размер буфера DMA драйвера в секундах
        void SetBufSize(float t) {top->SetBuffersCount(t*frequency/(1<<17)+1);};

        //Удаляет фреймы DMA, пока не повстречает фрейм с нулевым номером - он будет сохранён в *curr_frame
        void clear_buf(void);
    };

    board_if::board_if() throw(except)
    {
        if (!AnBDriverAPI::GetDevicesList(devices))
            throw except("GetDeviceList");

        //Определим какого типа плата у нас
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegMode;
            if (!devices.array[0].dev_ref->RegRawRead(reg))
            throw except("RegRawRead");
            type = reg.value.mode.mode;
        }

        if (type)
        {
            //Если Боб - то можно работать с двумя платами
            top = devices.array[0].dev_ref;
            bottom = devices.array[1].dev_ref; 
        }
        else
        {
            //Если Алиса - то только одна плата
            top = devices.array[0].dev_ref;
            bottom = nullptr;
        }

        //Выставим strobe-delay на ноль
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegSTB;
            reg.value.raw = 0;
            if (!top->RegRawWrite(reg)) throw except(top->LastError());
            if (type)
                if (!bottom->RegRawWrite(reg)) throw except(bottom->LastError());
        }

       //Установим в качестве источника ГСЧ TableRNG
       if (false)
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTest;
            if (!top->RegRawRead(reg)) throw except(top->LastError());
            reg.value.raw |= 0b1 << 31;//Отключил использование внешнего ГСЧ (LVDS) 
            reg.value.raw &= ~(0b1 << 30);//Включил использование таблицы TableRNG

            reg.value.raw &= ~(0b1 << 7);//Принимаем детектирования с настоящего детектора
            if (!top->RegRawWrite(reg)) throw except(top->LastError());
            if (type) 
                if (!bottom->RegRawWrite(reg)) throw except(bottom->LastError());
        }

        //Установим dma enable в ноль
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegDMA;
            reg.value.dma.enabled = 0;
            if (!top->RegRawWrite(reg)) throw except(top->LastError());
            if (type) 
                if (!bottom->RegRawWrite(reg)) throw except(bottom->LastError());
        }

        //Установка режима работы таблицы
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTable;
            if (!top->RegRawRead(reg)) throw except(top->LastError());
            reg.value.table.mode = 4;
            if (!top->RegRawWrite(reg)) throw except(top->LastError());
            if (type)
                if (!bottom->RegRawWrite(reg)) throw except(top->LastError());
        }
        top->SetBuffersCount(frequency/(1<<16));//Буферов на 2 секунды

        DMAStatus = false;
    }

    board_if::~board_if()
    {
        if (DMAStatus) SetDMA(false);
        //TODO: Надо запихнуть запись конфигурации в файл
    }

    detections board_if::get_detect(vector<unsigned int> count)
    {
        detections answer;

        for (auto i : count)
        {
            while(curr_frame->count < i/(AnBBufferMinSize*2))
            {
                if (curr_frame_initialized) delete curr_frame;
                char *buf;
                if (!top->DMARead(buf)) throw except(top->LastError());
                curr_frame = new DMAFrame(buf);
            }

            if (curr_frame->count == i / (AnBBufferMinSize*2))
            {
                uint8_t bs_tmp = curr_frame->bs_at(i % (AnBBufferMinSize*2));

                answer.basis.push_back(bs_tmp & 0b01);
                answer.key.push_back(bs_tmp & 0b10);
            }
        }

        return answer;
    };

    void board_if::TableRNG(const std::vector<int> t)
    {
        //Размер таблицы должен быть больше 8. При размере меньшем восьми работа немного непредсказуема - не понимаю кто куда уходит.
        using namespace std;
        
        //Сформируем буфер для записи в плату
        {
            size_t buf_size = t.size()/4 + ((t.size()%4)?1:0);
            char *buf = new char[buf_size];
            for (size_t i = 0; i < buf_size; i++) buf[i] = 0;

            for (size_t i = 0; i < t.size(); i++)
            {
                //Мулька в том, что после инициализации старта TableRNG начинает считываьтся с 9 элемента - поэтому запишем TableRNG с этим смещением
                const size_t offset = 9;
                size_t pos = t.size() + offset;  
                buf[i/4] |= (t[(i+offset)%t.size()] & 0b11) << ((i % 4)*2);
            }

            if (!top->WriteTable(buf, buf_size, DestTables::TableRNG))
                throw except(top->LastError());
            if (type) 
                if (!bottom->WriteTable(buf, buf_size, DestTables::TableRNG)) 
                    throw except(bottom->LastError()); 
            delete buf;
        }

        //Зафиксируем размер таблицы в регистрах ПЛИС
        {  
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTable;
            if (!top->RegRawRead(reg)) throw except(top->LastError());
            reg.value.table.size = t.size();
            if (!top->RegRawWrite(reg)) throw except(top->LastError());
            if (type)
                if (!bottom->RegRawWrite(reg)) throw except(bottom->LastError());
        }

    }

    void board_if::SetDMA(bool status)
    {
        if (status == DMAStatus) return; 
        DMAStatus = status;

        if (status)
        {
            //Поднимем start-бит
            cout << "Поднимаем Start-бит" << endl;
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTest;
            if (!top->RegRawRead(reg)) throw except(top->LastError());
            reg.value.raw |= 0b01;//Поднимаем start-бит
            reg.value.raw &= ~0b10;//Опускаем stop-бит
            if (!top->RegRawWrite(reg)) throw except(top->LastError());

            cout << "DMAEnable" << endl;
            //Включим DMA
            if (!top->DMAEnable()) throw except(top->LastError());
        } else
        {
            cout << "DMADisable" << endl;
            if (!top->DMADisable()) throw except(top->LastError());

            cout << "Опускаем start-бит" << endl;
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTest;
            if (!top->RegRawRead(reg)) throw except(top->LastError());
            reg.value.raw &= ~0b01;//Опускаем start-бит
            reg.value.raw |= 0b10;//Поднимаем stop-бит
            if (!top->RegRawWrite(reg)) throw except(top->LastError());
            usleep(100e3);//Задержка чтобы параллельные потоки успели тормознуть DMA и не возникало race conditions
        }
    }

    void board_if::StoreDMA(double t, int argc, char** argv)
    {

        if (true)
        {
            if (argc < 2) return;

            {
                vector<int> tmp;
                tmp.resize(stoi(argv[1]));
                for (size_t i = 0; i < tmp.size(); i++) tmp[i] = random();
                TableRNG(tmp);
            }

            SetDMA(true);

            clock_t start = clock();
            //Число прочитанных фреймов
            unsigned int readed = 0;
            while (readed < stoi(argv[2])) 
            {   
                char *buf = nullptr;
                //cout << 'R';
                if (!top->DMARead(buf)) throw except(top->LastError());

                //cout << errno << ' ';
                cout << ++readed;

                //if (false)

                {
                    unsigned int *p = (unsigned int *)buf;
                    cout << ' ' << hex << ((p[0] >> 16) & 0xFFFF);
                    cout << ' ' << dec << (p[0] & 0xFFFF);
                    if ((p[0] & 0xFFFF) == 0 || true)
                        //Выведем базисные состояния
                        for (int w = 1; w <= stoi(argv[1])/8+1; w++)
                        {
                            short int tmp = p[w] & 0xFFFF;
                            cout << ' ';
                            for (int j = 0; j < 16; j+=2) cout << ((tmp >>j) & 0b11); 
                        }
                }   
                cout << endl;
                delete buf;
            }
            SetDMA(false);
            cout << "Скорость: " << (stoi(argv[2])*(1<<17)/(clock() - start)*(float)CLOCKS_PER_SEC) << " Мсэмпл/сек" << endl;
        }
    }

    void board_if::clear_buf()
    {
        if (curr_frame_initialized) delete curr_frame;
        do {
            char *buf;
            if (!top->DMARead(buf)) throw except(top->LastError());
            curr_frame = new DMAFrame(buf);
        } while(curr_frame->count != 0);
    }
};

#endif // !BOARD_IF_CPP
