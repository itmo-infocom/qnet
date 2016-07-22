#ifndef SNU_MENU_H
#define SNU_MENU_H

class MenuItem;
template<typename T> class MenuItemValue;
template<typename T> long SetValue(MenuItemValue<T>* const ref);

typedef long (*itemAction_t) (unsigned long);
typedef MenuItemValue<unsigned int> MenuItemUIntValue;
typedef MenuItemValue<int>          MenuItemIntValue;
typedef MenuItemValue<const char*>  MenuItemStrValue;

class Menu {
public:
    static itemAction_t GetSubMenuAction();
    static long CallMenu(Menu* const Menu);

    Menu();
    ~Menu();

    int Call();

    int Display();
    int Process();
    
    int AddItem(const MenuItem* item);
    
    void RemoveItems();

private:
    MenuItem**   items;
    unsigned int items_count;
};

class MenuItem {
public:
    static itemAction_t GetExitAction();

    MenuItem(const char* title = "MenuItem", char key = 0, itemAction_t action = nullptr, unsigned long arg = 0);
    
    void SetTitle(const char* title);
    void SetKey(char key);
    
    char GetKey();
    
    virtual void SetAction(itemAction_t action = nullptr, unsigned long arg = 0);
    virtual void Display();

    int InvokeAction();

protected: 
    itemAction_t  action;    
    unsigned long action_arg;
    
    const char* title;
    char        key;
    
private:
    static const char* default_title;
    static const char  default_key;  
};

template<typename T>
class MenuItemValue : public MenuItem {
public:
    MenuItemValue(T value, const char* title = "MenuItem", char key = 0, bool restricted = false, T minVal = 0, T maxVal = 0);

    T GetValue();
    
    virtual void Display() final;
        
private:
    virtual void SetAction(itemAction_t action = nullptr, unsigned long arg = 0) final;

    T value;    
    
    const T maximum;
    const T minimum;
    
    bool restricted;
    
    friend long SetValue<T>(MenuItemValue<T>* const ref);
};

#endif
