#ifndef DRIVER_H
#define DRIVER_H

#include <pthread.h>

namespace AnB {

    class AnBDriverAPI;
    
    enum Errors {
        Success,           //0
        NoDevDir,          //1
        DevAlloc,          //2
        NoDevs,            //3
        Open,              //4
        Null,              //5
        Ill,               //6 
        DmaEnabled,        //7  
        DmaEnabled_r,      //8
        DmaDisabled,       //9
        DmaMap,            //10
        DmaThread,         //11
        DmaJoinThread,     //12
        Disabled,          //13
        RegRead,           //14
        RegWrite,          //15
        DumpSource,        //16
        DestSource,        //17
        DeviceMode,        //18
        TableRead,         //19
        TableWrite,        //20
        TableSize,         //21
        TableMode,         //22
        BufferCount,       //23
        DumpError,         //24
        DumpError_file,    //25

        Errors_MAX,
    };

    struct device_id {
        mutable char   const* dev_path;
        mutable AnBDriverAPI* dev_ref;
    };

    struct device_list {
        ~device_list();

        const struct device_id* array;
        unsigned int count;
    };

    struct error {
        enum Errors type;
        int         errn;
    };

    class AnBDriverAPI {
    public: 
        static bool GetDevicesList(struct device_list& devices);

        static void GlobalError(struct error& e);
        
        static const char* ErrorStr(const struct error& e);
        
        ~AnBDriverAPI();
            
        bool RegRawRead(AnBRegInfo& reg);
        bool RegRawWrite(const AnBRegInfo& reg);
        
        const char* GetDump(enum DumpSources source);

        const char* LastError();
        void LastError(struct error& e);
        
        bool WriteTable(const char* table, unsigned int size, enum DestTables dest);
        bool ReadTable(char*& table, unsigned int& size, enum DestTables dest);
      
        bool SetDev(enum AnBDevices mode);
        bool SetTable(enum AnBTableModes mode, unsigned int size);

        bool SetBuffersCount(unsigned int count);
        
        bool DMAEnable();        
        bool DMADisable();
        bool DMARead(char*& buffer);
        bool DMAIsReady(); 
        
        int RunDMATestCount(const char* file, unsigned int count);

    protected:
        AnBDriverAPI();
        
        bool OpenDevice(const char* name = nullptr);

    private:
        static struct device_list devices;
        static struct error       global_error;
         
        static bool searchDevices();

        int             dev_fd;
        struct error    dev_error;
        pthread_t       dev_thread;
        bool            dev_live;
        pthread_mutex_t dev_mutex;
        pthread_cond_t  dev_cond_ready;
        pthread_cond_t  dev_cond_read;

        struct dma_region {
            struct chunk {
                char data[AnBBufferMinSize];
            } chunks[AnBBuffers];
        } *dev_dma_region;

        bool  dev_ready;
        void* dev_buffer;
        int   dev_dma_chunk;
        int   dev_dma_buffers;

        const char* generateDump();    

        void unmap_all();

        friend long taskDMA(AnBDriverAPI* const device);        
    };
}

#endif
