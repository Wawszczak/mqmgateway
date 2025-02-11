#include <cstddef>
#include <string>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstdio>

#include "logging.hpp"

using namespace std ;

namespace modmqttd {

const char* LogLevelToString(Log::severity level)
{
    static const char* strings[] =
    {
        "[CRITICAL] ",
        "[ERROR]    ",
        "[WARNING]  ",
        "[INFO]     ",
        "[DEBUG]    ",
	"[UNKNOWN]  "
    };

    if (static_cast< std::size_t >(level) < (sizeof(strings) / sizeof(*strings) - 1))
        return strings[level];
    else
        return strings[sizeof(strings) / sizeof(*strings)];


}

std::ostream& operator<< (std::ostream& strm, Log::severity level)
{
    stringstream* sstrm = new stringstream();
//    *sstrm << static_cast<int>(level) << " " << static_cast<int>(Log::LogLevel) << " " ;
	
    if(Log::LogLevel >= level)
    {
	std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char mbstr[100];
	struct tm tmi;
	*sstrm << std::put_time(localtime_r(&t, &tmi), "[%c] " ); 
        //if (std::strftime(mbstr, sizeof(mbstr), "[%c] ", std::localtime(&t))) {
        //   *sstrm << mbstr ;
        //}
    	*sstrm << LogLevelToString(level);
    }
    else
    {
	*sstrm << "$SUPPRESSED$";
    }
    return *sstrm;
}


std::ostream& endl( ostream& os )
{
	try
	{
	stringstream& s = dynamic_cast<stringstream&>(os);
	string msg = s.str();
	if(msg.rfind("$SUPPRESSED$",0) == string::npos)
	{
        	printf("%s\n", msg.c_str()) ;
	}
	stringstream* s2strm = &s;
	delete s2strm;
	return cout;
	}
	catch(std::bad_cast)
	{
		return os << std::endl;
	}
}


Log::severity Log::LogLevel = Log::severity::debug;

void Log::init_logging(severity level) {
	cout << Log::severity::debug << "Initializing Logging with Level " << LogLevelToString(level) << endl ;
	LogLevel = level;
}

}
