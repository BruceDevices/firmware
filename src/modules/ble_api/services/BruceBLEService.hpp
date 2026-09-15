#ifndef BRUCE_BLE_SERVICE_H
#define BRUCE_BLE_SERVICE_H
#if !defined(LITE_VERSION)
#include <NimBLEServer.h>

class BruceBLEService {
protected:
    NimBLEService *pService = nullptr;
    uint16_t mtu = 23; // default MTU size
public:
    virtual ~BruceBLEService() = default;
    virtual void setup(NimBLEServer *pServer) = 0;
    virtual void end() = 0;
    void setMTU(int new_mtu) { mtu = new_mtu; }
};
#endif
#endif // BRUCE_BLE_SERVICE_H
