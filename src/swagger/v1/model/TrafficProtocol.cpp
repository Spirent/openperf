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


#include "TrafficProtocol.h"

namespace swagger {
namespace v1 {
namespace model {

TrafficProtocol::TrafficProtocol()
{
    m_ModifiersIsSet = false;
    m_CustomIsSet = false;
    m_EthernetIsSet = false;
    m_Ipv4IsSet = false;
    m_Ipv6IsSet = false;
    m_MplsIsSet = false;
    m_TcpIsSet = false;
    m_UdpIsSet = false;
    m_VlanIsSet = false;
    
}

TrafficProtocol::~TrafficProtocol()
{
}

void TrafficProtocol::validate()
{
    // TODO: implement validation
}

nlohmann::json TrafficProtocol::toJson() const
{
    nlohmann::json val = nlohmann::json::object();

    if(m_ModifiersIsSet)
    {
        val["modifiers"] = ModelBase::toJson(m_Modifiers);
    }
    if(m_CustomIsSet)
    {
        val["custom"] = ModelBase::toJson(m_Custom);
    }
    if(m_EthernetIsSet)
    {
        val["ethernet"] = ModelBase::toJson(m_Ethernet);
    }
    if(m_Ipv4IsSet)
    {
        val["ipv4"] = ModelBase::toJson(m_Ipv4);
    }
    if(m_Ipv6IsSet)
    {
        val["ipv6"] = ModelBase::toJson(m_Ipv6);
    }
    if(m_MplsIsSet)
    {
        val["mpls"] = ModelBase::toJson(m_Mpls);
    }
    if(m_TcpIsSet)
    {
        val["tcp"] = ModelBase::toJson(m_Tcp);
    }
    if(m_UdpIsSet)
    {
        val["udp"] = ModelBase::toJson(m_Udp);
    }
    if(m_VlanIsSet)
    {
        val["vlan"] = ModelBase::toJson(m_Vlan);
    }
    

    return val;
}

void TrafficProtocol::fromJson(nlohmann::json& val)
{
    if(val.find("modifiers") != val.end())
    {
        if(!val["modifiers"].is_null())
        {
            std::shared_ptr<TrafficProtocol_modifiers> newItem(new TrafficProtocol_modifiers());
            newItem->fromJson(val["modifiers"]);
            setModifiers( newItem );
        }
        
    }
    if(val.find("custom") != val.end())
    {
        if(!val["custom"].is_null())
        {
            std::shared_ptr<PacketProtocolCustom> newItem(new PacketProtocolCustom());
            newItem->fromJson(val["custom"]);
            setCustom( newItem );
        }
        
    }
    if(val.find("ethernet") != val.end())
    {
        if(!val["ethernet"].is_null())
        {
            std::shared_ptr<PacketProtocolEthernet> newItem(new PacketProtocolEthernet());
            newItem->fromJson(val["ethernet"]);
            setEthernet( newItem );
        }
        
    }
    if(val.find("ipv4") != val.end())
    {
        if(!val["ipv4"].is_null())
        {
            std::shared_ptr<PacketProtocolIpv4> newItem(new PacketProtocolIpv4());
            newItem->fromJson(val["ipv4"]);
            setIpv4( newItem );
        }
        
    }
    if(val.find("ipv6") != val.end())
    {
        if(!val["ipv6"].is_null())
        {
            std::shared_ptr<PacketProtocolIpv6> newItem(new PacketProtocolIpv6());
            newItem->fromJson(val["ipv6"]);
            setIpv6( newItem );
        }
        
    }
    if(val.find("mpls") != val.end())
    {
        if(!val["mpls"].is_null())
        {
            std::shared_ptr<PacketProtocolMpls> newItem(new PacketProtocolMpls());
            newItem->fromJson(val["mpls"]);
            setMpls( newItem );
        }
        
    }
    if(val.find("tcp") != val.end())
    {
        if(!val["tcp"].is_null())
        {
            std::shared_ptr<PacketProtocolTcp> newItem(new PacketProtocolTcp());
            newItem->fromJson(val["tcp"]);
            setTcp( newItem );
        }
        
    }
    if(val.find("udp") != val.end())
    {
        if(!val["udp"].is_null())
        {
            std::shared_ptr<PacketProtocolUdp> newItem(new PacketProtocolUdp());
            newItem->fromJson(val["udp"]);
            setUdp( newItem );
        }
        
    }
    if(val.find("vlan") != val.end())
    {
        if(!val["vlan"].is_null())
        {
            std::shared_ptr<PacketProtocolVlan> newItem(new PacketProtocolVlan());
            newItem->fromJson(val["vlan"]);
            setVlan( newItem );
        }
        
    }
    
}


std::shared_ptr<TrafficProtocol_modifiers> TrafficProtocol::getModifiers() const
{
    return m_Modifiers;
}
void TrafficProtocol::setModifiers(std::shared_ptr<TrafficProtocol_modifiers> value)
{
    m_Modifiers = value;
    m_ModifiersIsSet = true;
}
bool TrafficProtocol::modifiersIsSet() const
{
    return m_ModifiersIsSet;
}
void TrafficProtocol::unsetModifiers()
{
    m_ModifiersIsSet = false;
}
std::shared_ptr<PacketProtocolCustom> TrafficProtocol::getCustom() const
{
    return m_Custom;
}
void TrafficProtocol::setCustom(std::shared_ptr<PacketProtocolCustom> value)
{
    m_Custom = value;
    m_CustomIsSet = true;
}
bool TrafficProtocol::customIsSet() const
{
    return m_CustomIsSet;
}
void TrafficProtocol::unsetCustom()
{
    m_CustomIsSet = false;
}
std::shared_ptr<PacketProtocolEthernet> TrafficProtocol::getEthernet() const
{
    return m_Ethernet;
}
void TrafficProtocol::setEthernet(std::shared_ptr<PacketProtocolEthernet> value)
{
    m_Ethernet = value;
    m_EthernetIsSet = true;
}
bool TrafficProtocol::ethernetIsSet() const
{
    return m_EthernetIsSet;
}
void TrafficProtocol::unsetEthernet()
{
    m_EthernetIsSet = false;
}
std::shared_ptr<PacketProtocolIpv4> TrafficProtocol::getIpv4() const
{
    return m_Ipv4;
}
void TrafficProtocol::setIpv4(std::shared_ptr<PacketProtocolIpv4> value)
{
    m_Ipv4 = value;
    m_Ipv4IsSet = true;
}
bool TrafficProtocol::ipv4IsSet() const
{
    return m_Ipv4IsSet;
}
void TrafficProtocol::unsetIpv4()
{
    m_Ipv4IsSet = false;
}
std::shared_ptr<PacketProtocolIpv6> TrafficProtocol::getIpv6() const
{
    return m_Ipv6;
}
void TrafficProtocol::setIpv6(std::shared_ptr<PacketProtocolIpv6> value)
{
    m_Ipv6 = value;
    m_Ipv6IsSet = true;
}
bool TrafficProtocol::ipv6IsSet() const
{
    return m_Ipv6IsSet;
}
void TrafficProtocol::unsetIpv6()
{
    m_Ipv6IsSet = false;
}
std::shared_ptr<PacketProtocolMpls> TrafficProtocol::getMpls() const
{
    return m_Mpls;
}
void TrafficProtocol::setMpls(std::shared_ptr<PacketProtocolMpls> value)
{
    m_Mpls = value;
    m_MplsIsSet = true;
}
bool TrafficProtocol::mplsIsSet() const
{
    return m_MplsIsSet;
}
void TrafficProtocol::unsetMpls()
{
    m_MplsIsSet = false;
}
std::shared_ptr<PacketProtocolTcp> TrafficProtocol::getTcp() const
{
    return m_Tcp;
}
void TrafficProtocol::setTcp(std::shared_ptr<PacketProtocolTcp> value)
{
    m_Tcp = value;
    m_TcpIsSet = true;
}
bool TrafficProtocol::tcpIsSet() const
{
    return m_TcpIsSet;
}
void TrafficProtocol::unsetTcp()
{
    m_TcpIsSet = false;
}
std::shared_ptr<PacketProtocolUdp> TrafficProtocol::getUdp() const
{
    return m_Udp;
}
void TrafficProtocol::setUdp(std::shared_ptr<PacketProtocolUdp> value)
{
    m_Udp = value;
    m_UdpIsSet = true;
}
bool TrafficProtocol::udpIsSet() const
{
    return m_UdpIsSet;
}
void TrafficProtocol::unsetUdp()
{
    m_UdpIsSet = false;
}
std::shared_ptr<PacketProtocolVlan> TrafficProtocol::getVlan() const
{
    return m_Vlan;
}
void TrafficProtocol::setVlan(std::shared_ptr<PacketProtocolVlan> value)
{
    m_Vlan = value;
    m_VlanIsSet = true;
}
bool TrafficProtocol::vlanIsSet() const
{
    return m_VlanIsSet;
}
void TrafficProtocol::unsetVlan()
{
    m_VlanIsSet = false;
}

}
}
}
