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


#include "TimeSourceStats.h"

namespace swagger {
namespace v1 {
namespace model {

TimeSourceStats::TimeSourceStats()
{
    m_NtpIsSet = false;
    m_SystemIsSet = false;
    
}

TimeSourceStats::~TimeSourceStats()
{
}

void TimeSourceStats::validate()
{
    // TODO: implement validation
}

nlohmann::json TimeSourceStats::toJson() const
{
    nlohmann::json val = nlohmann::json::object();

    if(m_NtpIsSet)
    {
        val["ntp"] = ModelBase::toJson(m_Ntp);
    }
    if(m_SystemIsSet)
    {
        val["system"] = ModelBase::toJson(m_System);
    }
    

    return val;
}

void TimeSourceStats::fromJson(nlohmann::json& val)
{
    if(val.find("ntp") != val.end())
    {
        if(!val["ntp"].is_null())
        {
            std::shared_ptr<TimeSourceStats_ntp> newItem(new TimeSourceStats_ntp());
            newItem->fromJson(val["ntp"]);
            setNtp( newItem );
        }
        
    }
    if(val.find("system") != val.end())
    {
        if(!val["system"].is_null())
        {
            std::shared_ptr<TimeSourceStats_system> newItem(new TimeSourceStats_system());
            newItem->fromJson(val["system"]);
            setSystem( newItem );
        }
        
    }
    
}


std::shared_ptr<TimeSourceStats_ntp> TimeSourceStats::getNtp() const
{
    return m_Ntp;
}
void TimeSourceStats::setNtp(std::shared_ptr<TimeSourceStats_ntp> value)
{
    m_Ntp = value;
    m_NtpIsSet = true;
}
bool TimeSourceStats::ntpIsSet() const
{
    return m_NtpIsSet;
}
void TimeSourceStats::unsetNtp()
{
    m_NtpIsSet = false;
}
std::shared_ptr<TimeSourceStats_system> TimeSourceStats::getSystem() const
{
    return m_System;
}
void TimeSourceStats::setSystem(std::shared_ptr<TimeSourceStats_system> value)
{
    m_System = value;
    m_SystemIsSet = true;
}
bool TimeSourceStats::systemIsSet() const
{
    return m_SystemIsSet;
}
void TimeSourceStats::unsetSystem()
{
    m_SystemIsSet = false;
}

}
}
}

