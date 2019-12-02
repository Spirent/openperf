
#ifndef _OP_YAML_JSON_EMITTER_HPP_
#define _OP_YAML_JSON_EMITTER_HPP_

// Custom YAML -> JSON converter class.
// Uses yaml-cpp parsing events to output JSON that is
// Compatible with OpenPerf plugins.
// This is NOT a complete, standards-compliant converter.

#include "yaml-cpp/yaml.h"
#include "yaml-cpp/eventhandler.h"
#include <stack>

namespace openperf::config::file {

using namespace YAML;

class yaml_json_emitter : public YAML::EventHandler
{
public:
    yaml_json_emitter(std::ostringstream& output_string);

    void OnAlias(const Mark& mark __attribute__((unused)),
                 anchor_t anchor __attribute__((unused)))
    {}
    void OnDocumentStart(const Mark& mark __attribute__((unused))) {}
    void OnDocumentEnd() {}
    void OnMapStart(const Mark& mark,
                    const std::string& tag,
                    anchor_t anchor,
                    EmitterStyle::value style);
    void OnMapEnd();
    void OnNull(const Mark& mark __attribute__((unused)),
                anchor_t anchor __attribute__((unused)))
    {}
    void OnScalar(const Mark& mark,
                  const std::string& tag,
                  anchor_t anchor,
                  const std::string& value);
    void OnSequenceStart(const Mark& mark,
                         const std::string& tag,
                         anchor_t anchor,
                         EmitterStyle::value style);
    void OnSequenceEnd();

private:
    void BeginNode();

    YAML::Emitter m_emitter;

    struct State
    {
        enum value { WaitingForSequenceEntry, WaitingForKey, WaitingForValue };
    };
    std::stack<State::value> m_state_stack;
};
} // namespace openperf::config::file

#endif /*_OP_CONFIG_FILE_ADAPTER_HPP_*/
