#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <csignal>

#include "jsonrpc.h"
#include <libmodbus/modbus.h>
#include "rpc.h"
#include "cfg.h"
#include "modbus_client.h"
#include "system.h" 
#include "update.h" 

using namespace system_util;

DWORD WINAPI ThreadFunc(LPVOID lpParam) {
    while (1)
    {
        std::cout << "Hello from the new thread!" << std::endl;
        Sleep(500);
    }
    return 0;
}

void creat_thread(void)
{
    HANDLE hThread;
    DWORD dwThreadId;
    std::cout << "This is main thread " << std::endl;
    hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &dwThreadId);
    std::cout << "new thread id == "<< dwThreadId << std::endl;
    if (hThread == NULL) {
        std::cerr << "Failed to create thread!" << std::endl;
    }
}

/**
 * \class MyClass
 * \brief Example class.
 */
void UpdateThread::StartUpdate(const char *file, const char *port_info, int channel, int dev_addr)
{
    void* ret = NULL;
    PUpdateInfo ptUpdateInfo;
    static int cnt = 0;

    std::cout << "StartUpdate to create thread(bRunning = "<< bRunning <<") cnt = "<< cnt << std::endl;

    if (bRunning)
        return;

    iPercent = 0;
    bRunning = 1;

    ptUpdateInfo = &tUpdateInfo; // new UpdateInfo();
    
    memset(ptUpdateInfo, 0, sizeof(UpdateInfo));

    strncpy_s(ptUpdateInfo->file, file, sizeof(ptUpdateInfo->file)-1);
    strncpy_s(ptUpdateInfo->port_info, port_info, sizeof(ptUpdateInfo->port_info)-1);
    ptUpdateInfo->channel = channel;
    ptUpdateInfo->dev_addr = dev_addr;

    std::cout << "to create arg ..." << std::endl;
    ThreadArg* arg = new ThreadArgImpl<UpdateThread>(*this,
    &UpdateThread::MethodForUpdateThread, ptUpdateInfo);

    std::cout << "to create Thread ..." << std::endl;
    Thread *th = new Thread(arg);

    std::cout << "to run Thread ..." << std::endl;
    th->Start(false);
    //th->Join(&ret);

    std::cout << "StartUpdate ..." << std::endl;
}


/**
 * \brief Method executed in thread.
 * \param arg argument
 * \return
 */
void* UpdateThread::MethodForUpdateThread(void* arg)
{
    size_t i = 0;
    PUpdateInfo ptUpdateInfo = (PUpdateInfo)arg; 
    int err;

    std::cout << "to call modbus_update (cur bRunning = "<< bRunning << ")" << std::endl;
    
    iPercent = 0;
    
    err = modbus_update(ptUpdateInfo->file, ptUpdateInfo->port_info, ptUpdateInfo->channel, ptUpdateInfo->dev_addr, local_set_update_percent);
    if (err)
    {
        local_set_update_percent(-1);
    }
    
    bRunning = 0;

    //delete ptUpdateInfo;

    return NULL;
}

int UpdateThread::GetUpdatePercent(void) 
{
    return iPercent;
}

void UpdateThread::SetUpdatePercent(int percent)
{
    iPercent = percent;
}


int UpdateThread::isRunning(void) 
{
    return bRunning;
}


