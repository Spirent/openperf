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


#include "BulkStartPacketGeneratorsResponse.h"

namespace swagger {
namespace v1 {
namespace model {

BulkStartPacketGeneratorsResponse::BulkStartPacketGeneratorsResponse()
{
    
}

BulkStartPacketGeneratorsResponse::~BulkStartPacketGeneratorsResponse()
{
}

void BulkStartPacketGeneratorsResponse::validate()
{
    // TODO: implement validation
}

nlohmann::json BulkStartPacketGeneratorsResponse::toJson() const
{
    nlohmann::json val = nlohmann::json::object();

    {
        nlohmann::json jsonArray;
        for( auto& item : m_Items )
        {
            jsonArray.push_back(ModelBase::toJson(item));
        }
        val["items"] = jsonArray;
            }
    

    return val;
}

void BulkStartPacketGeneratorsResponse::fromJson(nlohmann::json& val)
{
    {
        m_Items.clear();
        nlohmann::json jsonArray;
                for( auto& item : val["items"] )
        {
            
            if(item.is_null())
            {
                m_Items.push_back( std::shared_ptr<PacketGeneratorResult>(nullptr) );
            }
            else
            {
                std::shared_ptr<PacketGeneratorResult> newItem(new PacketGeneratorResult());
                newItem->fromJson(item);
                m_Items.push_back( newItem );
            }
            
        }
    }
    
}


std::vector<std::shared_ptr<PacketGeneratorResult>>& BulkStartPacketGeneratorsResponse::getItems()
{
    return m_Items;
}

}
}
}
