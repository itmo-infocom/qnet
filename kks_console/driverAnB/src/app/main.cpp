#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "AnBDefs.h"
#include "driver.h"
#include "menu.h"

using namespace AnB;

static struct device_list devices;

static MenuItemUIntValue* regAddress;
static MenuItemUIntValue* regValue; 

static MenuItemIntValue* dmaLaunches;
static MenuItemIntValue* dmaTimer;  
static MenuItemIntValue* deviceSelect;  
static MenuItemIntValue* tableMode; 
static MenuItemIntValue* tableSize; 
static MenuItemIntValue* deviceMode; 
static MenuItemIntValue* buffersCount;

static MenuItemStrValue* fileName;

bool CheckDevice(int selected)
{
    if(selected < 0 || selected >= devices.count)
    {
        printf("The selected device doesn't exist (%d)\n", selected);
        return false;
    }
    
    if(devices.array[selected].dev_ref == nullptr)
    {
        printf("Null pointer (%d)\n", selected);
        return false;
    }
    
    return true;
}

long itemRegisterRead(unsigned long arg)
{
    int selected = deviceSelect->GetValue();
    
    if(!CheckDevice(selected))
        return 0;
    
    AnBDriverAPI* device = devices.array[selected].dev_ref;
    AnBRegInfo reg;
     
    reg.address = (AnBRegs)regAddress->GetValue();
    
    if(arg == 0)
    {
        if(device->RegRawRead(reg))
        {
            printf("Address: 0x%08x Hex value: 0x%08x Char value: |", reg.address, reg.value.raw);
            
            for(int i = 0; i < 4; i++)
            {
                if(((char*)&reg.value.raw)[i] >= 0x20 && ((char*)&reg.value.raw)[i] <= 0x7e)
                    printf("%c", ((char*)&reg.value.raw)[i]);
                else 
                    printf(".");
            }       
            
            printf("| Decimal value: %d\n", reg.value.raw);
        }
        else
        {
            printf("%s (0x%08x)\n", device->LastError(), reg.address);
            perror("Reason");
        }
    }
    else
    {
        reg.value.raw = regValue->GetValue();
        
        if(device->RegRawWrite(reg))
        {
            printf("Register was written\n");
            printf("Address: 0x%08x Hex value: 0x%08x Char value: |", reg.address, reg.value.raw);
            
            for(int i = 0; i < 4; i++)
            {
                if(((char*)&reg.value.raw)[i] >= 0x20 && ((char*)&reg.value.raw)[i] <= 0x7e)
                    printf("%c", ((char*)&reg.value.raw)[i]);
                else 
                    printf(".");
            }       
            
            printf("| Decimal value: %d\n", reg.value.raw);
        }
        else
        {
            printf("%s (0x%08x)\n", device->LastError(), reg.address);
            perror("Reason");
        }
    }
    return 0;
}

long itemListDevices(unsigned long arg)
{
    if(devices.array != nullptr && devices.count > 0)
    {
        printf("Device list.      ID                 NAME\n");

        for(int i = 0; i < devices.count; i++)
            printf("             %7d %20s\n", i, devices.array[i].dev_path);
    }
    else
        printf("Empty device list\n");

    return 0;
}

long itemDump(unsigned long arg)
{
    int selected = deviceSelect->GetValue();

    if(!CheckDevice(selected))
        return 0;

    AnBDriverAPI* device = devices.array[selected].dev_ref;    
    
    const char* dump = device->GetDump((DumpSources)arg);
    
    if(dump == nullptr)
    {
        printf("%s (0x%08x)\n", device->LastError(), arg);
        perror("Reason");
    }
    else
    {
        printf("%s", dump);
        free(const_cast<char*>(dump));
    }
    
    return 0;
}

long itemDMA(unsigned long arg)
{
    int selected = deviceSelect->GetValue();

    if(!CheckDevice(selected))
        return 0;
        
    AnBDriverAPI* device = devices.array[selected].dev_ref;    

    if(arg == 1)
    {
        if(device->RunDMATestCount(fileName->GetValue(), dmaLaunches->GetValue()))
        {
            printf("%s\n", device->LastError());
            perror("Reason");            
        }
        else
            printf("DMA test has been finished\n");
    }
    else if(arg == 2)
    {

    }
    
    return 0;
}

long itemPref(unsigned long arg)
{
    int selected = deviceSelect->GetValue();

    if(!CheckDevice(selected))
        return 0;

    AnBDriverAPI* device = devices.array[selected].dev_ref;        

    if(!device->SetDev((enum AnBDevices)deviceMode->GetValue()))
    {
        printf("%s\n", device->LastError());
        perror("Reason");            
    }
    else if(!device->SetTable((enum AnBTableModes)tableMode->GetValue(), tableSize->GetValue()))
    {
        printf("%s\n", device->LastError());
        perror("Reason");            
    }
    else if(!device->SetBuffersCount(buffersCount->GetValue()))
    {
        printf("%s\n", device->LastError());
        perror("Reason"); 
    }

    printf("Preferences were set successfuly\n");

    return 0;
}
                              
int main(int argc, char* const argv[])
{
    Menu menu;
    Menu registerMenu;
    Menu dumpsMenu;
    Menu dmaMenu;
    Menu prefMenu;

    memset(&devices, 0, sizeof(devices));
    
    if(!AnBDriverAPI::GetDevicesList(devices))
    {
        printf("There are no compatible devices presented in this system\n");
        return -1;
    }    

    menu.AddItem(                new MenuItem(           "List devices ", '1', itemListDevices));
    menu.AddItem((deviceSelect = new MenuItemIntValue(0, "Select device", '2', true, 0, devices.count - 1)));
    menu.AddItem(                new MenuItem(           "Preferences  ", '3', Menu::GetSubMenuAction(), reinterpret_cast<unsigned long>(&prefMenu)));
    menu.AddItem(                new MenuItem(           "Registers    ", '4', Menu::GetSubMenuAction(), reinterpret_cast<unsigned long>(&registerMenu)));
    menu.AddItem(                new MenuItem(           "Dumps        ", '5', Menu::GetSubMenuAction(), reinterpret_cast<unsigned long>(&dumpsMenu)));
    menu.AddItem(                new MenuItem(           "DMA          ", '6', Menu::GetSubMenuAction(), reinterpret_cast<unsigned long>(&dmaMenu)));
    menu.AddItem(                new MenuItem(           "Exit         ", '0', MenuItem::GetExitAction()));
    
    registerMenu.AddItem((regAddress = new MenuItemUIntValue(0, "Address", '1')));
    registerMenu.AddItem((regValue   = new MenuItemUIntValue(0, "Value  ", '2')));
    registerMenu.AddItem(              new MenuItem(            "Write  ", '3', itemRegisterRead, 1));
    registerMenu.AddItem(              new MenuItem(            "Read   ", '4', itemRegisterRead, 0));
    registerMenu.AddItem(              new MenuItem(            "Back   ", '0', MenuItem::GetExitAction()));
    
    dumpsMenu.AddItem(new MenuItem("Register bank        ", '1', itemDump, DumpSources::BAR0));
    dumpsMenu.AddItem(new MenuItem("RNG table            ", '2', itemDump, DumpSources::BAR1_RNG));
    dumpsMenu.AddItem(new MenuItem("DAC table            ", '3', itemDump, DumpSources::BAR1_DAC));
    dumpsMenu.AddItem(new MenuItem("DAC calibration table", '4', itemDump, DumpSources::BAR1_CAL)); 
    dumpsMenu.AddItem(new MenuItem("Back                 ", '0', MenuItem::GetExitAction()));        
    
    dmaMenu.AddItem((fileName    = new MenuItemStrValue("/home/snuffick/Photon/test", "Set output file      ", '1')));
    dmaMenu.AddItem((dmaLaunches = new MenuItemIntValue(1,                            "DMA launches         ", '2', true, 1, 0x7fffffff)));
    dmaMenu.AddItem((dmaTimer    = new MenuItemIntValue(1,                            "DMA timer (ms)       ", '3', true, 1, 0x7fffffff)));
    dmaMenu.AddItem(               new MenuItem(                                      "Run DMA test (number)", '4', itemDMA, 1));
    dmaMenu.AddItem(               new MenuItem(                                      "Run DMA test (timer) ", '5', itemDMA, 2));
    dmaMenu.AddItem(               new MenuItem(                                      "Back                 ", '0', MenuItem::GetExitAction()));        

    prefMenu.AddItem((tableMode    = new MenuItemIntValue(AnBTableModes::Mode1, "Table mode  (0 - 5)              ", '1', true, 0, AnBTableMode_Illegal - 1)));
    prefMenu.AddItem((tableSize    = new MenuItemIntValue(1,                    "Table size  (1 - 4096)           ", '2', true, 1, AnBTableMaxSize)));
    prefMenu.AddItem((deviceMode   = new MenuItemIntValue(AnBDevices::Bob,      "Device mode (1 - Alice, 0 - Bob) ", '3', true, 0, AnBDevice_Illegal - 1)));
    prefMenu.AddItem((buffersCount = new MenuItemIntValue(AnBBuffers,           "Buffers count                    ", '4', true, 2, 256)));
    prefMenu.AddItem(                new MenuItem(                              "Set preferences                  ", '5', itemPref));
    prefMenu.AddItem(                new MenuItem(                              "Back                             ", '0', MenuItem::GetExitAction()));

    itemPref(0);

    char table[] = {
        '\x00',
        '\x01',
        '\x02',
        '\x03',
        '\x04',
        '\x05',
        '\x06',
        '\x07',
        '\x08',
        '\x09',
        '\x0a',
        '\x0b',
        '\x0c',
        '\x0d',
        '\x0e',
        '\x0f'
    };
        
    int selected = deviceSelect->GetValue();

    if(!CheckDevice(selected))
        return -1;

    AnBDriverAPI* device = devices.array[selected].dev_ref; 

    if(!device->WriteTable(table, 16, DestTables::TableRNG))
    {
        printf("%s\n", device->LastError());
        perror("Reason");    

        return -1;
    }

    return menu.Call();
}
