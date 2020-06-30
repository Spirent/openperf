/**
* OpenPerf API
* REST API interface for OpenPerf
*
* OpenAPI spec version: 1
* Contact: support@spirent.com
*
* NOTE: This class is auto generated by the swagger code generator program.
* https://github.com/swagger-api/swagger-codegen.git
* Do not edit the class manually.
*/


#include "PacketAnalyzerProtocolCounters_protocol.h"

namespace swagger {
namespace v1 {
namespace model {

PacketAnalyzerProtocolCounters_protocol::PacketAnalyzerProtocolCounters_protocol()
{
    m_Tcp = 0L;
    m_Udp = 0L;
    m_Fragmented = 0L;
    m_Sctp = 0L;
    m_Icmp = 0L;
    m_Non_fragmented = 0L;
    m_Igmp = 0L;
    
}

PacketAnalyzerProtocolCounters_protocol::~PacketAnalyzerProtocolCounters_protocol()
{
}

void PacketAnalyzerProtocolCounters_protocol::validate()
{
    // TODO: implement validation
}

nlohmann::json PacketAnalyzerProtocolCounters_protocol::toJson() const
{
    nlohmann::json val = nlohmann::json::object();

    val["tcp"] = m_Tcp;
    val["udp"] = m_Udp;
    val["fragmented"] = m_Fragmented;
    val["sctp"] = m_Sctp;
    val["icmp"] = m_Icmp;
    val["non_fragmented"] = m_Non_fragmented;
    val["igmp"] = m_Igmp;
    

    return val;
}

void PacketAnalyzerProtocolCounters_protocol::fromJson(nlohmann::json& val)
{
    setTcp(val.at("tcp"));
    setUdp(val.at("udp"));
    setFragmented(val.at("fragmented"));
    setSctp(val.at("sctp"));
    setIcmp(val.at("icmp"));
    setNonFragmented(val.at("non_fragmented"));
    setIgmp(val.at("igmp"));
    
}


int64_t PacketAnalyzerProtocolCounters_protocol::getTcp() const
{
    return m_Tcp;
}
void PacketAnalyzerProtocolCounters_protocol::setTcp(int64_t value)
{
    m_Tcp = value;
    
}
int64_t PacketAnalyzerProtocolCounters_protocol::getUdp() const
{
    return m_Udp;
}
void PacketAnalyzerProtocolCounters_protocol::setUdp(int64_t value)
{
    m_Udp = value;
    
}
int64_t PacketAnalyzerProtocolCounters_protocol::getFragmented() const
{
    return m_Fragmented;
}
void PacketAnalyzerProtocolCounters_protocol::setFragmented(int64_t value)
{
    m_Fragmented = value;
    
}
int64_t PacketAnalyzerProtocolCounters_protocol::getSctp() const
{
    return m_Sctp;
}
void PacketAnalyzerProtocolCounters_protocol::setSctp(int64_t value)
{
    m_Sctp = value;
    
}
int64_t PacketAnalyzerProtocolCounters_protocol::getIcmp() const
{
    return m_Icmp;
}
void PacketAnalyzerProtocolCounters_protocol::setIcmp(int64_t value)
{
    m_Icmp = value;
    
}
int64_t PacketAnalyzerProtocolCounters_protocol::getNonFragmented() const
{
    return m_Non_fragmented;
}
void PacketAnalyzerProtocolCounters_protocol::setNonFragmented(int64_t value)
{
    m_Non_fragmented = value;
    
}
int64_t PacketAnalyzerProtocolCounters_protocol::getIgmp() const
{
    return m_Igmp;
}
void PacketAnalyzerProtocolCounters_protocol::setIgmp(int64_t value)
{
    m_Igmp = value;
    
}

}
}
}
