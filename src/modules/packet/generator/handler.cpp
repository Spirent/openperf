#include "api/api_route_handler.hpp"

namespace openperf::packet::generator::api {

using namespace Pistache;

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    /* Generator operations */
    void list_generators(const request_type& request, response_type response);
    void create_generators(const request_type& request, response_type response);
    void delete_generators(const request_type& request, response_type response);
    void get_generator(const request_type& request, response_type response);
    void delete_generator(const request_type& request, response_type response);
    void start_generator(const request_type& request, response_type response);
    void stop_generator(const request_type& request, response_type response);

    /* Bulk generator operations */
    void bulk_create_generators(const request_type& request,
                                response_type response);
    void bulk_delete_generators(const request_type& request,
                                response_type response);
    void bulk_start_generators(const request_type& request,
                               response_type response);
    void bulk_stop_generators(const request_type& request,
                              response_type response);

    /* Replace a running generator with an inactive generator */
    void toggle_generators(const request_type& request, response_type response);

    /* Generator result operations */
    void list_generator_results(const request_type& request,
                                response_type response);
    void delete_generator_results(const request_type& request,
                                  response_type response);
    void get_generator_result(const request_type& request,
                              response_type response);
    void delete_generator_result(const request_type& request,
                                 response_type response);

    /* Tx flow operations */
    void list_tx_flows(const request_type& request, response_type response);
    void get_tx_flow(const request_type& request, response_type response);
};

handler::handler(void*, Pistache::Rest::Router& router)
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/packet/generators", bind(&handler::list_generators, this));
    Post(router, "/packet/generators", bind(&handler::create_generators, this));
    Delete(
        router, "/packet/generators", bind(&handler::delete_generators, this));

    Get(router, "/packet/generators/:id", bind(&handler::get_generator, this));
    Delete(router,
           "/packet/generators/:id",
           bind(&handler::delete_generator, this));

    Post(router,
         "/packet/generators/:id/start",
         bind(&handler::start_generator, this));
    Post(router,
         "/packet/generators/:id/stop",
         bind(&handler::stop_generator, this));

    Post(router,
         "/packet/generators/x/bulk-create",
         bind(&handler::bulk_create_generators, this));
    Post(router,
         "/packet/generators/x/bulk-delete",
         bind(&handler::bulk_delete_generators, this));
    Post(router,
         "/packet/generators/x/bulk-start",
         bind(&handler::bulk_start_generators, this));
    Post(router,
         "/packet/generators/x/bulk-stop",
         bind(&handler::bulk_stop_generators, this));

    Post(router,
         "/packet/generators/x/toggle",
         bind(&handler::toggle_generators, this));

    Get(router,
        "/packet/generator-results",
        bind(&handler::list_generator_results, this));
    Delete(router,
           "/packet/generator-results",
           bind(&handler::delete_generator_results, this));

    Get(router,
        "/packet/generator-results/:id",
        bind(&handler::get_generator_result, this));
    Delete(router,
           "/packet/generator-results/:id",
           bind(&handler::delete_generator_result, this));

    Get(router, "/packet/tx-flows", bind(&handler::list_tx_flows, this));
    Get(router, "/packet/tx-flows/:id", bind(&handler::get_tx_flow, this));
}

void handler::list_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::create_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::delete_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::get_generator(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::delete_generator(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::start_generator(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::stop_generator(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_create_generators(const request_type&,
                                     response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_delete_generators(const request_type&,
                                     response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_start_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_stop_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::toggle_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::list_generator_results(const request_type&,
                                     response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::delete_generator_results(const request_type&,
                                       response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::get_generator_result(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::delete_generator_result(const request_type&,
                                      response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::list_tx_flows(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::get_tx_flow(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

} // namespace openperf::packet::generator::api
