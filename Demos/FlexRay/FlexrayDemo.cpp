// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <sstream>
#include <thread>

#include "ib/IntegrationBus.hpp"
#include "ib/mw/sync/all.hpp"
#include "ib/mw/sync/string_utils.hpp"
#include "ib/sim/fr/all.hpp"
#include "ib/util/functional.hpp"

using namespace ib::mw;
using namespace ib::sim;
using namespace ib::util;

using namespace std::chrono_literals;
using namespace std::placeholders;

std::ostream& operator<<(std::ostream& out, std::chrono::nanoseconds timestamp)
{
    auto seconds = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(timestamp);
    out << seconds.count() << "s";
    return out;
}

template<typename T>
void ReceiveMessage(fr::IFrController* /*controller*/, const T& t)
{
    std::cout << ">> " << t << "\n";
}

struct FlexRayNode
{
    FlexRayNode(fr::IFrController* controller, fr::ControllerConfig config)
        : controller{controller}
        , controllerConfig{std::move(config)}
    {
      oldPocStatus.state = fr::PocState::DefaultConfig;
    }

    void SetStartupDelay(std::chrono::nanoseconds delay)
    {
        _startupDelay = delay;
    }

    void Init()
    {
        if (_configureCalled)
            return;

        controller->Configure(controllerConfig);
        _configureCalled = true;
    }

    void doAction(std::chrono::nanoseconds now)
    {
        if (now < _startupDelay)
            return;
        switch (oldPocStatus.state)
        {
        case fr::PocState::DefaultConfig:
            Init();
        case fr::PocState::Ready:
            return pocReady(now);
        case fr::PocState::NormalActive:
            if (now == 100ms + std::chrono::duration_cast<std::chrono::milliseconds>(_startupDelay))
            {
                return ReconfigureTxBuffers();
            }
            else
            {
                return txBufferUpdate(now);
            }
        case fr::PocState::Config:
        case fr::PocState::Startup:
        case fr::PocState::Wakeup:
        case fr::PocState::NormalPassive:
        case fr::PocState::Halt:
            return;
        }
    }

    void pocReady(std::chrono::nanoseconds /*now*/)
    {
        switch (busState)
        {
        case MasterState::PerformWakeup:
            controller->Wakeup();
            return;
        case MasterState::WaitForWakeup:
            return;
        case MasterState::WakeupDone:
            controller->AllowColdstart();
            controller->Run();
            return;
        default:
            return;
        }
    }

    void txBufferUpdate(std::chrono::nanoseconds /*now*/)
    {
        if (controllerConfig.bufferConfigs.empty())
            return;

        static auto msgNumber = -1;
        msgNumber++;

        auto bufferIdx = msgNumber % controllerConfig.bufferConfigs.size();

        // prepare a friendly message as payload
        std::stringstream payloadStream;
        payloadStream << "FrMessage#" << std::setw(4) << msgNumber
                      << "; bufferId=" << bufferIdx;
        auto payloadString = payloadStream.str();


        fr::TxBufferUpdate update;
        update.payload.resize(payloadString.size());
        update.payloadDataValid = true;
        update.txBufferIndex = static_cast<decltype(update.txBufferIndex)>(bufferIdx);

        std::copy(payloadString.begin(), payloadString.end(), update.payload.begin());
        //update.payload[payloadString.size()] = 0;

        controller->UpdateTxBuffer(update);
    }

    // Reconfigure buffers: Swap Channels A and B
    void ReconfigureTxBuffers()
    {
        std::cout << "Reconfiguring TxBuffers. Swapping Channel::A and Channel::B\n";
        for (uint16_t idx = 0; idx < controllerConfig.bufferConfigs.size(); idx++)
        {
            auto&& bufferConfig = controllerConfig.bufferConfigs[idx];
            switch (bufferConfig.channels)
            {
            case fr::Channel::A:
                bufferConfig.channels = fr::Channel::B;
                controller->ReconfigureTxBuffer(idx, bufferConfig);
                break;
            case fr::Channel::B:
                bufferConfig.channels = fr::Channel::A;
                controller->ReconfigureTxBuffer(idx, bufferConfig);
                break;
            default:
                break;
            }
        }
    }

    void PocStatusHandler(fr::IFrController* /*controller*/, const fr::PocStatus& pocStatus)
    {
        std::cout << ">> POC=" << pocStatus.state
                  << ", Freeze=" <<  pocStatus.freeze
                  << ", Wakeup=" <<  pocStatus.wakeupStatus
                  << ", Slot=" <<  pocStatus.slotMode
                  << " @t=" << pocStatus.timestamp
                  << std::endl;

        if (oldPocStatus.state == fr::PocState::Wakeup
            && pocStatus.state == fr::PocState::Ready)
        {
            std::cout << "   Wakeup finished..." << std::endl;
            busState = MasterState::WakeupDone;
        }

        oldPocStatus = pocStatus;
    }

    void WakeupHandler(fr::IFrController* frController, const fr::FrSymbol& symbol)
    {
        std::cout << ">> WAKEUP! (" << symbol.pattern << ")" << std::endl;
        frController->AllowColdstart();
        frController->Run();
    }


    fr::IFrController* controller = nullptr;

    fr::ControllerConfig controllerConfig;
    fr::PocStatus oldPocStatus{};
    bool _configureCalled = false;
    std::chrono::nanoseconds _startupDelay = 0ns;

    enum class MasterState
    {
        Ignore,
        PerformWakeup,
        WaitForWakeup,
        WakeupDone
    };
    MasterState busState = MasterState::Ignore;
};


auto MakeNodeParams(const std::string& participantName) -> ib::sim::fr::NodeParameters
{
    ib::sim::fr::NodeParameters nodeParams;
    nodeParams.pAllowHaltDueToClock = 1;
    nodeParams.pAllowPassiveToActive = 0;
    nodeParams.pChannels = fr::Channel::AB;
    nodeParams.pClusterDriftDamping = 2;
    nodeParams.pdAcceptedStartupRange = 212;
    nodeParams.pdListenTimeout = 400162;
    nodeParams.pKeySlotOnlyEnabled = 0;
    nodeParams.pKeySlotUsedForStartup = 1;
    nodeParams.pKeySlotUsedForSync = 0;
    nodeParams.pLatestTx = 249;
    nodeParams.pMacroInitialOffsetA = 3;
    nodeParams.pMacroInitialOffsetB = 3;
    nodeParams.pMicroInitialOffsetA = 6;
    nodeParams.pMicroInitialOffsetB = 6;
    nodeParams.pMicroPerCycle = 200000;
    nodeParams.pOffsetCorrectionOut = 127;
    nodeParams.pOffsetCorrectionStart = 3632;
    nodeParams.pRateCorrectionOut = 81;
    nodeParams.pWakeupChannel = fr::Channel::A;
    nodeParams.pWakeupPattern = 33;
    nodeParams.pdMicrotick = fr::ClockPeriod::T25NS;
    nodeParams.pSamplesPerMicrotick = 2;

    if (participantName == "Node0")
    {
        nodeParams.pKeySlotId = 40;
    }
    else if (participantName == "Node1")
    {
        nodeParams.pKeySlotId = 60;
    }
    else
    {
        throw std::runtime_error("Invalid participant name.");
    }

    return nodeParams;
}

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char** argv)
{
    ib::sim::fr::ClusterParameters clusterParams;
    clusterParams.gColdstartAttempts = 8;
    clusterParams.gCycleCountMax = 63;
    clusterParams.gdActionPointOffset = 2;
    clusterParams.gdDynamicSlotIdlePhase = 1;
    clusterParams.gdMiniSlot = 5;
    clusterParams.gdMiniSlotActionPointOffset = 2;
    clusterParams.gdStaticSlot = 31;
    clusterParams.gdSymbolWindow = 0;
    clusterParams.gdSymbolWindowActionPointOffset = 1;
    clusterParams.gdTSSTransmitter = 9;
    clusterParams.gdWakeupTxActive = 60;
    clusterParams.gdWakeupTxIdle = 180;
    clusterParams.gListenNoise = 2;
    clusterParams.gMacroPerCycle = 3636;
    clusterParams.gMaxWithoutClockCorrectionFatal = 2;
    clusterParams.gMaxWithoutClockCorrectionPassive = 2;
    clusterParams.gNumberOfMiniSlots = 291;
    clusterParams.gNumberOfStaticSlots = 70;
    clusterParams.gPayloadLengthStatic = 13;
    clusterParams.gSyncFrameIDCountMax = 15;

    if (argc < 3)
    {
        std::cerr << "Missing arguments! Start demo with: " << argv[0]
                  << " <ParticipantConfiguration.yaml|json> <ParticipantName> [domainId]" << std::endl
                  << "Use \"Node0\" or \"Node1\" as <ParticipantName>." << std::endl;
        return -1;
    }

    try
    {
        std::string participantConfigurationFilename(argv[1]);
        std::string participantName(argv[2]);

        uint32_t domainId = 42;
        if (argc >= 4)
        {
            domainId = static_cast<uint32_t>(std::stoul(argv[3]));
        }   

        auto participantConfiguration = ib::cfg::ParticipantConfigurationFromFile(participantConfigurationFilename);

        std::cout << "Creating ComAdapter for Participant=" << participantName << " in Domain " << domainId << std::endl;
        auto participant = ib::CreateSimulationParticipant(participantConfiguration, participantName, domainId, true);
        auto* controller = participant->CreateFlexrayController("FlexRay1", "PowerTrain1");
        auto* participantController = participant->GetParticipantController();

        std::vector<fr::TxBufferConfig> bufferConfigs;

        if (participantName == "Node0")
        {
            // initialize bufferConfig to send some FrMessages
            fr::TxBufferConfig cfg;
            cfg.channels = fr::Channel::AB;
            cfg.slotId = 40;
            cfg.offset = 0;
            cfg.repetition = 1;
            cfg.hasPayloadPreambleIndicator = false;
            cfg.headerCrc = 5;
            cfg.transmissionMode = fr::TransmissionMode::SingleShot;
            bufferConfigs.push_back(cfg);

            cfg.channels = fr::Channel::A;
            cfg.slotId = 41;
            bufferConfigs.push_back(cfg);

            cfg.channels = fr::Channel::B;
            cfg.slotId = 42;
            bufferConfigs.push_back(cfg);
        }
        else if (participantName == "Node1")
        {
            // initialize bufferConfig to send some FrMessages
            fr::TxBufferConfig cfg;
            cfg.channels = fr::Channel::AB;
            cfg.slotId = 60;
            cfg.offset = 0;
            cfg.repetition = 1;
            cfg.hasPayloadPreambleIndicator = false;
            cfg.headerCrc = 5;
            cfg.transmissionMode = fr::TransmissionMode::SingleShot;
            bufferConfigs.push_back(cfg);

            cfg.channels = fr::Channel::A;
            cfg.slotId = 61;
            bufferConfigs.push_back(cfg);

            cfg.channels = fr::Channel::B;
            cfg.slotId = 62;
            bufferConfigs.push_back(cfg);
        }

        fr::ControllerConfig config;
        config.bufferConfigs = bufferConfigs;

        config.clusterParams = clusterParams;
        
        config.nodeParams = MakeNodeParams(participantName);

        FlexRayNode frNode(controller, std::move(config));
        if (participantName == "Node0")
        {
            frNode.busState = FlexRayNode::MasterState::PerformWakeup;
        }
        if (participantName == "Node1")
        {
            frNode.busState = FlexRayNode::MasterState::PerformWakeup;
            frNode.SetStartupDelay(0ms);
        }

        controller->RegisterPocStatusHandler(bind_method(&frNode, &FlexRayNode::PocStatusHandler));
        controller->RegisterMessageHandler(&ReceiveMessage<fr::FrMessage>);
        controller->RegisterMessageAckHandler(&ReceiveMessage<fr::FrMessageAck>);
        controller->RegisterWakeupHandler(bind_method(&frNode, &FlexRayNode::WakeupHandler));
        controller->RegisterSymbolHandler(&ReceiveMessage<fr::FrSymbol>);
        controller->RegisterSymbolAckHandler(&ReceiveMessage<fr::FrSymbolAck>);
        controller->RegisterCycleStartHandler(&ReceiveMessage<fr::CycleStart>);

        participantController->SetSimulationTask(
            [&frNode](std::chrono::nanoseconds now, std::chrono::nanoseconds /*duration*/) {
                
                auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now);
                std::cout << "now=" << nowMs.count() << "ms" << std::endl;
                frNode.doAction(now);
                std::this_thread::sleep_for(500ms);
                
        });

        //auto finalStateFuture = participantController->RunAsync();
        //auto finalState = finalStateFuture.get();

        auto finalState = participantController->Run();

        std::cout << "Simulation stopped. Final State: " << finalState << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
    }
    catch (const ib::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -3;
    }

    return 0;
}
