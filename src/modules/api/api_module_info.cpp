#include <regex>

#include "json.hpp"

#include "api/api_route_handler.hpp"
#include "swagger/v1/model/Module.h"

#include "core/op_modules.h"

namespace openperf {
namespace api {
namespace modules {

using namespace Pistache;
using json = nlohmann::json;
using namespace swagger::v1::model;

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Rest::Router& router);
    void list_modules(const Rest::Request& request,
                      Http::ResponseWriter response);
    void get_module(const Rest::Request& request,
                    Http::ResponseWriter response);
};

handler::handler(void* context __attribute__((unused)), Rest::Router& router)
{
    Rest::Routes::Get(router, "/modules",
                      Rest::Routes::bind(&handler::list_modules, this));

    Rest::Routes::Get(router, "/modules/:id",
                      Rest::Routes::bind(&handler::get_module, this));
}

static auto to_string(enum op_module_linkage_type type)
{
    switch (type) {
    case DYNAMIC:
        return "dynamic";
    case STATIC:
        return "static";
    default:
        return "fixme";
    }
}

static auto create_json_module_info(const struct op_module_info* module_info)
{
    auto out_mod = std::make_shared<Module>();
    out_mod->setId(module_info->id);
    out_mod->setDescription(module_info->description);
    out_mod->setLinkage(to_string(module_info->linkage));
    if (module_info->path) out_mod->setPath(module_info->path);

    auto out_version = std::make_shared<ModuleVersion>();
    out_version->setVersion(module_info->version.version);
    if (module_info->version.build_number)
        out_version->setBuildNumber(module_info->version.build_number);
    if (module_info->version.build_date)
        out_version->setBuildDate(module_info->version.build_date);
    if (module_info->version.source_commit)
        out_version->setSourceCommit(module_info->version.source_commit);

    out_mod->setVersion(out_version);

    return out_mod;
}

void handler::list_modules(const Rest::Request& request __attribute__((unused)),
                           Http::ResponseWriter response)
{
    auto module_count = op_modules_get_loaded_count();
    std::vector<const struct op_module_info*> modules(module_count);
    auto returned_module_count =
        op_modules_get_info_list(modules.data(), module_count);

    if (returned_module_count < 0) {
        response.send(Http::Code::Internal_Server_Error);
        return;
    }

    if (returned_module_count == 0) {
        response.send(Http::Code::Ok);
        return;
    }

    json jmodules = json::array();
    for (auto module : modules) {
        assert(module);

        auto out_mod = create_json_module_info(module);
        jmodules.emplace_back(out_mod->toJson());
    }

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Ok, jmodules.dump());
}

void handler::get_module(const Rest::Request& request,
                         Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();

    const std::regex id_regex(OP_MODULE_ID_REGEX);
    if (!std::regex_match(id, id_regex)) {
        response.send(Http::Code::Bad_Request,
                      "Invalid characters detected in request. Only a-z, 0-9, "
                      "-, and . are allowed.");
        return;
    }

    auto module_info = op_modules_get_info_by_id(id.c_str());
    if (!module_info) {
        response.send(Http::Code::Not_Found);
        return;
    }

    auto out_mod = create_json_module_info(module_info);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Ok, out_mod->toJson().dump());
}

} // namespace modules
} // namespace api
} // namespace openperf
