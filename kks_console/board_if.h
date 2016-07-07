//Класс, реализующий безопасный интерфейс для работы с платой

#ifndef BOARD_IF_CPP
#define BOARD_IF_CPP

#include <string>

#include "./driverAnB/devel/AnBDefs.h"
#include "./driverAnB/devel/driver.h"

namespace board_if
{
using namespace AnB;
using namespace std;

    class board_if: public AnBDriverAPI
    {
        bool type;//Тип устройства
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
    public:
        board_if();
        ~board_if();
    };

    board_if::board_if()
    {
        try {
            if (true || !GetDevicesList(devices))
                throw std::string(LastError());
        } catch(...) {throw;}

        //Определим какого типа плата у нас
        AnBRegInfo reg;
        reg.address = AnBRegs::RegMode;
        devices.array[0].dev_ref->RegRawRead(reg);
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
        reg.address = AnBRegs::RegDMA;
        top->RegRawRead(reg);
        reg.value.dma.enabled = 0;
        top->RegRawWrite(reg);
        if (type) bottom->RegRawWrite(reg);

        //
    }

    board_if::~board_if()
    {
    }
};

#endif // !BOARD_IF_CPP