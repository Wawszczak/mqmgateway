#include "plugin.hpp"
#include "expr.hpp"
#include <memory>

IStateConverter*
StdConvPlugin::getStateConverter(const std::string& name) {
    if(name == "evaluate") {
        return new ExprtkConverter();
    }
    return nullptr;
}

extern "C" std::shared_ptr<StdConvPlugin> ConverterFactory()
{
	return std::shared_ptr<StdConvPlugin>(new StdConvPlugin);
}
