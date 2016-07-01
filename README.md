# qnet
Quantum network

qcrypt -- test network settings

src -- coders/decoders

doc -- documentation

--------------------------------------
--------------Полезное----------------
Чтобы коммитить изменения:
git commit -m "<комментарий>" - коммит изменений
git rm <имя файла> - удаление файла из системы контроля версий и рабочего
каталога

vimdot - для рисования графов. Чтобы обновить - нужно записать файл :w

-------------------------------------
Драйвер AnB

Чтобы установить - rpm -Uvh driverAnB*
Затем перенести папку AnB из /tmp:
sudo mv /tmp/AnB ~/kks_v2/kks_console/driverAnB
И поправить права на использования (он же был установлен root-ом):
cd ~/kks_v2/kks_console/driverAnB/
sudo chown -R avasilev ./AnB/
