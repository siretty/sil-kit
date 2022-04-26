// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "ib/sim/can/CanDatatypes.hpp"

#include "IIbReceiver.hpp"
#include "IIbSender.hpp"

namespace ib {
namespace sim {
namespace can {

/*! \brief IIbToCanController interface
 *
 *  Used by the Participant, implemented by the CanController
 */
class IIbToCanControllerProxy
    : public mw::IIbReceiver<CanFrameEvent, CanFrameTransmitEvent, CanControllerStatus>
    , public mw::IIbSender<CanFrameEvent, CanConfigureBaudrate, CanSetControllerMode>
{
};


} // namespace can
} // namespace sim
} // namespace ib
