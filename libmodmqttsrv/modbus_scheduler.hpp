#pragma once

#include <map>
#include <vector>
#include <memory>
#include <chrono>

#include "logging.hpp"
#include "modbus_types.hpp"
#include "register_poll.hpp"

namespace modmqttd {

    class ModbusScheduler {
        public:
            /**
             * Returns map of devices with list of registers, that
             * should be polled now.
             *
             * sets outDuration to time period that should be waited
             * for next poll to be done.
             *
             * */
            std::map<int, std::vector<std::shared_ptr<RegisterPoll>>> getRegistersToPoll(
                const std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>& registers,
                std::chrono::steady_clock::duration& outDuration,
                const std::chrono::time_point<std::chrono::steady_clock>& timePoint
            );
//        private:
//            boost::log::sources::severity_logger<Log::severity> log;
    };
}

