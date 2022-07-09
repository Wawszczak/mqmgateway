#include "modbus_thread.hpp"

#include "modmqtt.hpp"
#include "modbus_types.hpp"
#include "modbus_context.hpp"
#include <iostream>

using namespace std;


namespace modmqttd {

ModbusThread::ModbusThread(
    moodycamel::BlockingReaderWriterQueue<QueueItem>& toModbusQueue,
    moodycamel::BlockingReaderWriterQueue<QueueItem>& fromModbusQueue)
    : mToModbusQueue(toModbusQueue), mFromModbusQueue(fromModbusQueue)
{
}

void
ModbusThread::configure(const ModbusNetworkConfig& config) {
    mNetworkName = config.mName;
    mModbus = ModMqtt::getModbusFactory().getContext(config.mName);
    mModbus->init(config);
}

void
ModbusThread::pollRegisters(int slaveId, const std::vector<std::shared_ptr<RegisterPoll>>& registers, bool sendIfChanged) {
    for(std::vector<std::shared_ptr<RegisterPoll>>::const_iterator reg_it = registers.begin();
        reg_it != registers.end(); reg_it++)
    {
        try {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            u_int16_t newValue;
            RegisterPoll& reg(**reg_it);
            newValue = mModbus->readModbusRegister(slaveId, reg);
            reg.mLastRead = std::chrono::steady_clock::now();

            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            cout << Log::severity::debug << "Register " << slaveId << "." << reg.mRegister << " (0x" << std::hex << slaveId << ".0x" << std::hex << reg.mRegister << ")"
                            << " polled in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << endl;

            if ((reg.mLastValue != newValue) || !sendIfChanged || (reg.mReadErrors != 0)) {
                MsgRegisterValue val(slaveId, reg.mRegisterType, reg.mRegister, newValue);
                sendMessage(QueueItem::create(val));
                reg.mLastValue = newValue;
                reg.mReadErrors = 0;
                cout << Log::severity::debug << "Register " << slaveId << "." << reg.mRegister
                    << " value sent, data=" << reg.mLastValue << endl;
            };
            //handle incoming write requests
            //in poll loop to avoid delays
            processCommands();
        } catch (const ModbusReadException& ex) {
            handleRegisterReadError(slaveId, **reg_it, ex.what());
        };
    };
};

void
ModbusThread::handleRegisterReadError(int slaveId, RegisterPoll& regPoll, const char* errorMessage) {
    // avoid flooding logs with register read error messages - log last error every 5 minutes
    regPoll.mReadErrors++;
    if (regPoll.mReadErrors == 1 || (std::chrono::steady_clock::now() - regPoll.mFirstErrorTime > RegisterPoll::DurationBetweenLogError)) {
        cout << Log::severity::error << regPoll.mReadErrors << " error(s) when reading register "
            << slaveId << "." << regPoll.mRegister << ", last error: " << errorMessage << endl;
        regPoll.mFirstErrorTime = std::chrono::steady_clock::now();
        if (regPoll.mReadErrors != 1)
            regPoll.mReadErrors = 0;
    }

    // start sending MsgRegisterReadFailed if we cannot read register DefaultReadErrorCount times
    if (regPoll.mReadErrors > RegisterPoll::DefaultReadErrorCount) {
        MsgRegisterReadFailed msg(slaveId, regPoll.mRegisterType, regPoll.mRegister);
        sendMessage(QueueItem::create(msg));
    }
}

void
ModbusThread::setPollSpecification(const MsgRegisterPollSpecification& spec) {
    for(std::vector<MsgRegisterPoll>::const_iterator it = spec.mRegisters.begin();
        it != spec.mRegisters.end(); it++)
    {
        std::shared_ptr<RegisterPoll> reg(new RegisterPoll(it->mRegister, it->mRegisterType, it->mRefreshMsec));
        mRegisters[it->mSlaveId].push_back(reg);
    }
    cout << Log::severity::debug << "Poll specification set, got " << mRegisters.size() << " slaves," << spec.mRegisters.size() << " registers to poll" << endl;

    //now wait for MqttNetworkState(up)
}

void
ModbusThread::doInitialPoll() {
    cout << Log::debug << "starting initial poll" << endl;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for(std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>::const_iterator slave = mRegisters.begin();
        slave != mRegisters.end(); slave++)
    {
        pollRegisters(slave->first, slave->second, false);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    cout << Log::severity::info << "Initial poll done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << endl;
    mNeedInitialPoll = false;
}


bool
ModbusThread::hasRegisters() const {
    for(std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>::const_iterator it = mRegisters.begin();
        it != mRegisters.end(); it++)
    {
        if (it->second.size() != 0)
            return true;
    }
    return false;
}

void
ModbusThread::processWrite(const MsgRegisterValue& msg) {
    try {
        mModbus->writeModbusRegister(msg);
        //send state change immediately if we
        //are polling this register
        std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>::iterator slave = mRegisters.find(msg.mSlaveId);
        if (slave != mRegisters.end()) {
            int regNumber = msg.mRegisterNumber;
            std::vector<std::shared_ptr<RegisterPoll>>::iterator reg_it = std::find_if(
                slave->second.begin(), slave->second.end(),
                [&regNumber](const std::shared_ptr<RegisterPoll>& item) -> bool { return regNumber == item->mRegister; }
            );
            if (reg_it != slave->second.end()) {
                RegisterPoll& reg = **reg_it;
                reg.mLastValue = msg.mValue;
                reg.mLastRead = std::chrono::steady_clock::now();
                MsgRegisterValue val(msg.mSlaveId, msg.mRegisterType, msg.mRegisterNumber, msg.mValue);
                sendMessage(QueueItem::create(val));
            }
        }
    } catch (const ModbusWriteException& ex) {
        cout << Log::severity::error << "error writing register "
            << msg.mSlaveId << "." << msg.mRegisterNumber << ": " << ex.what() << endl;
        MsgRegisterWriteFailed msg(msg.mSlaveId, msg.mRegisterType, msg.mRegisterNumber);
        sendMessage(QueueItem::create(msg));
    }
}

void
ModbusThread::dispatchMessages(const QueueItem& readed) {
    QueueItem item(readed);
    bool gotItem = true;
    // WARNING: dispatch loop can be run from inside pollRegisters loop
    // when processing commands
    // do not call pollRegisters when handing incoming messages!
    do {
        if (item.isSameAs(typeid(ModbusNetworkConfig))) {
            configure(*item.getData<ModbusNetworkConfig>());
        } else if (item.isSameAs(typeid(MsgRegisterPollSpecification))) {
            setPollSpecification(*item.getData<MsgRegisterPollSpecification>());
        } else if (item.isSameAs(typeid(EndWorkMessage))) {
            cout << Log::severity::debug << "Got exit command" << endl;
            mShouldRun = false;
        } else if (item.isSameAs(typeid(MsgRegisterValue))) {
            processWrite(*item.getData<MsgRegisterValue>());
        } else if (item.isSameAs(typeid(MsgMqttNetworkState))) {
            std::unique_ptr<MsgMqttNetworkState> netstate(item.getData<MsgMqttNetworkState>());
            mShouldPoll = netstate->mIsUp;
        } else {
            cout << Log::severity::error << "Unknown messsage received, ignoring" << endl;
        }
        gotItem = mToModbusQueue.try_dequeue(item);
    } while(gotItem);
}

void
ModbusThread::processCommands() {
    QueueItem item;
    while(mToModbusQueue.try_dequeue(item))
        dispatchMessages(item);
}

void
ModbusThread::sendMessage(const QueueItem& item) {
    mFromModbusQueue.enqueue(item);
    modmqttd::notifyQueues();
}

void
ModbusThread::run() {
    try {
        cout << Log::severity::debug << "Modbus thread started" << endl;
        const int maxReconnectTime = 60;
        std::chrono::steady_clock::duration waitDuration = std::chrono::steady_clock::duration::max();
        while(mShouldRun) {
            if (mModbus) {
                if (!mModbus->isConnected()) {
                    if (waitDuration > std::chrono::seconds(maxReconnectTime))
                        waitDuration = std::chrono::seconds(0);
                    cout << Log::severity::info << "modbus: connecting" << endl;
                    mModbus->connect();
                    if (mModbus->isConnected()) {
                        cout << Log::severity::info << "modbus: connected" << endl;
                        sendMessage(QueueItem::create(MsgModbusNetworkState(mNetworkName, true)));
                        mNeedInitialPoll = true;
                    }
                }

                // start polling only if Mosquitto
                // have succesfully connected to Mqtt broker
                // to avoid growing mFromModbusQueue with queued register updates
                if (mModbus->isConnected()) {
                    if (mShouldPoll) {
                        // if modbus network was disconnected
                        // we need to refresh everything
                        if (mNeedInitialPoll)
                            doInitialPoll();

                        //initial wait set to infinity, scheduler will adjust this value
                        //to time period for next poll
                        waitDuration = std::chrono::steady_clock::duration::max();

                        // note start time and find registers that need a refresh now
                        auto start = std::chrono::steady_clock::now();
                        std::map<int, std::vector<std::shared_ptr<RegisterPoll>>> toRefresh = mScheduler.getRegistersToPoll(mRegisters, waitDuration, start);
                        for(std::map<int, std::vector<std::shared_ptr<RegisterPoll>>>::const_iterator slave = toRefresh.begin();
                            slave != toRefresh.end(); slave++)
                        {
                            //this may call processCommands
                            pollRegisters(slave->first, slave->second);
                        }
                        //decrease time to next poll by last poll time
                        if (toRefresh.size() != 0) {
                            auto pollDuration = std::chrono::steady_clock::now() - start;
                            waitDuration -= pollDuration;
                            if (waitDuration < std::chrono::steady_clock::duration::zero()) {
                               cout << Log::severity::debug << "Next poll is eariler than current poll time (" <<  std::chrono::duration_cast<std::chrono::milliseconds>(waitDuration).count() << "ms)" << endl;
                                waitDuration = std::chrono::steady_clock::duration::zero();
                            }
                        }
                    } else {
                        cout << Log::severity::info << "Waiting for mqtt network to become online" << endl;
                        waitDuration = std::chrono::steady_clock::duration::max();
                    }
                } else {
                    sendMessage(QueueItem::create(MsgModbusNetworkState(mNetworkName, false)));
                    if (waitDuration < std::chrono::seconds(maxReconnectTime))
                        waitDuration += std::chrono::seconds(5);
                };
            } else {
                //wait for poll specification
                waitDuration = std::chrono::steady_clock::duration::max();
            }

            //register polling can change mShouldRun flag, do not wait
            //for next poll if we are exiting
            if (mShouldRun) {
                QueueItem item;
                cout << Log::severity::debug << "Waiting " <<  std::chrono::duration_cast<std::chrono::milliseconds>(waitDuration).count() << "ms for messages" << endl;
                if (!mToModbusQueue.wait_dequeue_timed(item, waitDuration))
                    continue;
                dispatchMessages(item);
            }
        };
        if (mModbus && mModbus->isConnected())
            mModbus->disconnect();
       cout << Log::severity::debug << "Modbus thread " << mNetworkName << " ended" << endl;
    } catch (const std::exception& ex) {
        cout << Log::severity::critical << "Error in modbus thread " << mNetworkName << ": " << ex.what() << endl;
    } catch (...) {
        cout << Log::severity::critical << "Unknown error in modbus thread " << mNetworkName << endl;
    }
}

}
