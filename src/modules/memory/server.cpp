#include "memory/server.hpp"

#include "memory/api.hpp"
#include "memory/info.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"
#include "memory/generator_config.hpp"
#include "memory/generator_config_converters.hpp"

namespace openperf::memory::api {

namespace model = ::swagger::v1::model;

// ENUM to STRING converters
std::string to_string(request_type type)
{
    const static std::unordered_map<request_type, std::string> request_types = {
        {request_type::LIST_GENERATORS, "LIST_GENERATORS"},
        {request_type::CREATE_GENERATOR, "CREATE_GENERATOR"},
        {request_type::GET_GENERATOR, "GET_GENERATOR"},
        {request_type::DELETE_GENERATOR, "DELETE_GENERATOR"},
        {request_type::START_GENERATOR, "START_GENERATOR"},
        {request_type::STOP_GENERATOR, "STOP_GENERATOR"},
        {request_type::BULK_START_GENERATORS, "BULK_START_GENERATORS"},
        {request_type::BULK_STOP_GENERATORS, "BULK_STOP_GENERATORS"},
        {request_type::LIST_RESULTS, "LIST_RESULTS"},
        {request_type::GET_RESULT, "GET_RESULT"},
        {request_type::DELETE_RESULT, "DELETE_RESULT"},
        {request_type::GET_INFO, "GET_INFO"},
        {request_type::NONE, "UNKNOWN"}};

    return request_types.find(type) == request_types.end()
               ? request_types.at(request_type::NONE)
               : request_types.at(type);
}

std::string to_string(reply_code code)
{
    const static std::unordered_map<reply_code, std::string> reply_codes = {
        {reply_code::OK, "OK"},
        {reply_code::NO_GENERATOR, "NO_GENERATOR"},
        {reply_code::NO_RESULT, "NO_RESULT"},
        {reply_code::BAD_INPUT, "BAD_INPUT"},
        {reply_code::ERROR, "ERROR"},
        {reply_code::NONE, "UNKNOWN"}};

    return reply_codes.find(code) == reply_codes.end()
               ? reply_codes.at(reply_code::NONE)
               : reply_codes.at(code);
}

server::server(void* context, openperf::core::event_loop& loop)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint))
    , generator_stack(std::make_unique<memory::generator_collection>())
{
    // Setup event loop
    struct op_event_callbacks callbacks = {
        .on_read = [](const op_event_data* data, void* arg) -> int {
            auto s = reinterpret_cast<server*>(arg);
            return s->handle_rpc_request(data);
        }};
    loop.add(m_socket.get(), &callbacks, this);
}

int server::handle_rpc_request(const op_event_data* data)
{
    int recv_or_err = 0;
    int send_or_err = 0;
    zmq_msg_t request_msg;

    if (zmq_msg_init(&request_msg) == -1) {
        throw std::runtime_error(zmq_strerror(errno));
    }

    while (
        (recv_or_err = zmq_msg_recv(&request_msg, data->socket, ZMQ_DONTWAIT))
        > 0) {
        std::vector<uint8_t> request_buffer(
            static_cast<uint8_t*>(zmq_msg_data(&request_msg)),
            static_cast<uint8_t*>(zmq_msg_data(&request_msg))
                + zmq_msg_size(&request_msg));

        json reply = handle_json_request(json::from_cbor(request_buffer));

        std::vector<uint8_t> reply_buffer = json::to_cbor(reply);
        if ((send_or_err = zmq_send(
                 data->socket, reply_buffer.data(), reply_buffer.size(), 0))
            != static_cast<int>(reply_buffer.size())) {
            OP_LOG(OP_LOG_ERROR,
                   "Request reply failed: %s\n",
                   zmq_strerror(errno));
        } else {
            OP_LOG(OP_LOG_TRACE,
                   "Sent %s reply\n",
                   //"Sent %s reply to %s request\n",
                   to_string(reply["code"].get<reply_code>()).c_str());
            // to_string(type).c_str());
        }
    }

    zmq_msg_close(&request_msg);

    return (((recv_or_err < 0 || send_or_err < 0) && errno == ETERM) ? -1 : 0);
}

json server::handle_json_request(const json& request)
{
    request_type type = request.find("type") == request.end()
                            ? request_type::NONE
                            : request["type"].get<request_type>();

    switch (type) {
    case request_type::GET_GENERATOR:
    case request_type::CREATE_GENERATOR:
    case request_type::DELETE_GENERATOR:
    case request_type::START_GENERATOR:
    case request_type::STOP_GENERATOR:
        OP_LOG(OP_LOG_TRACE,
               "Received %s request for generator %s\n",
               to_string(type).c_str(),
               request["id"].get<std::string>().c_str());
        break;
    case request_type::GET_RESULT:
    case request_type::DELETE_RESULT:
        OP_LOG(OP_LOG_TRACE,
               "Received %s request for result %s\n",
               to_string(type).c_str(),
               request["id"].get<std::string>().c_str());
        break;
    default:
        OP_LOG(OP_LOG_TRACE, "Received %s request\n", to_string(type).c_str());
    }

    switch (type) {
    case request_type::LIST_GENERATORS:
        return list_generators();
    case request_type::GET_GENERATOR:
        return get_generator(request);
    case request_type::CREATE_GENERATOR:
        return create_generator(request);
    case request_type::DELETE_GENERATOR:
        return delete_generator(request);
    case request_type::START_GENERATOR:
        return start_generator(request);
    case request_type::STOP_GENERATOR:
        return stop_generator(request);
    case request_type::BULK_START_GENERATORS:
        return bulk_start_generators(request);
    case request_type::BULK_STOP_GENERATORS:
        return bulk_stop_generators(request);
    case request_type::LIST_RESULTS:
        return list_results();
    case request_type::GET_RESULT:
        return get_result(request);
    case request_type::DELETE_RESULT:
        return delete_result(request);
    case request_type::GET_INFO:
        return get_info();
    default:
        return json{
            {"code", reply_code::ERROR},
            {"error",
             {{"code", ENOSYS},
              {"message",
               "Request type not implemented in packetio interface server"}}}};
    }
}

json server::list_generators()
{
    json jdata = json::array();

    for (const auto& kv_pair : generator_stack->list()) {
        model::MemoryGenerator model;
        model.setId(kv_pair.first);
        model.setRunning(kv_pair.second.is_running());
        model.setConfig(std::make_shared<model::MemoryGeneratorConfig>(
            config_to_swagger_model(kv_pair.second)));

        jdata.emplace_back(model.toJson());
    }

    return json{{"code", reply_code::OK}, {"data", jdata.dump()}};
}

json server::create_generator(const json& request)
{
    try {
        auto json_object = json::parse(request["data"].get<std::string>());

        model::MemoryGenerator generator_model;
        generator_model.fromJson(json_object);

        generator_config config = swagger_model_to_config([&json_object]() {
            model::MemoryGeneratorConfig config_model;
            config_model.fromJson(json_object["config"]);
            return config_model;
        }());

        try {
            auto result =
                generator_stack->create(generator_model.getId(), config);

            generator_model.setRunning(result.is_running());
            generator_model.setConfig(
                std::make_shared<model::MemoryGeneratorConfig>(
                    config_to_swagger_model(result)));

            return json{{"code", reply_code::OK},
                        {"data", generator_model.toJson().dump()}};
        } catch (std::invalid_argument e) {
            throw std::runtime_error(e.what());
        }
    } catch (const std::runtime_error& e) {
        return json{{"code", reply_code::BAD_INPUT},
                    {"error", json_error(EINVAL, e.what())}};
    } catch (const json::exception& e) {
        return json{{"code", reply_code::BAD_INPUT},
                    {"error", json_error(e.id, e.what())}};
    }
}

json server::get_generator(const json& request)
{
    auto id = request["id"].get<std::string>();

    if (generator_stack->contains(id)) {
        const auto& g = generator_stack->get(id);

        model::MemoryGenerator model;
        model.setId(id);
        model.setRunning(g.is_running());
        model.setConfig(std::make_shared<model::MemoryGeneratorConfig>(
            config_to_swagger_model(g)));

        return json{{"code", reply_code::OK}, {"data", model.toJson().dump()}};
    }

    return json{"code", reply_code::NO_GENERATOR};
}

json server::delete_generator(const json& request)
{
    auto id = request["id"];
    if (generator_stack->contains(id)) {
        generator_stack->erase(id);
        return json{"code", reply_code::OK};
    }

    return json{"code", reply_code::NO_GENERATOR};
}

json server::start_generator(const json& request)
{
    auto id = request["id"];
    if (generator_stack->contains(id)) {
        generator_stack->start(id);
        return json{"code", reply_code::OK};
    }

    return json{"code", reply_code::NO_GENERATOR};
}

json server::stop_generator(const json& request)
{
    auto id = request["id"];
    if (generator_stack->contains(id)) {
        generator_stack->stop(id);
        return json{"code", reply_code::OK};
    }

    return json{"code", reply_code::NO_GENERATOR};
}

json server::bulk_start_generators(const json& request)
{
    auto ids = request["ids"];
    auto successed = json::array();
    auto failed = json::array();

    for (auto& id : ids) {
        generator_stack->start(id);
        successed.emplace_back(id);
    }

    return json{"code", reply_code::NO_GENERATOR};
}

json server::bulk_stop_generators(const json& request)
{
    auto ids = request["ids"];
    auto successed = json::array();
    auto failed = json::array();

    for (auto& id : ids) {
        generator_stack->stop(id);
        successed.emplace_back(id);
    }

    return json{"code", reply_code::NO_GENERATOR};
}

json server::list_results() { return json{}; }

json server::get_result(const json& /*request*/)
{
    return json{"code", reply_code::NO_RESULT};
}

json server::delete_result(const json& /*request*/)
{
    return json{"code", reply_code::NO_RESULT};
}

json server::get_info()
{
    try {
        return json{{"code", reply_code::OK},
                    {"data", memory_info::get().toJson().dump()}};
    } catch (std::exception e) {
        return json{{"code", reply_code::ERROR},
                    {"error", json_error(-1, e.what())}};
    }
}

} // namespace openperf::memory::api
