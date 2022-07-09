#include <iostream>

//#include <boost/program_options.hpp>
//#include <boost/filesystem.hpp>
#include "libmodmqttsrv/common.hpp"
#include "libmodmqttsrv/modmqtt.hpp"
#include "config.hpp"

//namespace args = boost::program_options;
using namespace std;
using namespace modmqttd;

modmqttd::ModMqtt server;

int main(int ac, char* av[]) {
//    std::shared_ptr<boost::log::sources::severity_logger<modmqttd::Log::severity>> log;
     // TODO add version information
     cout << Log::severity::info << "modmqttd is starting" << endl;

    int LogLevel = 1;
    char* LOG_LEVEL  = NULL;
    if(LOG_LEVEL = std::getenv("LOG_LEVEL"))
    {
        LogLevel = atoi(LOG_LEVEL);
	cout << Log::severity::info << "Using LogLevel from Environment: " << LogLevel << endl;
    }
    else
	cout << Log::severity::info << "Using Default LogLevel: 1 [Critical]\n";
    std::string configPath("/etc/mqmgateway.yaml");
    if(const char* CONFIG_PATH = std::getenv("CONFIG_PATH"))
    {
        configPath = CONFIG_PATH ;
        cout << Log::severity::info << "Using configPath from Environment: " << configPath << endl;
    }
    else
        cout << Log::severity::info << "Using Default configPath: " << configPath << endl;
    try {
	modmqttd::Log::severity  level = (modmqttd::Log::severity)(LogLevel - 1);
        cout << Log::severity::info << "Using converted severity " << static_cast<int>(level) << endl;
        modmqttd::Log::init_logging(level);
        cout << Log::severity::info << "Log initialized " << endl;
        server.init(configPath);
        cout << Log::severity::info << "Server initialized " << endl;
        server.start();

        cout << Log::severity::info << "modmqttd stopped" << endl;
        return EXIT_SUCCESS;
    } catch (const YAML::BadFile& ex) {
        cout << Log::severity::error << "Failed to load configuration from " + configPath << endl;
    } catch (const std::exception& ex) {
        cout << Log::severity::error << ex.what() << endl;
    } catch (...) {
        cout << Log::severity::error << "Unknown initialization error occured" << endl;
    }
    return EXIT_FAILURE;
}
