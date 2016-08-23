#include <string>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <iomanip>

const std::string URL1 = "http://127.0.0.1:77";
const std::string URL2 = "http://127.0.0.1:77";

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
        string cmd = "curl --data-ascii ";
        stringstream ss;
        //Отбросим некратные четырём биты с конца
        key.resize((key.size()/4)*4);
        //Теперь будем формировать по одному hex-символу и добавлять ко строке
        for (size_t i = 0; i < key.size()/4; i++)
        {
            int val = 0;
            for (size_t j = 0; j < 4; j++) val |= key[i*4 + j] << j;
            ss << setbase(16) << val;
        }
        cmd += ss.str();
        string cmd2 = cmd + ' ' + URL2;
        cmd += ' ' + URL1;
        system(cmd.c_str());
        system(cmd2.c_str());
        //cout << cmd << endl;
    }
}