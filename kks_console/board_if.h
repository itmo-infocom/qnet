//Класс, реализующий безопасный интерфейс для работы с платой

#ifndef BOARD_IF_CPP
#define BOARD_IF_CPP

#include <string>
#include <time.h>
#include <iostream>

#include "./driverAnB/devel/AnBDefs.h"
#include "./driverAnB/devel/driver.h"
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
        enum reg_map
        {
            // Board Mode Register
            // bit0 - '0' => Alice, '1' => Bob
            // bit[7:4] - Table Mode
            // bit[23:8] - Table Size
            // bit[25:24] - mem select '00' -> rnd, '01' -> DAC, '10' - DAC CALIB, '11' -> calib
            //LVDS_PCI_REG_MODE 0x08,
            
            // STB delay Register
            AB_STB_DELAY_REG = 0x20,
            
            // Start pattern Register
            AB_PATTERN_REG = 0x28,
            
            LVDS_PCI_INT_DMA = 0x02,
            LVDS_CALIBR_RDY = 0x04,
            
            AB_CALIBRMAX_REG = 0x50,
            LVDS_CALIBR_MAX_MASK = 0xFF,
            LVDS_CALIBR_REVERS = 0x80000000
        };

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
        void StoreDMA(size_t t);
    };

    board_if::board_if() throw(except)
    {
        if (!AnBDriverAPI::GetDevicesList(devices))
            throw except("GetDeviceList");

        //Определим какого типа плата у нас
        AnBRegInfo reg;
        reg.address = AnBRegs::RegMode;
        if (!devices.array[0].dev_ref->RegRawRead(reg))
        throw except("RegRawRead");

        type = reg.value.mode.mode;
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
        DMAStatus = false;
    }

    board_if::~board_if()
    {
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
        size_t buf_size = t.size()/4 + (t.size()%4)?1:0;
        char *buf = new char[buf_size];
        for (size_t i = 0; i < buf_size; i++) buf[i] = 0;

        for (size_t i = 0; i < t.size(); i++)
        buf[i/4] |= (t[i] >> i%4) & 0b11;

        top->WriteTable(buf, buf_size, DestTables::TableRNG);
        if (type) bottom->WriteTable(buf, buf_size, DestTables::TableRNG);

        //Зафиксируем размер таблицы в регистрах ПЛИС
        {  
            AnBRegInfo reg;
            reg.address = AnBRegs::RegTable;
            reg.value.table.mode = AnBTableModes::Mode6;
            reg.value.table.size = t.size();
            top->RegRawWrite(reg);
            if (type) bottom->RegRawWrite(reg);
        }

        delete buf;
    }

    void board_if::SetDMA(bool status)
    {
        if (status == DMAStatus) return; 

        if (status) top->DMAEnable();
        else top->DMADisable();

        /*AnBRegInfo reg;
        reg.address = AnBRegs::RegDMA;
        reg.value.dma.enabled = status;
        top->RegRawWrite(reg);*/
        
        DMAStatus = status;
    }

    void board_if::StoreDMA(size_t t)
    {
        if (!DMAStatus) top->DMAEnable();

        //Дёрнем регистром DMA, чтобы ПЛИС начала вещание
        if (false)
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegDMA;
            reg.value.dma.enabled = 1;
            top->RegRawWrite(reg);            
        }
        
        clock_t start = clock();
        vector<char*> buffers(t);

        for (size_t i = 0; i < t; i++)
        {
            char *buf;
            //if (!top->DMAIsReady()) continue;
            top->DMARead(buffers[i]);
            cout << i << endl;
        }

        top->DMADisable();
    }
};

#endif // !BOARD_IF_CPP