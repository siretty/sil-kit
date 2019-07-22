// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <array>
#include <chrono>
#include <vector>

#include "ib/IbMacros.hpp"
#include "ib/util/vector_view.hpp"
#include "ib/sim/datatypes.hpp"

// ================================================================================
//  Ethernet specific data types
// ================================================================================
namespace ib {
namespace sim {
//! The Ethernet namespace
namespace eth {

//! \brief Representing a MAC address, i.e. FF:FF:FF:FF:FF:FF
using EthMac = std::array<uint8_t, 6>;

/*! \brief VLAN Identifier
 * 
 * VLAN Identifier (12 Bit, range 1-4094 for a valid identifier or zero when no identifier is used).
 * The identifier 0x000 in a frame indicates that the frame has no VLAN identifier.
 * The identifier 0xFFF is reserved for special use inside switches and must not be used.
 */
using EthVid = uint16_t;

//! \brief Tag Control Information (TCI), part of the VLAN tag.
struct EthTagControlInformation
{
    uint8_t pcp : 3; //!< Priority code point (0 lowest priority, 7 highest priority)
    uint8_t dei : 1; //!< Drop eligible indicator
    EthVid vid : 12; //!< VLAN identifier
};

//! \brief An Ethernet frame
class EthFrame
{
public:
    IntegrationBusAPI EthFrame() = default;
    IntegrationBusAPI EthFrame(const EthFrame& other) = default;
    IntegrationBusAPI EthFrame(EthFrame&& other) = default;
    IntegrationBusAPI explicit EthFrame(const std::vector<uint8_t>& rawFrame);
    /*! \brief Constructor of an ethernet frame
    *
    * \param rawFrame The raw ethernet frame.
    */
    IntegrationBusAPI explicit EthFrame(std::vector<uint8_t>&& rawFrame);
    IntegrationBusAPI explicit EthFrame(const uint8_t* rawFrame, size_t size_t);

    IntegrationBusAPI auto operator=(const EthFrame& other) -> EthFrame & = default;
    IntegrationBusAPI auto operator=(EthFrame&& other) -> EthFrame& = default;

    //! \brief Get the destination MAC address from the ethernet frame.
    IntegrationBusAPI auto GetDestinationMac() const -> EthMac;
    //! \brief Set the destination MAC address of the ethernet frame.
    IntegrationBusAPI void SetDestinationMac(const EthMac& mac);
    //! \brief Get the source MAC address from the ethernet frame.
    IntegrationBusAPI auto GetSourceMac() const -> EthMac;
    //! \brief Set the source MAC address of the ethernet frame.
    IntegrationBusAPI void SetSourceMac(const EthMac& mac);

    //! \brief Get the vlan tag from the ethernet frame.
    IntegrationBusAPI auto GetVlanTag() const -> EthTagControlInformation;
    //! \brief Set the vlan tag of the ethernet frame.
    IntegrationBusAPI void SetVlanTag(const EthTagControlInformation& tci);

    //! \brief Get the ether type.
    IntegrationBusAPI auto GetEtherType() const -> uint16_t;
    //! \brief Set the ether type.
    IntegrationBusAPI void SetEtherType(uint16_t etherType);

    //! \brief Get the size of the ethernet frame.
    IntegrationBusAPI auto GetFrameSize() const -> size_t;
    //! \brief Get the size of the ethernet frame's header.
    IntegrationBusAPI auto GetHeaderSize() const -> size_t;
    //! \brief Get the size of the ethernet frame's payload.
    IntegrationBusAPI auto GetPayloadSize() const -> size_t;

    //! \brief Get the payload of the ethernet frame.
    IntegrationBusAPI auto GetPayload() -> util::vector_view<uint8_t>;
    IntegrationBusAPI auto GetPayload() const -> util::vector_view<const uint8_t>;
    //! \brief Set the payload of the ethernet frame.
    IntegrationBusAPI void SetPayload(const std::vector<uint8_t>& payload);
    IntegrationBusAPI void SetPayload(const uint8_t* payload, size_t size);

    //! \brief Get the raw ethernet frame.
    IntegrationBusAPI auto RawFrame() const -> const std::vector<uint8_t>&;

private:
    std::vector<uint8_t> _rawFrame;
};

//! \brief An Ethernet transmit id
using EthTxId = uint32_t;

/*! \brief An Ethernet frame including ID and timestamp, sent in both directions
 * 
 * Directions:
 * - From: Ethernet Controller  To: Network Simulator
 * - From: Network Simulator    To: Ethernet Controller
 */
struct EthMessage
{
    EthTxId transmitId; //!< Set by the EthController, used for acknowledgements
    std::chrono::nanoseconds timestamp; //!< Reception time

    EthFrame ethFrame; //!< The Ethernet frame
};

//! \brief Acknowledgement status for an EthTransmitRequest
enum class EthTransmitStatus : uint8_t
{
    Transmitted = 0, //!< The message was successfully transmitted on the Ethernet link.
    ControllerInactive = 1, //!< The transmit request was rejected, because the Ethernet controller is not active.
    LinkDown = 2, //!< The transmit request was rejected, because the Ethernet link is down.
    Dropped = 3, //!< The transmit request was dropped, because the transmit queue is full.
    DuplicatedTransmitId = 4, //!< The  transmit request was rejected, because there is already another request with the same transmitId.
    InvalidFrameFormat = 5 //!< The given raw Ethernet frame is ill formated (For example frame length is too small or too large, wrong order of VLAN tags).
};

/*! \brief Publishes status of the simulated Ethernet controller
 * 
 * Directions:
 * - From: Network Simulator  To: Ethernet Controller
 */
struct EthTransmitAcknowledge
{
    EthTxId transmitId;   //!< Identifies the EthTransmitRequest, to which this EthTransmitAcknowledge refers to.
    std::chrono::nanoseconds timestamp; //!< Timestamp of the Ethernet acknowledge.
    EthTransmitStatus status; //!< Status of the EthTransmitRequest.
};

//! \brief State of the Ethernet controller
enum class EthState : uint8_t
{
    Inactive = 0, //!< The Ethernet controller is switched off (default after reset).
    LinkDown = 1, //!< The Ethernet controller is active, but a link to another Ethernet controller in not yet established.
    LinkUp = 2, //!< The Ethernet controller is active and the link to another Ethernet controller is established.
};

/*! \brief Publishes status of the simulated Ethernet controller
 * 
 * Directions:
 * - From: Network Simulator  To: Ethernet Controller
 */
struct EthStatus
{
    std::chrono::nanoseconds timestamp; //!< Timestamp of the status change.
    EthState state; //!< State of the Ethernet controller.
    uint32_t bitRate; //!< Bit rate in kBit/sec of the link when in state LinkUp, otherwise zero.
};

/*! \brief Mode for switching an Ethernet Controller on or off
 */
enum class EthMode : uint8_t
{
    Inactive = 0, //!< The controller is inactive (default after reset).
    Active = 1, //!< The controller is active.
};

/*! \brief Set the Mode of the Ethernet Controller.
 * 
 * Directions:
 * - From: Ethernet Controller  To: Network Simulator
 */
struct EthSetMode
{
    EthMode mode; //!< EthMode that the Ethernet controller should reach.
};
  
} // namespace eth
} // namespace sim
} // namespace ib
