// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <iomanip>
#include <sstream>

#include "LinDatatypes.hpp"

#include "ib/exception.hpp"

namespace ib {
namespace sim {
namespace lin { 

inline std::string to_string(ChecksumModel model);
inline std::string to_string(FrameResponseType responseType);
inline std::string to_string(FrameResponseMode responseMode);
inline std::string to_string(FrameStatus frameStatus);
inline std::string to_string(ControllerMode mode);
inline std::string to_string(ControllerStatus status);

inline std::string to_string(const Frame& frame);
inline std::string to_string(const SendFrameRequest& request);
inline std::string to_string(const SendFrameHeaderRequest& request);
inline std::string to_string(const Transmission& transmission);
inline std::string to_string(const WakeupPulse& pulse);
inline std::string to_string(const ControllerConfig& controllerConfig);
inline std::string to_string(const ControllerStatusUpdate& controllerStatusUpdate);
inline std::string to_string(const FrameResponseUpdate& frameResponseUpdate);

inline std::ostream& operator<<(std::ostream& out, ChecksumModel model);
inline std::ostream& operator<<(std::ostream& out, FrameResponseType responseType);
inline std::ostream& operator<<(std::ostream& out, FrameResponseMode responseMode);
inline std::ostream& operator<<(std::ostream& out, FrameStatus frameStatus);       
inline std::ostream& operator<<(std::ostream& out, ControllerMode mode);           
inline std::ostream& operator<<(std::ostream& out, ControllerStatus status);

inline std::ostream& operator<<(std::ostream& out, const Frame& frame);
inline std::ostream& operator<<(std::ostream& out, const SendFrameRequest& request);
inline std::ostream& operator<<(std::ostream& out, const SendFrameHeaderRequest& request);
inline std::ostream& operator<<(std::ostream& out, const Transmission& transmission);
inline std::ostream& operator<<(std::ostream& out, const WakeupPulse& pulse);
inline std::ostream& operator<<(std::ostream& out, const ControllerConfig& controllerConfig);
inline std::ostream& operator<<(std::ostream& out, const ControllerStatusUpdate& controllerStatusUpdate);
inline std::ostream& operator<<(std::ostream& out, const FrameResponseUpdate& frameResponseUpdate);


// ================================================================================
//  Inline Implementations
// ================================================================================
std::string to_string(ChecksumModel model)
{
    switch (model)
    {
    case ChecksumModel::Undefined:
        return "Undefined";
    case ChecksumModel::Enhanced:
        return "Enhanced";
    case ChecksumModel::Classic:
        return "Classic";
    }
    throw ib::type_conversion_error{};
}

std::string to_string(FrameResponseType responseType)
{
    switch (responseType)
    {
    case FrameResponseType::MasterResponse:
        return "MasterResponse";
    case FrameResponseType::SlaveResponse:
        return "SlaveResponse";
    case FrameResponseType::SlaveToSlave:
        return "SlaveToSlave";
    }
    throw ib::type_conversion_error{};
}

std::string to_string(FrameResponseMode responseMode)
{
    switch (responseMode)
    {
    case FrameResponseMode::Unused:
        return "Unused";
    case FrameResponseMode::Rx:
        return "Rx";
    case FrameResponseMode::TxUnconditional:
        return "TxUnconditional";
    }
    throw ib::type_conversion_error{};
}

std::string to_string(FrameStatus frameStatus)
{
    switch (frameStatus)
    {
    case FrameStatus::NOT_OK:
        return "NOT_OK";
    case FrameStatus::LIN_TX_OK:
        return "LIN_TX_OK";
    case FrameStatus::LIN_TX_BUSY:
        return "LIN_TX_BUSY";
    case FrameStatus::LIN_TX_HEADER_ERROR:
        return "LIN_TX_HEADER_ERROR";
    case FrameStatus::LIN_TX_ERROR:
        return "LIN_TX_ERROR";
    case FrameStatus::LIN_RX_OK:
        return "LIN_RX_OK";
    case FrameStatus::LIN_RX_BUSY:
        return "LIN_RX_BUSY";
    case FrameStatus::LIN_RX_ERROR:
        return "LIN_RX_ERROR";
    case FrameStatus::LIN_RX_NO_RESPONSE:
        return "LIN_RX_NO_RESPONSE";
    }
    throw ib::type_conversion_error{};
}

std::string to_string(ControllerMode mode)
{
    switch (mode)
    {
    case ControllerMode::Inactive:
        return "Inactive";
    case ControllerMode::Master:
        return "Master";
    case ControllerMode::Slave:
        return "Slave";
    }
    throw ib::type_conversion_error{};
}


std::string to_string(ControllerStatus status)
{
    switch (status)
    {
    case ControllerStatus::Unknown:
        return "Unknown";
    case ControllerStatus::Operational:
        return "Operational";
    case ControllerStatus::Sleep:
        return "Sleep";
    }
    throw ib::type_conversion_error{};
};


std::string to_string(const Frame& frame)
{
    std::stringstream out;
    out << frame;
    return out.str();
}

std::string to_string(const SendFrameRequest& request)
{
    std::stringstream out;
    out << request;
    return out.str();
}

std::string to_string(const SendFrameHeaderRequest& request)
{
    std::stringstream out;
    out << request;
    return out.str();
}

std::string to_string(const Transmission& transmission)
{
    std::stringstream out;
    out << transmission;
    return out.str();
}

std::string to_string(const WakeupPulse& pulse)
{
    std::stringstream out;
    out << pulse;
    return out.str();
}

std::string to_string(const ControllerConfig& controllerConfig)
{
    std::stringstream out;
    out << controllerConfig;
    return out.str();
}

std::string to_string(const ControllerStatusUpdate& controllerStatusUpdate)
{
    std::stringstream out;
    out << controllerStatusUpdate;
    return out.str();
}

std::string to_string(const FrameResponseUpdate& frameResponseUpdate)
{
    std::stringstream out;
    out << frameResponseUpdate;
    return out.str();
}



std::ostream& operator<<(std::ostream& out, ChecksumModel model)
{
    try
    {
        return out << to_string(model);
    }
    catch (const ib::type_conversion_error&)
    {
        return out << "ChecksumModel{" << static_cast<uint32_t>(model) << "}";
    }
}
std::ostream& operator<<(std::ostream& out, FrameResponseType responseType)
    {
    try
    {
        return out << to_string(responseType);
    }
    catch (const ib::type_conversion_error&)
    {
        return out << "FrameResponseType{" << static_cast<uint32_t>(responseType) << "}";
    }
}

std::ostream& operator<<(std::ostream& out, FrameResponseMode responseMode)
    {
    try
    {
        return out << to_string(responseMode);
    }
    catch (const ib::type_conversion_error&)
    {
        return out << "FrameResponseMode{" << static_cast<uint32_t>(responseMode) << "}";
    }
}

std::ostream& operator<<(std::ostream& out, FrameStatus frameStatus)
{
    try
    {
        return out << to_string(frameStatus);
    }
    catch (const ib::type_conversion_error&)
    {
        return out << "FrameStatus{" << static_cast<uint32_t>(frameStatus) << "}";
    }
}

std::ostream& operator<<(std::ostream& out, ControllerMode mode)
{
    try
    {
        return out << to_string(mode);
    }
    catch (const ib::type_conversion_error&)
    {
        return out << "ControllerMode{" << static_cast<uint32_t>(mode) << "}";
    }
}

std::ostream& operator<<(std::ostream& out, ControllerStatus status)
{
    try
    {
        return out << to_string(status);
    }
    catch (const ib::type_conversion_error&)
    {
        return out << "ControllerStatus{" << static_cast<uint32_t>(status) << "}";
    }
}

std::ostream& operator<<(std::ostream& out, const Frame& frame)
{
    std::ios oldState(nullptr);
    oldState.copyfmt(out);

    out
        << "lin::Frame{id=" << static_cast<uint16_t>(frame.id)
        << ", cs=" << to_string(frame.checksumModel)
        << ", dl=" << static_cast<uint16_t>(frame.dataLength)
        << ", d={" << std::hex << std::setfill('0')
        << std::setw(2) << static_cast<uint16_t>(frame.data[0]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[1]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[2]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[3]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[4]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[5]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[6]) << " "
        << std::setw(2) << static_cast<uint16_t>(frame.data[7]) << "}}";
    out.copyfmt(oldState);
    return out;
}

std::ostream& operator<<(std::ostream& out, const SendFrameRequest& request)
{
    return out
        << "lin::SendFrameRequest{fr=" << request.frame
        << ", rt=" << request.responseType
        << "}";
}

std::ostream& operator<<(std::ostream& out, const SendFrameHeaderRequest& request)
{
    return out << "LinSendHeaderRequest{id=" << request.id << "}";
}

std::ostream& operator<<(std::ostream& out, const Transmission& transmission)
{
    auto timestamp = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(transmission.timestamp);
    return out
        << "lin::Transmission{" << transmission.frame
        << ", status= " << transmission.status
        << ", time=" << timestamp.count()
        << "ms}";
}
std::ostream& operator<<(std::ostream& out, const WakeupPulse& pulse)
{
    auto timestamp = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(pulse.timestamp);
    return out << "LinWakeupPulse{@" << pulse.timestamp.count() << "ms}";
}

std::ostream& operator<<(std::ostream& out, const ControllerConfig& controllerConfig)
{
    out << "lin::ControllerConfig{br=" << controllerConfig.baudRate
        << ", mode=" << controllerConfig.controllerMode
        << ", responses=[";
    if (controllerConfig.frameResponses.size() > 0)
    {
        out << controllerConfig.frameResponses[0].frame.id;
        for (int i = 1; i < controllerConfig.frameResponses.size(); ++i)
        {
            out << "," << controllerConfig.frameResponses[1].frame.id;
        }
    }
    out << "]}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const ControllerStatusUpdate& controllerStatusUpdate)
{
    auto timestamp = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(controllerStatusUpdate.timestamp);
    return out
        << "lin::ControllerStatusUpdate{" << controllerStatusUpdate.status
        << " @" << timestamp.count() << "ms}";
}

std::ostream& operator<<(std::ostream& out, const FrameResponseUpdate& frameResponseUpdate)
{
    auto& responses = frameResponseUpdate.frameResponses;
    out << "LinFrameResponseUpdate{[";

    if (responses.size() > 0)
    {
        out << responses[0].frame.id;
        for (int i = 1; i < responses.size(); ++i)
        {
            out << "," << responses[1].frame.id;
        }
    }

    out << "]}";
    return out;
}


    
} // namespace lin
} // namespace sim
} // namespace ib
