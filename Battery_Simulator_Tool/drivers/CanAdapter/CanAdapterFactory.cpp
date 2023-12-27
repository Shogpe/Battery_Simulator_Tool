#include "CanAdapterFactory.h"

#include "CanAdapterChina.h"
#include "CanAdapterLawicel.h"
#include "CanAdapterLoopback.h"
#include "CanAdapterTesting.h"
#include "CanAdapterTritium.h"
#ifdef BUILD_PCAN
#include "CanAdapterPCAN.h"
#endif

QStringList CanAdapterFactory::getAdapterNames() {
    return QStringList() << "Loopback"
                         << "China"
                         << "Testing"
#ifdef BUILD_PCAN
                         << "PCAN"
#endif
        ;
}

CanAdapter *CanAdapterFactory::createAdapter(QString name, CanHub &canHub) {
    if (name == "Loopback") return new CanAdapterLoopback(canHub);
    if (name == "China") return new CanAdapterChina(canHub);
    if (name == "Testing") return new CanAdapterTesting(canHub);
#ifdef BUILD_PCAN
    if (name == "PCAN") return new CanAdapterPCAN(canHub);
#endif
    return 0;
}
