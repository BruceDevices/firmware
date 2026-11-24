#ifndef BRUCE_MASS_STORAGE_H
#define BRUCE_MASS_STORAGE_H

#include <soc/soc_caps.h>

#if defined(SOC_USB_OTG_SUPPORTED)
class MassStorage {
public:
    MassStorage();
};
#endif

#endif
