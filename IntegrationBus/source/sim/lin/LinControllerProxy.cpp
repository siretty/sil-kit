// Copyright (c)  Vector Informatik GmbH. All rights reserved.

#include "LinControllerProxy.hpp"

#include <iostream>

#include "ib/mw/IComAdapter.hpp"

namespace ib {
namespace sim {
namespace lin {

namespace {
    constexpr LinId   GotosleepId{0x3c};
    constexpr Payload GotosleepPayload{8, {0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
}

LinControllerProxy::LinControllerProxy(mw::IComAdapter* comAdapter)
: _comAdapter(comAdapter)
{
}

void LinControllerProxy::SetMasterMode()
{
    if (_controllerMode != ControllerMode::Inactive)
    {
        throw std::runtime_error{"LinControllerProxy::SetMasterMode() must only be called on unconfigured controllers!"};
    }
    _configuredControllerMode = ControllerMode::Master;
    _controllerMode = ControllerMode::Master;
    sendControllerConfig();
}

void LinControllerProxy::SetSlaveMode()
{
    if (_controllerMode != ControllerMode::Inactive)
    {
        throw std::runtime_error{"LinController::SetSlaveMode() must only be called on unconfigured controllers!"};
    }

    // set slave mode
    _configuredControllerMode = ControllerMode::Slave;
    _controllerMode = ControllerMode::Slave;
    sendControllerConfig();
}

void LinControllerProxy::SetBaudRate(uint32_t baudrate)
{
    _baudrate = baudrate;
    sendControllerConfig();
}

void LinControllerProxy::SetSleepMode()
{
    if (_configuredControllerMode == ControllerMode::Inactive)
    {
        throw std::runtime_error{"LinControllerProxy:SetSleepMode() must not be called before SetMasterMode() or SetSlaveMode()"};
    }

    _controllerMode = ControllerMode::Sleep;
    sendControllerConfig();
}

void LinControllerProxy::SetOperationalMode()
{
    if (_controllerMode != ControllerMode::Sleep)
    {
        throw std::runtime_error{"LinController:SetOperationalMode() must only be called when controller is in sleep mode"};
    }

    // restore configured controller mode
    _controllerMode = _configuredControllerMode;
    sendControllerConfig();
}

void LinControllerProxy::sendControllerConfig()
{
    ControllerConfig config;
    config.controllerMode = _controllerMode;
    config.baudrate = _baudrate;

    SendIbMessage(config);
}

void LinControllerProxy::SetSlaveConfiguration(const SlaveConfiguration& config)
{
    SendIbMessage(config);
}

void LinControllerProxy::SetResponse(LinId linId, const Payload& payload)
{
    SlaveResponse response;
    response.linId = linId;
    response.payload = payload;
    response.checksumModel = ChecksumModel::Undefined;

    SendIbMessage(response);
}

void LinControllerProxy::SetResponseWithChecksum(LinId linId, const Payload& payload, ChecksumModel checksumModel)
{
    if (checksumModel == ChecksumModel::Undefined)
    {
        std::cerr << "WARNING: LinControllerProxy::SetResponseWithChecksum() was called with ChecksumModel::Undefined, which does NOT alter the checksum model!\n";
    }

    SlaveResponse response;
    response.linId = linId;
    response.payload = payload;
    response.checksumModel = checksumModel;

    SendIbMessage(response);
}

void LinControllerProxy::RemoveResponse(LinId /*linId*/)
{
    throw std::runtime_error("LinControllerProxy::RemoveResponse not implemented");
}

void LinControllerProxy::SendWakeupRequest()
{
    if (_controllerMode != ControllerMode::Sleep)
    {
        std::cerr << "ERROR: LinController::SendWakeupRequest() must only be called in sleep mode!" << std::endl;
        throw std::logic_error("LinController::SendWakeupRequest() must only be called in sleep mode!");
    }

    SendIbMessage(WakeupRequest{});
}

void LinControllerProxy::SendMessage(const LinMessage& msg)
{
    auto msgCopy = msg;
    msgCopy.status = MessageStatus::TxSuccess;

    SendIbMessage(msgCopy);
}

void LinControllerProxy::RequestMessage(const RxRequest& request)
{
    SendIbMessage(request);
}

void LinControllerProxy::SendGoToSleep()
{
    if (_controllerMode != ControllerMode::Master)
    {
        std::cerr << "ERROR: LinControllerProxy::SendGoToSleep() must only be called in master mode!" << std::endl;
        throw std::logic_error("LinControllerProxy::SendGoToSleep() must only be called in master mode!");
    }

    LinMessage gotosleep;

    gotosleep.status = MessageStatus::TxSuccess;
    gotosleep.checksumModel = ChecksumModel::Classic;
    gotosleep.linId = GotosleepId;
    gotosleep.payload = GotosleepPayload;

    SendIbMessage(gotosleep);
}

void LinControllerProxy::RegisterTxCompleteHandler(TxCompleteHandler handler)
{
    RegisterHandler(std::move(handler));
}

void LinControllerProxy::RegisterReceiveMessageHandler(ReceiveMessageHandler handler)
{
    RegisterHandler(std::move(handler));
}

void LinControllerProxy::RegisterWakeupRequestHandler(WakeupRequestHandler handler)
{
    _wakeuprequestCallbacks.emplace_back(std::move(handler));
}

void LinControllerProxy::RegisterSleepCommandHandler(SleepCommandHandler handler)
{
    _gotosleepCallbacks.emplace_back(std::move(handler));
}

void LinControllerProxy::ReceiveIbMessage(mw::EndpointAddress from, const LinMessage& msg)
{
    if (from.participant == _endpointAddr.participant || from.endpoint != _endpointAddr.endpoint)
        return;

    CallHandlers(msg);

    if (msg.linId == GotosleepId && msg.payload == GotosleepPayload)
    {
        for (auto&& handler : _gotosleepCallbacks)
            handler(this);
    }

}

void LinControllerProxy::ReceiveIbMessage(mw::EndpointAddress from, const TxAcknowledge& msg)
{
    if (from.participant == _endpointAddr.participant || from.endpoint != _endpointAddr.endpoint)
        return;

    if (_controllerMode != ControllerMode::Master)
    {
        std::cerr << "LinControllerProxy"
                  << "{P=" << _endpointAddr.participant << ", E=" << _endpointAddr.endpoint << "}"
                  << " in non-Master mode received TxAcknowledge!";
        return;
    }

    CallHandlers(msg.status);
}

void LinControllerProxy::ReceiveIbMessage(mw::EndpointAddress from, const WakeupRequest& msg)
{
    if (_controllerMode != ControllerMode::Sleep)
    {
        std::cerr << "WARNING: Received WakeupRequest while not in sleep mode\n";
    }

    for (auto&& handler : _wakeuprequestCallbacks)
    {
        handler(this);
    }
}

void LinControllerProxy::SetEndpointAddress(const mw::EndpointAddress& endpointAddress)
{
    _endpointAddr = endpointAddress;
}

auto LinControllerProxy::EndpointAddress() const -> const mw::EndpointAddress&
{
    return _endpointAddr;
}

template<typename MsgT>
void LinControllerProxy::RegisterHandler(CallbackT<MsgT>&& handler)
{
    auto&& handlers = std::get<CallbackVector<MsgT>>(_callbacks);
    handlers.emplace_back(std::move(handler));
}

template<typename MsgT>
void LinControllerProxy::CallHandlers(const MsgT& msg)
{
    auto&& handlers = std::get<CallbackVector<MsgT>>(_callbacks);
    for (auto&& handler : handlers)
    {
        handler(this, msg);
    }
}

template <typename MsgT>
void LinControllerProxy::SendIbMessage(MsgT&& msg)
{
    _comAdapter->SendIbMessage(_endpointAddr, std::forward<MsgT>(msg));
}


} // namespace lin
} // namespace sim
} // namespace ib
