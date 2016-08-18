#include <string>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

const std::string URL = "http://localhost4/qchannel/1/";

int main (void)
{
    using namespace std;

    //Создадим ключ с псевдослучайной последовательностью бит
    srand(time(NULL));
    //Размер ключа
    size_t size = rand()%2048;//Для определённости положим, что это будет наибольший размер ключа 
    vector<bool> key(size);
    for (int i = 0; i < key.size(); i++) key[i] = rand()%2;

    //Сформируем команду для отправки curl-запроса потребителю.
    //Ключ передаётся в шестнадцатеричном виде в видe ascii-текста
    //Если чисто бит не кратно четырём, то ненужные биты с конца отбрасываем -
    //невелика потеря
    {
        string cmd = "curl --data ";
        stringstream ss;
        //Отбросим некратные четырём биты с конца
        key.resize((key.size()/4)*4);
        //Теперь будем формировать по одному hex-символу и добавлять ко строке
        for (size_t i = 0; i < key.size()/4; i++)
        {
            uint8_t val = 0;
            for (size_t j = 0; j < 4; j++) val |= key[i*4 + j] << j;
            ss << hex << val;
        }
        cmd += ss.str();
        cmd += ' ' + URL;
        //system(cmd.c_str());
        cout << cmd << endl;
    }
}