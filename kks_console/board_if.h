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
        void StoreDMA(double t, int argc, char** argv);
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

        if (status) top->DMAEnable();
        else top->DMADisable();

        /*AnBRegInfo reg;
        reg.address = AnBRegs::RegDMA;
        reg.value.dma.enabled = status;
        top->RegRawWrite(reg);*/
        
        DMAStatus = status;
    }

    void board_if::StoreDMA(double t, int argc, char** argv)
    {

        if (false)
        {
            SetDMA(true);
            clock_t start = clock();
            vector<char*> buffers;
            size_t count = 0;

            while((clock() - start)/(CLOCKS_PER_SEC*1e-3) < t)
            {
                char *buf;
                top->DMARead(buf);
                delete buf;
                count++;
                //cout << buffers.size() << endl;
            }
            cout << count << endl;

            SetDMA(false);
            for (auto i : buffers) delete i;
        }

        if (true)
        {
            if (argc < 2) return;
            AnBRegInfo reg;
            //reg.value.table.size = stoi(argv[1]);
            reg.value.table.size = 4096;
            reg.value.table.mode = stoi(argv[2]);
            reg.address = AnBRegs::RegTable;
            top->RegRawWrite(reg);
            
            unsigned long int a = 0xFEDCBA9876543210;
            
            if (false && !top->WriteTable((char*)&a, 8, DestTables::TableRNG))
                {cerr << "Cannot write TableRNG" << endl; return;};
            
            if (false)
            {
                vector<unsigned short> tmp;
                size_t s = stoi(argv[1]);
                for (unsigned short i = 0; i < s; i++) tmp[i] = (i % 4);
                TableRNG(tmp);
            }
            if (true)
            {
                size_t s = stoi(argv[1]);
                char *buf = new char[s/4];
                for (int i = 0; i < s/4; i++) buf[i] = 0;
                for (int i = 0; i < s; i++)
                {
                    int v = random() % 4;
                    buf[i/4] += ((v & 0b11) << ((i % 4)*2));
                    if (i % 4 == 0) cout << ' ';
                    cout << (v & 0b11);
                }
                cout << endl;
                top->WriteTable(buf, s/4, DestTables::TableRNG);
                reg.address = AnBRegs::RegTable;
                reg.value.table.size = s;
                top->RegRawWrite(reg);
                delete buf;
            };

            if (false)
            {
                char *table;
                unsigned int size_table;
                if (!top->ReadTable(table, size_table, DestTables::TableRNG))
                {
                    cerr << "Cannot read TableRNG" << endl;
                    cerr << top->LastError() << endl;
                    return;
                };
                for (int i = 0; i < 16; i++)
                    for (int j = 0; j < 4; j++)
                        cout << ((table[i] >> (2*j)) & 0b11);
                cout << endl;
                delete table;
            }

            if (false)
            {
                reg.address = AnBRegs::RegDMA;
                reg.value.dma.enabled = 1;
                top->RegRawWrite(reg);
                reg.value.dma.enabled = 0;
                top->RegRawWrite(reg);
                reg.value.dma.enabled = 1;
                top->RegRawWrite(reg);
            }
            top->SetBuffersCount(1);
            int buf_count = 32;
            char *buf[buf_count];
            for (int j = 0; j < 10; j++)
            {
                SetDMA(true);
                unsigned short int prev = UINT16_MAX;
                if (true)
                for (int i = 0; i < buf_count; i++) 
                {
                    top->DMARead(buf[i]);
                    if (((unsigned short int*)buf[i])[0] != prev && i != 0) cout << i << endl;
                    if (false)
                    {
                        size_t s = stoi(argv[1]);
                        char *buf_loc = new char[s/4];
                        for (int i = 0; i < s/4; i++) buf_loc[i] = 0;
                        for (int i = 0; i < s; i++)
                        {
                            int v = random() % 4;
                            buf_loc[i/4] += ((v & 0b11) << ((i % 4)*2));
                            if (i % 4 == 0) cout << ' ';
                            cout << (v & 0b11);
                        }
                        cout << endl;
                        SetDMA(false);
                        usleep(50000);
                        top->WriteTable(buf_loc, s/4, DestTables::TableRNG);
                        usleep(50000);
                        SetDMA(true);
                        delete buf_loc;
                    };
                    prev = ((unsigned short int*)buf[i])[0];
                }
                SetDMA(false);
                if (true)
                for (int i = 0; i < 2; i++)
                {
                    unsigned int *p = (unsigned int*)buf[i];
                    //cout << setbase(16) << a << " -> ";
                    //cout << "DMA:" << endl;
                    for (int i = 0; i < 1; i++)
                        //cout << setbase(16) << (p[i]) << "; ";
                    if (true)
                    {
                        for (int j = 0; j < 16; j+=2)
                            cout << ((p[i] >> j) & 0b11);
                        cout << endl;
                    } 
                    //cout << endl;
                }
                for (auto i : buf) delete i;
                if (true)
                {
                    size_t s = stoi(argv[1]);
                    char *buf = new char[s/4];
                    for (int i = 0; i < s/4; i++) buf[i] = 0;
                    for (int i = 0; i < s; i++)
                    {
                        int v = random() % 4;
                        buf[i/4] += ((v & 0b11) << ((i % 4)*2));
                        if (i % 4 == 0) cout << ' ';
                        cout << (v & 0b11);
                    }
                    cout << endl;
                    top->WriteTable(buf, s/4, DestTables::TableRNG);
                    reg.address = AnBRegs::RegTable;
                    reg.value.table.size = s;
                    top->RegRawWrite(reg);
                    delete buf;
                    reg.address = AnBRegs::RegDMA;
                    reg.value.dma.enabled = 1;
                    top->RegRawWrite(reg);
                    reg.value.dma.enabled = 0;
                    top->RegRawWrite(reg);
                    reg.value.dma.enabled = 1;
                    top->RegRawWrite(reg);
                };
                usleep(50000);
            }

            if (false)
            {
                cout << "TableRNG:" << endl;
                cout << top->GetDump(DumpSources::BAR1_RNG);
                if (false)
                for (int i = sizeof(a)*8/2 - 2; i >= 0 ; i-=2)
                    cout << ((a>>i) & 0b11);
                cout << endl;
            }
        }
    }


};

#endif // !BOARD_IF_CPP