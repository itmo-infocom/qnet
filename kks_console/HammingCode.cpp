//
//  HammingCode.cpp
//  HammingCode
//
//  Created by Никита Булдаков on 02.07.16.
//  Copyright © 2016 Никита Булдаков. All rights reserved.
//


#include <vector>

using namespace std;

vector<bool> HammingCode(vector<bool> key);
vector<bool> HammingDecode(vector<bool> key, vector <bool> cbit_storage_input);


vector <bool> HammingCode(vector <bool> key) //На вход подается ансамбль бит для обработки, функция возвращает контрольные биты
   {
      unsigned int i=0; //Счетчик
      unsigned int pw=1; //Туточки переменная для расчета степеней. Кек.
      vector <bool> cbit_output; //Вектор для накопления контрольных бит
       
   //Процесс инициализации контрольных бит
       while ((pw<<i)<key.size())
         {
            key.insert(key.begin()+(pw<<i)-1,0);
            i++;
         }
   ////>
    
       
   //Процесс расчета контрольных бит
      int st=0,j=0; //st - счетчики
      i=0;
      while ((pw<<st)<=key.size()) //Расчет четности контроллируемых бит. Каждный 2^st контрольный бит, контроллирует последовательности по 2^st бит через каждые 2^st бит
         {
            int counter=0;
            while (i<=key.size()-1)
               {
                  j=0;
                     while ((j<(pw<<st)) && (j+i<=key.size()))
                        {
                           if (key[i+j]==1)
                              counter=counter+1;
                           j++;
                        }
                     i=i+2*(pw<<st);
               }
            if (counter%2!=0)   //Четное число контроллируемых бит -> Контрольный бит = 0.  Не четное число контроллируемых бит -> Контрольный бит = 1.
               key[(pw<<st)-1]=1;
            st++;
            i=(pw<<st)-1;
         }
   ////>
      
       
   //Передача контрольных бит на вывод
      i=0;
      while ((pw<<i)<key.size())
         {
            cbit_output.push_back(key[(pw<<i)-1]);
            i++;
         }
                  
      return cbit_output;
   }


vector <bool> HammingDecode(vector <bool> key, vector <bool> cbit_storage_input)
{
               unsigned int i=0,st=0;
               unsigned int pw=1; //Туточки переменная для расчета степеней. Кек.
               unsigned int j=0, pos=0; //pos-позиция ошибочного бита
               vector <bool> cbit_storage_output;
               
   //Инициализация контрольных бит
               while (pow(2,i)<key.size())
                  {
                     key.insert(key.begin()+pow(2,i)-1,0);
                     i++;
                  }
   //Расчет контрольных бит
               st=0;
               i=0;
               while ((pw<<st)<=key.size()) //Расчет четности контроллируемых бит. Каждный 2^st контрольный бит, контроллирует последовательности по 2^st бит через каждые 2^st бит
               {
                   unsigned int counter=0;
                   while (i<=key.size()-1) {
                       j=0;
                       while ((j<(pw<<st)) && (j+i<=key.size())){
                           if (key[i+j]==1)
                               counter=counter+1;
                           j++;
                       }
                       i=i+2*(pw<<st);
                   }
                   if (counter%2!=0)
                       key[(pw<<st)-1]=1;
                   st++;
                   i=(pw<<st)-1;
               }

    //Занесение контрольных бит в вектор, который будет использоваться для сравнения
               i=0;
               st=0;
               while ((pw<<i)<key.size())
                  {
                     cbit_storage_output.push_back(key[(pw<<i)-1]);
                     i++;
                  }
    //Расчет позиции ошибочного бита
               pos=0;
               for (i=0; i<cbit_storage_input.size(); i++)
                  {
                     if (cbit_storage_input[i]!=cbit_storage_output[i])
                     {
                         pos=pos+(pw<<i);
                     }
                  }
               
               
               //Инвертирование ошибочного бита
               if (pos!=0) //Если нашлись ошибочные биты, то происходит инвертирование
               {
                   if (key[pos-1]==1)
                       key[pos-1]=0;
                   else
                       key[pos-1]=1;
               }
    
               //Удаление контрольных бит
               i=0; //i-счетчик, st-счетчик степеней (2^st) для определения индекса контрольного бита
               while ((pw<<i)<(key.size()))
               {
                   key.erase(key.begin()+(pw<<i)-1-i);
                   i++;
               }
               cout << endl;
               for (int i=0; i<key.size(); i++)
               {
                   cout << key[i] << " ";
               }
    return key;
}
