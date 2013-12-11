#include <HAL/Devices/DeviceFactory.h>
#include "ProtoReaderLIDARDriver.h"

namespace hal
{

class ProtoReaderLIDARFactory : public DeviceFactory<LIDARDriverInterface>
{
public:
    ProtoReaderLIDARFactory(const std::string& name)
        : DeviceFactory<LIDARDriverInterface>(name)
    {
        Params() = {
        };
    }

    std::shared_ptr<LIDARDriverInterface> GetDevice(const Uri& uri)
    {
        ProtoReaderLIDARDriver* pDriver = new ProtoReaderLIDARDriver(uri.url);
        return std::shared_ptr<LIDARDriverInterface>( pDriver );
    }
};

// Register this factory by creating static instance of factory
static ProtoReaderLIDARFactory g_ProtoReaderLIDARFactory1("proto");
static ProtoReaderLIDARFactory g_ProtoReaderLIDARFactory2("log");

}
