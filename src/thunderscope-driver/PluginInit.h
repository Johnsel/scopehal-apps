#include "../../lib/scopehal/scopehal.h"

#include "SCPILitePCIeTransport.h"
//#include "LiteXOscilloscope.h"


void TransportStaticInit();
void DriverStaticInit();

extern "C" __declspec( dllexport ) void PluginInit();