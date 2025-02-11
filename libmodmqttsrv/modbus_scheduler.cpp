#include "modbus_scheduler.hpp"
#include "modbus_types.hpp"
#include <iostream>
#include <sstream>

namespace modmqttd {

	using namespace std;

std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>
ModbusScheduler::getRegistersToPoll(
    const std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>& registers,
    std::chrono::steady_clock::duration& outDuration,
    const std::chrono::time_point<std::chrono::steady_clock>& timePoint
) {
    std::map<int, std::vector<std::shared_ptr<RegisterPoll>>> ret;

//    cout << Log::severity::debug << "ModbusScheduler::getRegistersToPoll(): initial outduration " << std::chrono::duration_cast<std::chrono::milliseconds>(outDuration).count() << endl;

    for(std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>::const_iterator slave = registers.begin();
        slave != registers.end(); slave++)
    {
        for(std::vector<std::shared_ptr<RegisterPoll>>::const_iterator reg_it = slave->second.begin();
            reg_it != slave->second.end(); reg_it++)
        {
            const RegisterPoll& reg = **reg_it;

            auto time_passed = timePoint - reg.mLastRead;
            auto time_to_poll = reg.mRefresh;

            cout << Log::severity::debug << "ModbusScheduler::getRegistersTooPoll(): time passed: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_to_poll).count() << endl;

            if (time_passed >= reg.mRefresh) {
                cout << Log::severity::debug << "ModbusScheduler::getRegistersToPoll(): Register " << slave->first << "." << reg.mRegister << " ( " << slave->first << "." << reg.mRegister << ")"
                                << " added, last read " << std::chrono::duration_cast<std::chrono::milliseconds>(time_passed).count() << "ms ago" << endl;
                ret[slave->first].push_back(*reg_it);
            } else {
                time_to_poll = reg.mRefresh - time_passed;
            }
            if (outDuration > time_to_poll) {
                outDuration = time_to_poll;
                cout << Log::severity::debug << "ModbusScheduler::getRegistersToPoll(): Wait duration set to " << std::chrono::duration_cast<std::chrono::milliseconds>(time_to_poll).count() 
                                << "ms as next poll for register " << slave->first << "." << reg.mRegister << " (" << slave->first << "." << reg.mRegister << ")" << endl;
            }
        }
    }


    return ret;
}

}
