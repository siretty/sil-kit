/* Copyright (c) 2022 Vector Informatik GmbH

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include <algorithm>
#include <cstring>
#include <future>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "silkit/SilKit.hpp"
#include "silkit/services/logging/ILogger.hpp"
#include "silkit/services/orchestration/all.hpp"
#include "silkit/services/orchestration/string_utils.hpp"
#include "silkit/services/can/all.hpp"
#include "silkit/services/can/string_utils.hpp"

using namespace SilKit::Services;
using namespace SilKit::Services::Can;

using namespace std::chrono_literals;

namespace std {
namespace chrono {
std::ostream& operator<<(std::ostream& out, nanoseconds timestamp)
{
    auto seconds = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(timestamp);
    out << seconds.count() << "s";
    return out;
}
} // namespace chrono
} // namespace std

void FrameTransmitHandler(const CanFrameTransmitEvent& ack, Logging::ILogger* logger)
{
    std::stringstream buffer;
    buffer << ">> " << ack.status
           << " for CAN Message with timestamp=" << ack.timestamp;
    logger->Info(buffer.str());
}

void FrameHandler(const CanFrameEvent& frameEvent, Logging::ILogger* logger)
{
    std::string payload(frameEvent.frame.dataField.begin(), frameEvent.frame.dataField.end());
    std::stringstream buffer;
    buffer << ">> CAN Message: canId=" << frameEvent.frame.canId
           << " timestamp=" << frameEvent.timestamp
           << " \"" << payload << "\"";
    logger->Info(buffer.str());
}

void SendFrame(ICanController* controller, Logging::ILogger* logger)
{
    CanFrame canFrame {};
    canFrame.canId = 3;
    canFrame.flags |= static_cast<CanFrameFlagMask>(CanFrameFlag::Fdf) // FD Format Indicator
                      | static_cast<CanFrameFlagMask>(CanFrameFlag::Brs); // Bit Rate Switch (for FD Format only)

    static int msgId = 0;
    std::stringstream payloadBuilder;
    payloadBuilder << "CAN " << (msgId++)%100;
    auto payloadStr = payloadBuilder.str();

    std::vector<uint8_t> payload;
    std::copy(payloadStr.begin(), payloadStr.end(), std::back_inserter(payload));

    canFrame.dataField = payload;
    canFrame.dlc = static_cast<uint16_t>(canFrame.dataField.size());

    controller->SendFrame(std::move(canFrame));
    std::stringstream buffer;
    buffer << "<< CAN Message sent";
    logger->Info(buffer.str());
}

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Missing arguments! Start demo with: " << argv[0]
                  << " <ParticipantConfiguration.yaml|json> <ParticipantName> [RegistryUri] [--async]" << std::endl
                  << "Use \"CanWriter\" or \"CanReader\" as <ParticipantName>." << std::endl;
        return -1;
    }

    if (argc > 5)
    {
        std::cerr << "Too many arguments! Start demo with: " << argv[0]
                  << " <ParticipantConfiguration.yaml|json> <ParticipantName> [RegistryUri] [--async]" << std::endl
                  << "Use \"CanWriter\" or \"CanReader\" as <ParticipantName>." << std::endl;
        return -1;
    }
    
    try
    {
        std::string participantConfigurationFilename(argv[1]);
        std::string participantName(argv[2]);

        std::string registryUri = "silkit://localhost:8500";

        bool runSync = true;
        SilKit::Services::Orchestration::OperationMode runCoordinated =
            SilKit::Services::Orchestration::OperationMode::Coordinated;

        std::vector<std::string> args;
        std::copy((argv + 3), (argv + argc), std::back_inserter(args));

        for (auto arg : args)
        {
            if (arg == "--async")
            {
                runSync = false;
            }
            else if (arg == "--autonomous")
            {
                runCoordinated = SilKit::Services::Orchestration::OperationMode::Autonomous;
            }
            else
            {
                registryUri = arg;
            }
        }

        auto participantConfiguration = SilKit::Config::ParticipantConfigurationFromFile(participantConfigurationFilename);
        auto sleepTimePerTick = 1000ms;

        std::cout << "Creating participant '" << participantName << "' with registry " << registryUri << std::endl;

        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryUri);

        auto* logger = participant->GetLogger();
        auto* canController = participant->CreateCanController("CAN1", "CAN1");

        canController->AddFrameTransmitHandler(
            [logger](ICanController* /*ctrl*/, const CanFrameTransmitEvent& ack) {
                FrameTransmitHandler(ack, logger);
            });
        canController->AddFrameHandler(
            [logger](ICanController* /*ctrl*/, const CanFrameEvent& frameEvent) {
                FrameHandler(frameEvent, logger);
            });

        if (runSync)
        {
            // always use life cycle
            auto* lifecycleService = participant->CreateLifecycleService({runCoordinated});

            // Set a CommunicationReady Handler
            lifecycleService->SetCommunicationReadyHandler([canController, &participantName]() {
                std::cout << "Communication ready for " << participantName << std::endl;
                canController->SetBaudRate(10'000, 1'000'000, 2'000'000);
                canController->Start();
            });

            // Set a Stop Handler
            lifecycleService->SetStopHandler([]() {
                std::cout << "Stopping..." << std::endl;
            });

            // Set a Shutdown Handler
            lifecycleService->SetShutdownHandler([]() {
                std::cout << "Shutting down..." << std::endl;
            });

            auto* timeSyncService = lifecycleService->CreateTimeSyncService();

            if (participantName == "Sender")
            {
                timeSyncService->SetSimulationStepHandler(
                    [canController, logger, sleepTimePerTick](std::chrono::nanoseconds now,
                                                              std::chrono::nanoseconds duration) {
                        std::cout << "now=" << now << ", duration=" << duration << std::endl;
                        SendFrame(canController, logger);
                        std::this_thread::sleep_for(sleepTimePerTick);
                    }, 5ms);
            }
            else
            {
                timeSyncService->SetSimulationStepHandler(
                    [sleepTimePerTick](std::chrono::nanoseconds now, std::chrono::nanoseconds duration) {
                        std::cout << "now=" << now << ", duration=" << duration << std::endl;
                        std::this_thread::sleep_for(sleepTimePerTick);
                    }, 5ms);
            }

            auto finalStateFuture = lifecycleService->StartLifecycle();
            auto finalState = finalStateFuture.get();

            std::cout << "Simulation stopped. Final State: " << finalState << std::endl;
            std::cout << "Press enter to stop the process..." << std::endl;
            std::cin.ignore();
        }
        else
        {
            // always use life cycle
            auto* lifecycleService = participant->CreateLifecycleService({runCoordinated});

            // Set a CommunicationReady Handler
            lifecycleService->SetCommunicationReadyHandler([canController, &participantName]() {
                std::cout << "Communication ready for " << participantName << std::endl;
                canController->SetBaudRate(10'000, 1'000'000, 2'000'000);
                canController->Start();
            });

            // Set a Stop Handler
            lifecycleService->SetStopHandler([]() {
                std::cout << "Stopping..." << std::endl;
            });

            // Set a Shutdown Handler
            lifecycleService->SetShutdownHandler([]() {
                std::cout << "Shutting down..." << std::endl;
            });

            bool isStopped = false;
            std::promise<void> startSending;
            std::promise<void> stopped;
            std::thread workerThread;

            // Set a Starting Handler (only triggers for asynchronous participants)
            lifecycleService->SetStartingHandler([&]() {
                std::cout << "Starting..." << std::endl;
                startSending.set_value();
            });


            if (participantName == "Sender")
            {
                workerThread = std::thread{[&]() {
                    auto startSendingFuture = startSending.get_future();
                    startSendingFuture.get();

                    while (!isStopped)
                    {
                        SendFrame(canController, logger);
                        std::this_thread::sleep_for(sleepTimePerTick);
                    }
                    std::cout << "Sending stop signal..." << std::endl;
                    lifecycleService->Stop("Manual stop.");
                }};
            }
            else
            {
                workerThread = std::thread{[&]() {
                    startSending.get_future().wait();
                    stopped.get_future().wait();

                    std::cout << "Sending stop signal..." << std::endl;
                    lifecycleService->Stop("Manual stop.");
                }};
            }

            auto finalStateFuture =
                lifecycleService->StartLifecycle();
            std::cout << "Press enter to stop the process..." << std::endl;
            std::cin.ignore();
            isStopped = true;
            stopped.set_value();
            auto finalState = finalStateFuture.get();
            std::cout << "Simulation stopped. Final State: " << finalState << std::endl;
            workerThread.join();
        }
    }
    catch (const SilKit::ConfigurationError& error)
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
