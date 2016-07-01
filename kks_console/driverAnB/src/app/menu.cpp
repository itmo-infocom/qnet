#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "menu.h"

static long subMenu(Menu* const menu)
{
    int menu_result = 0;
    
    while(menu_result == 0)
    {   
        if((menu_result = menu->Display()) != 0)
            printf("An empty menu\n");
        else
            menu_result = menu->Process(); 
    }

    return 0;
}

itemAction_t Menu::GetSubMenuAction()
{
    return reinterpret_cast<itemAction_t>(subMenu);
}

long Menu::CallMenu(Menu* const menu)
{
    return subMenu(menu);
}

Menu::Menu()
{
    this->items       = nullptr;
    this->items_count = 0;
}

Menu::~Menu()
{
    this->RemoveItems();
}

int Menu::Call()
{
    return subMenu(this);
}

int Menu::Display()
{
    if(this->items == nullptr || this->items_count == 0)
        return -1;
        
    printf("\n");                
        
    for(int i = 0; i < this->items_count; i++)
        this->items[i]->Display();
        
    printf("\n");        
        
    return 0;        
}

int Menu::Process()
{
    char key;

    if(this->items == nullptr || this->items_count == 0)
        return -1;
        
    printf("Select command: ");        
        
    key = getchar();  
    
    printf("\n");   

    if(key != '\n')
        while(getchar() != '\n') { } 
    else
        return 0;
        
    for(int i = 0; i < this->items_count; i++)
        if(this->items[i]->GetKey() == key)
            return this->items[i]->InvokeAction();
    
    printf("Illegal command: %c (0x%02x)\n", key, (unsigned char)key);
    
    return 0;
}
   
int Menu::AddItem(const MenuItem* item)
{
    if(item == nullptr)
        return -1;

    MenuItem** temp = this->items;
    
    temp = reinterpret_cast<MenuItem**>(malloc((this->items_count + 1) * sizeof(item)));
    
    if(temp == nullptr)
        return -1;
    
    if(this->items != nullptr && this->items_count > 0)
    {
        memcpy(temp, this->items, this->items_count * sizeof(item));
        free(this->items);
    }
    
    temp[this->items_count] = const_cast<MenuItem*>(item);
    
    this->items_count += 1;
    
    this->items = temp;
    
    return 0;    
}

void Menu::RemoveItems()
{
    if(this->items != nullptr && this->items_count > 0)
    {
        free(this->items);
        
        this->items       = nullptr;
        this->items_count = 0;
    }
}

const char* MenuItem::default_title = "MenuItem";
const char  MenuItem::default_key   = 0;

static long exitAction(unsigned long arg)
{
    return -1;
}

itemAction_t MenuItem::GetExitAction()
{
    return exitAction;
}

MenuItem::MenuItem(const char* title, char key, itemAction_t action, unsigned long arg)
{
    this->title = title;
    this->key   = key;
    
    this->SetAction(action, arg);
}
    
void MenuItem::SetAction(itemAction_t action, unsigned long arg)
{
    this->action     = action;
    this->action_arg = arg;
}

void MenuItem::SetTitle(const char* title)
{
    this->title = title;
}

void MenuItem::SetKey(char key)
{
    this->key = key;
}
    
char MenuItem::GetKey()
{
    return this->key;
}
    
void MenuItem::Display()
{
    printf("%c. %s\n", this->key, this->title);
}

int MenuItem::InvokeAction()
{
    if(this->action == nullptr)
    {
        printf("No action for this item was set\n");
        return 0;
    }    
        
    return this->action(this->action_arg);
}

template<typename T>
MenuItemValue<T>::MenuItemValue(T value, const char* title, char key, bool restricted, T minVal, T maxVal) :
                  MenuItem(title, key, (itemAction_t)SetValue<T>, reinterpret_cast<unsigned long>(this)), 
                  minimum(minVal),
                  maximum(maxVal)
{
    this->value      = value;
    this->restricted = restricted;
}

template<typename T>
T MenuItemValue<T>::GetValue()
{
    return this->value;
}

template<typename T>
void MenuItemValue<T>::SetAction(itemAction_t action, unsigned long arg)
{
//    this->action     = action;
//    this->action_arg = reinterpret_cast<unsigned long>(this);
}

template class MenuItemValue<unsigned int>;
template class MenuItemValue<int>;
template class MenuItemValue<const char*>;

template<> void MenuItemValue<unsigned int>::Display()
{
    printf("%c. %s (value: 0x%08x)\n", this->key, this->title, this->value);
}    

template<> void MenuItemValue<int>::Display()
{
    printf("%c. %s (value: %d)\n", this->key, this->title, this->value);
}    

template<> void MenuItemValue<const char*>::Display()
{
    printf("%c. %s (value: %s)\n", this->key, this->title, this->value);
}    

template<> long SetValue<unsigned int>(MenuItemValue<unsigned int>* const ref)
{
    int temp   = 0;
    
    printf("Enter an unsigned integer value: ");
    
    if(fscanf(stdin, "%i", &temp) != 1)
    {
        printf("Illegal string was given\n");
        
        goto exit;
    }
    
    if(ref->restricted && (temp < ref->minimum || temp > ref->maximum))
    {
        printf("A value should be between 0x%08x (%d) and 0x%08x (%d)\n", ref->minimum, ref->minimum, ref->maximum, ref->maximum);
        
        goto exit;
    }
    
    ref->value = (unsigned int)temp;
    
exit:
    while(getchar() != '\n') { }
    
    return 0;
}

template<> long SetValue<int>(MenuItemValue<int>* const ref)
{
    int temp   = 0;
    
    printf("Enter an integer value: ");
    
    if(fscanf(stdin, "%i", &temp) != 1)
    {
        printf("Illegal string was given\n");
        
        goto exit;
    }
    
    if(ref->restricted && (temp < ref->minimum || temp > ref->maximum))
    {
        printf("A value should be between %d and %d\n", ref->minimum, ref->maximum);
        
        goto exit;
    }
    
    ref->value = (int)temp;
    
exit:
    while(getchar() != '\n') { }
    
    return 0;
}

template<> long SetValue<const char*>(MenuItemValue<const char*>* const ref)
{
    char* temp = nullptr;
    
    printf("Enter a string: ");
    
    if(fscanf(stdin, "%ms", temp) != 1)
    {
        printf("Illegal string was given\n");
        
        goto exit;
    }
    
    ref->value = temp;
    
exit:
    while(getchar() != '\n') { }
    
    return 0;
}