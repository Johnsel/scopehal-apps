#include "../../lib/scopehal/scopehal.h"

#include "SCPILitePCIeTransport.h"
#include "LiteXOscilloscope.h"

void PluginInit() {
    AddTransportClass(SCPILitePCIeTransport);
    AddDriverClass(LiteXOscilloscope);
}