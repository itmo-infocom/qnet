//Класс, реализующий безопасный интерфейс для работы с платой
#ifdef BOARD_IF_CPP
#define BOARD_IF_CPP

#include "./driverAnB/devel/AnBDefs.h"
#include "./driverAnB/devel/driver.h"

namespace board_if
{
    class board_if: public AnB::AnBDriverdAPI
    {
        
    public:
        board_if();
        ~board_if();
    };
};

#endif // !BOARD_IF_CPP