#include "rpc.h"
/**
 * \class MyClass
 * \brief Example class.
 */
class UpdateThread
{
  public:
    /**
     * \brief Method to create thread.
     */
    void StartUpdate(const char *file, const char *port_info, int channel, int dev_addr);

    /**
     * \brief Method executed in thread.
     * \param arg argument
     * \return return 0xABCD
     */
    void* MethodForUpdateThread(void* arg)    ;

    int GetUpdatePercent(void) ;
    void SetUpdatePercent(int percent) ;
    
    int isRunning(void) ;

private:
    int iPercent;
    int bRunning;
    UpdateInfo tUpdateInfo;
};


