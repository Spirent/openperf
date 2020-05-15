#include <cassert>

#include "yaml_json_emitter.hpp"
#include "op_config_file_utils.hpp"

namespace openperf::config::file {

yaml_json_emitter::yaml_json_emitter(std::ostringstream& output_stream)
    : m_emitter(YAML::Emitter(output_stream))
{}

void yaml_json_emitter::OnScalar(const Mark& mark __attribute__((unused)),
                                 const std::string& tag,
                                 anchor_t anchor __attribute__((unused)),
                                 const std::string& value)
{
    BeginNode();

    if (m_state_stack.empty()) return;

    // Need to handle keys and values slightly differently.
    if (m_state_stack.top() == State::WaitingForValue) {
        // JSON does not support non-string Keys.
        // Output all Keys as double-quoted strings.
        m_emitter << YAML::DoubleQuoted << value;
    } else if ((m_state_stack.top() == State::WaitingForKey)
               || (m_state_stack.top() == State::WaitingForSequenceEntry)) {
        // Tag of '!' means yaml-cpp read this node as a quoted string.
        if (tag == "!") {
            m_emitter << YAML::DoubleQuoted << value;
        } else if (value == "true" || value == "false") {
            // Boolean. Pass through as unquoted text.
            m_emitter << YAML::TrueFalseBool << value;
        } else if (op_config_is_number(value)) {
            m_emitter << value;
        } else {
            // Fall back on sending as a quoted string.
            m_emitter << YAML::DoubleQuoted << value;
        }
    } else {
        assert(false);
    }
}

void yaml_json_emitter::OnSequenceStart(const Mark& mark
                                        __attribute__((unused)),
                                        const std::string& tag
                                        __attribute__((unused)),
                                        anchor_t anchor __attribute__((unused)),
                                        EmitterStyle::value style
                                        __attribute__((unused)))
{
    BeginNode();
    m_emitter << Flow;
    m_emitter << BeginSeq;
    m_state_stack.push(State::WaitingForSequenceEntry);
}

void yaml_json_emitter::OnSequenceEnd()
{
    m_emitter << EndSeq;
    assert(m_state_stack.top() == State::WaitingForSequenceEntry);
    m_state_stack.pop();
}

void yaml_json_emitter::OnMapStart(const Mark& mark __attribute__((unused)),
                                   const std::string& tag
                                   __attribute__((unused)),
                                   anchor_t anchor __attribute__((unused)),
                                   EmitterStyle::value style
                                   __attribute__((unused)))
{
    BeginNode();
    m_emitter << Flow;
    m_emitter << BeginMap;
    m_state_stack.push(State::WaitingForKey);
}

void yaml_json_emitter::OnMapEnd()
{
    m_emitter << EndMap;
    assert(m_state_stack.top() == State::WaitingForKey);
    m_state_stack.pop();
}

void yaml_json_emitter::BeginNode()
{
    if (m_state_stack.empty()) return;

    switch (m_state_stack.top()) {
    case State::WaitingForKey:
        m_emitter << Key;
        m_state_stack.top() = State::WaitingForValue;
        break;
    case State::WaitingForValue:
        m_emitter << Value;
        m_state_stack.top() = State::WaitingForKey;
        break;
    default:
        break;
    }
}

} // namespace openperf::config::file
