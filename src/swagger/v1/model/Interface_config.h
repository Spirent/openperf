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
 * Interface_config.h
 *
 * Interface configuration
 */

#ifndef Interface_config_H_
#define Interface_config_H_


#include "ModelBase.h"

#include <string>
#include "InterfaceProtocolConfig.h"
#include <vector>

namespace swagger {
namespace v1 {
namespace model {

/// <summary>
/// Interface configuration
/// </summary>
class  Interface_config
    : public ModelBase
{
public:
    Interface_config();
    virtual ~Interface_config();

    /////////////////////////////////////////////
    /// ModelBase overrides

    void validate() override;

    nlohmann::json toJson() const override;
    void fromJson(nlohmann::json& json) override;

    /////////////////////////////////////////////
    /// Interface_config members

    /// <summary>
    /// A stack of protocol configurations, beginning with the outermost protocol (i.e. closest to the physical port) 
    /// </summary>
    std::vector<std::shared_ptr<InterfaceProtocolConfig>>& getProtocols();
        /// <summary>
    /// Berkley Packet Filter (BPF) rules that matches input packets for this interface. An empty rule, the default, matches all packets. 
    /// </summary>
    std::string getRxFilter() const;
    void setRxFilter(std::string value);
    bool rxFilterIsSet() const;
    void unsetRx_filter();
    /// <summary>
    /// Berkley Packet Filter (BPF) rules that matches output packets for this interface. An empty rule, the default, matches all packets. 
    /// </summary>
    std::string getTxFilter() const;
    void setTxFilter(std::string value);
    bool txFilterIsSet() const;
    void unsetTx_filter();

protected:
    std::vector<std::shared_ptr<InterfaceProtocolConfig>> m_Protocols;

    std::string m_Rx_filter;
    bool m_Rx_filterIsSet;
    std::string m_Tx_filter;
    bool m_Tx_filterIsSet;
};

}
}
}

#endif /* Interface_config_H_ */
