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
        void TableRNG(vector<unsigned short int> table);

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
            top->RegRawWrite(reg); 
        }

        //Установим в качестве источника ГСЧ TableRNG
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTest;
            top->RegRawRead(reg);
            reg.value.raw |= 0b1 << 31;//Отключил использование внешнего ГСЧ (LVDS) 
            reg.value.raw &= ~(0b1 << 30);//Включил использование таблицы TableRNG
            
            reg.value.raw &= ~(0b1 << 7);//Принимаем детектирования с настоящего детектора
            top->RegRawWrite(reg);
            if (type) bottom->RegRawWrite(reg);
        }

        //Установим dma enable в ноль
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegDMA;
            reg.value.dma.enabled = 0;
            top->RegRawWrite(reg);
            if (type) bottom->RegRawWrite(reg);
        }
        DMAStatus = false;
    }

    board_if::~board_if()
    {
        //Если Алиса - генерим stop condition
        if (!type) 
        {
            //Если Алиса
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTest;
            top->RegRawRead(reg);
            reg.value.raw &= ~0b01;//опускаем start-condition регистр на всякий случай
            reg.value.raw |= 0b10;//Поднимает регистр stop-condition
            top->RegRawWrite(reg);
            reg.value.raw &= ~0b10;//Опускаем регистр stop-condtition
            top->RegRawWrite(reg);
        }

        if (DMAStatus)
        {
            top->DMADisable();
            if (type) bottom->DMADisable();
        }
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

    void board_if::TableRNG(std::vector<unsigned short int> t)//TODO: Переделать в простой int
    {
        using namespace std;
        size_t buf_size = t.size()/4 + ((t.size()%4)?1:0);
        char *buf = new char[buf_size];
        for (size_t i = 0; i < buf_size; i++) buf[i] = 0;

        for (size_t i = 0; i < t.size(); i++)
            buf[i/4] += (t[i] >> (i % 4)) & 0b11;

        top->WriteTable(buf, buf_size, DestTables::TableRNG);
        if (type) bottom->WriteTable(buf, buf_size, DestTables::TableRNG);

        //Зафиксируем размер таблицы в регистрах ПЛИС
        {  
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTable;
            top->RegRawRead(reg);
            reg.value.table.size = t.size();
            top->RegRawWrite(reg);
            if (type) bottom->RegRawWrite(reg);
        }

        delete buf;
    }

    void board_if::SetDMA(bool status)
    {
        if (status == DMAStatus) return; 
        DMAStatus = status;

        cout << "Вкл/выкл DMA" << endl;
        if (status) top->DMAEnable();
        else top->DMADisable();

        cout << "Start/stop" << endl;
        //Дёрнем start/stop condition 
        if (!type) 
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTest;
            top->RegRawRead(reg);
           
            if (status)
            {
                //Включение
                reg.value.raw |= 0b01;//Поднимем start-бит
                reg.value.raw &= ~0b10;//Опустим stop-бит
                top->RegRawWrite(reg);
            } else
            {
                //Выключение
                reg.value.raw &= ~0b01;//Опускаем start-бит
                reg.value.raw |= 0b10;//Поднимаем stop-бит
                top->RegRawWrite(reg);
                reg.value.raw &= ~0b10;//Опустим stop-бит
                top->RegRawWrite(reg);
            }
        }

        //Дёрнем регистр dma.enabled не надо, т.к. это делается в модуле ядра

        cout << "Clear DMA" << endl;
        //Очистим буфер DMA в случае выключения
        if (!status)
        {
            char *buf;
            time_t finish = clock() + 100e-3*CLOCKS_PER_SEC;//Будем очищать всё, что приходит в буфер в течение 100 мс
            while (clock() < finish)
            if (top->DMAIsReady())
            {
                top->DMARead(buf);
                delete buf;
            }
        }
    }

    void board_if::StoreDMA(double t, int argc, char** argv)
    {

        if (true)
        {
            if (argc < 2) return;

            //Установка режима работы таблицы
            {
                AnBRegInfo reg;
                reg.address = AnBRegs::RegTable;
                top->RegRawRead(reg);
                reg.value.table.mode = stoi(argv[2]);
                top->RegRawWrite(reg);
            }
            
            //Заполнение TableRNG случайными числами
            if (true)
            {
                size_t s = stoi(argv[1]);
                char *buf = new char[s/4];
                for (int i = 0; i < s/4; i++) buf[i] = 0;
                for (int i = 0; i < s; i++)
                {
                    int v = random() % 4;
                    buf[i/4] += ((v & 0b11) << ((i % 4)*2));
                    //if (i % 4 == 0) cout << ' ';
                    //cout << (v & 0b11);
                }
                //cout << endl;
                top->WriteTable(buf, s/4, DestTables::TableRNG);
                
                AnBRegInfo reg;
                reg.address = AnBRegs::RegTable;
                top->RegRawRead(reg);
                reg.value.table.size = s;
                top->RegRawWrite(reg);
                delete buf;
            };

            top->SetBuffersCount(16);
            
            SetDMA(true);
            unsigned int seconds = 0;//Число секунд с момента старта
            unsigned int readed = 0;//Число прочитанных фреймов
            while (seconds < 10) 
            {

                char *buf;
                top->DMARead(buf);
                delete buf;
                readed++;

                if (false && top->DMAIsReady())
                {
                    char *buf = nullptr;
                    top->DMARead(buf);
                    if (buf != nullptr) delete buf;
                    
                    readed++;
                }
                
                if (clock() >= seconds*CLOCKS_PER_SEC)
                {
                    cout << readed << endl;
                    seconds++;
                }

                //if (seconds > 5) SetDMA(false);
            }
            
            SetDMA(false);
        }
    }
};

#endif // !BOARD_IF_CPP
