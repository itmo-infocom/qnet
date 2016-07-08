//Класс, реализующий безопасный интерфейс для работы с платой

#ifndef BOARD_IF_CPP
#define BOARD_IF_CPP

#include <string>
#include <time.h>
#include <iostream>

#include "./driverAnB/src/lib/AnBDefs.h"
#include "./driverAnB/src/lib/driver.h"
#include "DMAFrame.h"

namespace board_if
{
using namespace AnB;
using namespace std;

    class board_if: protected AnBDriverAPI
    {
        bool type;//Тип устройства
        bool DMAStatus;
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

        std::vector<DMAFrame> DMAFrames;
    public:
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
        void StoreDMA(double t);
    };

    board_if::board_if() throw(except)
    {
        if (!GetDevicesList(devices))
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

        //Принудительно выключим DMA регистром ПЛИС
        reg.value.dma.enabled = 0;
        reg.address = AnBRegs::RegDMA;
        if (!top->RegRawWrite(reg)) throw except("RegRawWrite/top");
        if (type) 
            if (!bottom->RegRawWrite(reg)) throw except("RegRawWrite/bottom");
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
        reg.value.raw = 0;
        top->RegRawWrite(reg);//Остановили DMA

        if (type) 
            answer.count.pop_back();//Удалим последний элемент, который используется для сшивки. Актуально только для Боба

        return answer;
    };

    void board_if::TableRNG(std::vector<unsigned short int> t)
    {
        using namespace std;
        size_t buf_size = t.size()/4 + (t.size()%4)?1:0;
        char *buf = new char[buf_size];
        for (size_t i = 0; i < buf_size; i++) buf[i] = 0;
        for (size_t i = 0; i < t.size(); i++)
        buf[i/4] |= (t[i] >> i%4) & 0b11;

        top->WriteTable(buf, buf_size, DestTables::TableRNG);
        if (type) bottom->WriteTable(buf, buf_size, DestTables::TableRNG);

        delete buf;
    }

    void board_if::SetDMA(bool status)
    {
        if (status == DMAStatus) return; 

        if (status) top->DMAEnable();
        else top->DMADisable();

        AnBRegInfo reg;
        reg.address = AnBRegs::RegDMA;
        reg.value.dma.enabled = status;
        top->RegRawWrite(reg);
        
        DMAStatus = status;
    }

    void board_if::StoreDMA(double t)
    {
        //Дёрнем регистром DMA, чтобы ПЛИС начала вещание
        {
            AnBRegInfo reg;
            reg.address = AnBRegs::RegDMA;
            reg.value.dma.enabled = 1;
            top->RegRawWrite(reg);
            reg.value.dma.enabled = 0;
            top->RegRawWrite(reg);
        }
        
        if (!DMAStatus) top->DMAEnable();

        clock_t start = clock();
        vector<char*> buffers;

        while ((start-clock())/(CLOCKS_PER_SEC*1e-3) < t)
        {
            buffers.push_back(nullptr);
            char *buf;
            top->DMARead(buf);
            buffers.push_back(buf);
            cout << buffers.size() << endl;
        }

        top->DMADisable();
    }
};

#endif // !BOARD_IF_CPP