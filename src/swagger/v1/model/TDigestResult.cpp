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


#include "TDigestResult.h"

namespace swagger {
namespace v1 {
namespace model {

TDigestResult::TDigestResult()
{
    m_Id = "";
    m_IdIsSet = false;
    m_Generator_type = "";
    m_Generator_typeIsSet = false;
    m_Generator_id = "";
    m_Generator_idIsSet = false;
    m_Factor = 0;
    m_FactorIsSet = false;
    m_CentroidsIsSet = false;
    
}

TDigestResult::~TDigestResult()
{
}

void TDigestResult::validate()
{
    // TODO: implement validation
}

nlohmann::json TDigestResult::toJson() const
{
    nlohmann::json val = nlohmann::json::object();

    if(m_IdIsSet)
    {
        val["id"] = ModelBase::toJson(m_Id);
    }
    if(m_Generator_typeIsSet)
    {
        val["generator_type"] = ModelBase::toJson(m_Generator_type);
    }
    if(m_Generator_idIsSet)
    {
        val["generator_id"] = ModelBase::toJson(m_Generator_id);
    }
    if(m_FactorIsSet)
    {
        val["factor"] = m_Factor;
    }
    {
        nlohmann::json jsonArray;
        for( auto& item : m_Centroids )
        {
            jsonArray.push_back(ModelBase::toJson(item));
        }
        
        if(jsonArray.size() > 0)
        {
            val["centroids"] = jsonArray;
        }
    }
    

    return val;
}

void TDigestResult::fromJson(nlohmann::json& val)
{
    if(val.find("id") != val.end())
    {
        setId(val.at("id"));
        
    }
    if(val.find("generator_type") != val.end())
    {
        setGeneratorType(val.at("generator_type"));
        
    }
    if(val.find("generator_id") != val.end())
    {
        setGeneratorId(val.at("generator_id"));
        
    }
    if(val.find("factor") != val.end())
    {
        setFactor(val.at("factor"));
    }
    {
        m_Centroids.clear();
        nlohmann::json jsonArray;
        if(val.find("centroids") != val.end())
        {
        for( auto& item : val["centroids"] )
        {
            
            if(item.is_null())
            {
                m_Centroids.push_back( std::shared_ptr<TDigestCentroid>(nullptr) );
            }
            else
            {
                std::shared_ptr<TDigestCentroid> newItem(new TDigestCentroid());
                newItem->fromJson(item);
                m_Centroids.push_back( newItem );
            }
            
        }
        }
    }
    
}


std::string TDigestResult::getId() const
{
    return m_Id;
}
void TDigestResult::setId(std::string value)
{
    m_Id = value;
    m_IdIsSet = true;
}
bool TDigestResult::idIsSet() const
{
    return m_IdIsSet;
}
void TDigestResult::unsetId()
{
    m_IdIsSet = false;
}
std::string TDigestResult::getGeneratorType() const
{
    return m_Generator_type;
}
void TDigestResult::setGeneratorType(std::string value)
{
    m_Generator_type = value;
    m_Generator_typeIsSet = true;
}
bool TDigestResult::generatorTypeIsSet() const
{
    return m_Generator_typeIsSet;
}
void TDigestResult::unsetGenerator_type()
{
    m_Generator_typeIsSet = false;
}
std::string TDigestResult::getGeneratorId() const
{
    return m_Generator_id;
}
void TDigestResult::setGeneratorId(std::string value)
{
    m_Generator_id = value;
    m_Generator_idIsSet = true;
}
bool TDigestResult::generatorIdIsSet() const
{
    return m_Generator_idIsSet;
}
void TDigestResult::unsetGenerator_id()
{
    m_Generator_idIsSet = false;
}
int32_t TDigestResult::getFactor() const
{
    return m_Factor;
}
void TDigestResult::setFactor(int32_t value)
{
    m_Factor = value;
    m_FactorIsSet = true;
}
bool TDigestResult::factorIsSet() const
{
    return m_FactorIsSet;
}
void TDigestResult::unsetFactor()
{
    m_FactorIsSet = false;
}
std::vector<std::shared_ptr<TDigestCentroid>>& TDigestResult::getCentroids()
{
    return m_Centroids;
}
bool TDigestResult::centroidsIsSet() const
{
    return m_CentroidsIsSet;
}
void TDigestResult::unsetCentroids()
{
    m_CentroidsIsSet = false;
}

}
}
}
