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
#include "DMAFrame.h"

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
       
        std::vector<DMAFrame> DMAFrames;//TODO: Переделать в структуру, которая хранит и сможет возвращать непрерывный поток бит или выдавать нужные биты по номеру или по поиску detections
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

        //Возвращает detections за t миллисекунд
        detections get_detect(float t);

        //Записывает таблицу случайных чисел из вектора
        void TableRNG(vector<int> table);

        //Устанавливает или считывает статус DMA
        bool GetDMA() {return DMAStatus;};
        void SetDMA(bool status);

        //Записывает во внутренную память метода накопленные за t миллисекунд фреймы DMA
        void StoreDMA(double t, int argc, char** argv);
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

    detections board_if::get_detect(float t)
    {
        //Исходим из предположения, что плата не запущена и не включен DMA
        //Также исходим из предположения, что TableRNG уже сконфигурирована как надо
        //Инициализируем DMA
        top->DMAEnable();
        AnBRegInfo reg;
        reg.value.dma.enabled = 1;
        reg.address = AnBRegs::RegDMA;
        detections answer;
        top->RegRawWrite(reg);//Стартовал DMA
        
        clock_t start = clock();
        while ((clock()-start)/(CLOCKS_PER_SEC*1e-3) < t)
        {
            char *buf;
            top->DMARead(buf);
            //TODO: Сделать так, чтобы не парсить все отсчёты, а брать из буфера только те, которые сработали на Бобе
            answer.append(DMAFrame(buf).to_detections(type));
        }

        reg.value.dma.enabled = 0;
        top->RegRawWrite(reg);//Остановили DMA

        if (type) 
            answer.count.pop_back();//Удалим последний элемент, который используется для сшивки. Актуально только для Боба

        return answer;
    };

    void board_if::TableRNG(const std::vector<int> t)
    {
        using namespace std;
        
        //Сформируем буфер для записи в плату
        {
            size_t buf_size = t.size()/4 + ((t.size()%4)?1:0);
            char *buf = new char[buf_size];
            for (size_t i = 0; i < buf_size; i++) buf[i] = 0;

            for (size_t i = 0; i < t.size(); i++)
                buf[i/4] |= (t[i] & 0b11) << ((i % 4)*2);

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
                if (!top->DMAIsReady() & false) 
                {   
                    usleep(1000); 
                    continue;
                } else
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
            }
            SetDMA(false);
            cout << "Скорость: " << (stoi(argv[2])*(1<<17)/(clock() - start)*(float)CLOCKS_PER_SEC) << " Мсэмпл/сек" << endl;
        }
    }
};

#endif // !BOARD_IF_CPP