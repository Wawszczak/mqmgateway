
#include "plugin.hpp"

#include "divide.hpp"
#include "int32.hpp"
#include "bitmask.hpp"
#include "scale.hpp"
#include <memory>

IStateConverter*
StdConvPlugin::getStateConverter(const std::string& name) {
    if (name == "divide")
        return new DivideConverter();
    else if (name == "int32")
        return new Int32Converter();
    else if (name == "bitmask")
        return new BitmaskConverter();
    else if (name == "scale")
        return new ScaleConverter();
    return nullptr;
}

extern "C" std::shared_ptr<StdConvPlugin> ConverterFactory()
{
	return std::shared_ptr<StdConvPlugin>(new StdConvPlugin);
}
