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
/*
 * PacketProtocolCustom.h
 *
 * Defines an arbitrary sequence of data
 */

#ifndef PacketProtocolCustom_H_
#define PacketProtocolCustom_H_


#include "ModelBase.h"

#include "BinaryString.h"

namespace swagger {
namespace v1 {
namespace model {

/// <summary>
/// Defines an arbitrary sequence of data
/// </summary>
class  PacketProtocolCustom
    : public ModelBase
{
public:
    PacketProtocolCustom();
    virtual ~PacketProtocolCustom();

    /////////////////////////////////////////////
    /// ModelBase overrides

    void validate() override;

    nlohmann::json toJson() const override;
    void fromJson(nlohmann::json& json) override;

    /////////////////////////////////////////////
    /// PacketProtocolCustom members

    /// <summary>
    /// 
    /// </summary>
    std::shared_ptr<BinaryString> getData() const;
    void setData(std::shared_ptr<BinaryString> value);
    
protected:
    std::shared_ptr<BinaryString> m_Data;

};

}
}
}

#endif /* PacketProtocolCustom_H_ */