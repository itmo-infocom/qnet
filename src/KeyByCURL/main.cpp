#include <string>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <iomanip>

int main(int ac, char *av[]) {
    using namespace std;

    if (ac <= 3) {
        fprintf(stderr, "Usage: %s KEY_SIZE KEYS_PER_MS [ctrl1ip:ctrl1port] [ctrl2ip:ctrl2port] ... [ctrlNip:ctrlNport]\n", av[0]);
        exit(EXIT_FAILURE);
    }
    size_t keysize = atoi(av[1]);
    if (keysize < 64) {
        fprintf(stderr, "KEYSIZE must be not less than 64\n");
        exit(EXIT_FAILURE);
    }
    float keyperms = atof(av[2]);
    time_t startTime = clock();
    time_t check = 0;
    //Создадим ключ с псевдослучайной последовательностью бит
    srand(time(NULL));
    while (1) //program main loop
    {
        time_t check = float(clock() - startTime) / CLOCKS_PER_SEC * 1000;
        if (check >= keyperms) {
            //Размер ключа
            vector<bool> key(keysize * 4);
            for (int i = 0; i < key.size(); i++) {
                key[i] = rand() % 2;
            }

            //Сформируем команду для отправки curl-запроса потребителю.
            //Ключ передаётся в шестнадцатеричном виде в видe ascii-текста
            //Если чисто бит не кратно четырём, то ненужные биты с конца отбрасываем -
            //невелика потеря

            string cmd = "curl --data-ascii ";
            stringstream ss;
            //Теперь будем формировать по одному hex-символу и добавлять ко строке
            for (size_t i = 0; i < key.size() / 4; i++) {
                int val = 0;
                for (size_t j = 0; j < 4; j++) val |= key[i * 4 + j] << j;
                ss << setbase(16) << val;
            }
            cmd += ss.str();
            //cout << cmd << endl;
            for (int i = 3; i < ac; i++) {
                system((cmd + ' ' + av[i]).c_str());
            }
            startTime = clock();
            check = 0;

            if (keyperms == 0) {
                break;
            }
        }
    }

}
