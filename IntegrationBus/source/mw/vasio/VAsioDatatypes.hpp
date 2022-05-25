// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <string>

#include "VAsioPeerInfo.hpp"
#include "ProtocolVersion.hpp" // for current ProtocolVersion in RegistryMsgHeader

namespace ib {
namespace mw {

struct RegistryMsgHeader
{
    std::array<uint8_t, 4> preambel{{'V', 'I', 'B', '-'}};
    // If versionHigh/Low changes here, update VIB version range .
    // Also, ensure backwards compatibility in the Ser/Des code path.
    // See VAsioProtcolVersion.hpp
    uint16_t versionHigh;
    uint16_t versionLow;
    RegistryMsgHeader()
        :versionHigh{(uint16_t)std::get<0>(CurrentProtocolVersion())}
        ,versionLow{(uint16_t)std::get<1>(CurrentProtocolVersion())}
    {
    }
};

struct VAsioMsgSubscriber
{
    EndpointId receiverIdx;
    std::string networkName;
    std::string msgTypeName;
    uint32_t version{0};
};

struct SubscriptionAcknowledge
{
    enum class Status : uint8_t {
        Failed = 0,
        Success = 1
    };
    Status status;
    VAsioMsgSubscriber subscriber;
};

struct ParticipantAnnouncement
{
    RegistryMsgHeader messageHeader;
    ib::mw::VAsioPeerInfo peerInfo;
};

struct ParticipantAnnouncementReply
{
    RegistryMsgHeader remoteHeader;
    enum class Status : uint8_t {
        Failed = 0,
        Success = 1
    };
    Status status{Status::Failed}; //default for failure to deserialize
    std::vector<VAsioMsgSubscriber> subscribers;
};

struct KnownParticipants
{
    RegistryMsgHeader messageHeader;
    std::vector<ib::mw::VAsioPeerInfo> peerInfos;
};

enum class RegistryMessageKind : uint8_t
{
    Invalid = 0,
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !! DO NOT CHANGE THE VALUE OF ParticipantAnnouncement !!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // The ParticipantAnnouncement is the first message transmitted over a new
    // connection and carries the protocol version. Thus, changing the enum
    // value of ParticipantAnnouncement will break protocol break detections
    // with older participants.
    ParticipantAnnouncement = 1,
    ParticipantAnnouncementReply = 2,
    KnownParticipants = 3
};

// ================================================================================
//  Inline Implementations
// ================================================================================
inline bool operator!=(const RegistryMsgHeader& lhs, const RegistryMsgHeader& rhs)
{
    return lhs.preambel != rhs.preambel
        || lhs.versionHigh != rhs.versionHigh
        || lhs.versionLow != rhs.versionLow;
}

inline bool operator==(const RegistryMsgHeader& lhs, const RegistryMsgHeader& rhs)
{
    return lhs.preambel == rhs.preambel
        && lhs.versionHigh == rhs.versionHigh
        && lhs.versionLow == rhs.versionLow
        ;
}

inline bool operator==(const VAsioMsgSubscriber& lhs, const VAsioMsgSubscriber& rhs)
{
    return lhs.receiverIdx == rhs.receiverIdx 
        && lhs.networkName == rhs.networkName
        && lhs.msgTypeName == rhs.msgTypeName;
}

} // namespace mw
} // namespace ib
