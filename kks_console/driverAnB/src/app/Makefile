include $(PWD)/Makefile.inc

CPP_FLAGS += -std=c++1y
CPP_FLAGS += -c

CPP_FLAGS_LIB += $(CPP_FLAGS)
CPP_FLAGS_LIB += -pthread

LD_FLAGS += -pthread

OBJS += menu.o
OBJS += main.o

LIB_LIST = 
LIB_PATH = 

all: $(APP_NAME)
	
$(APP_NAME): $(APP_NAME).o
	g++ $(LD_FLAGS) $(APP_NAME).o -o $(APP_NAME)

$(APP_NAME).o: $(OBJS) 
	ld -r $(OBJS) $(LIB_LIST) -o $(APP_NAME).o

main.o: main.cpp
	g++ $(CPP_FLAGS) -I$(LIB_PATH) main.cpp

menu.o: menu.cpp menu.h 
	g++ $(CPP_FLAGS) menu.cpp

clean:
	rm -f *.o $(APP_NAME) 
    
install:
	echo "Nothing to install"
    
remove:
	echo "Nothing to remove"
        
