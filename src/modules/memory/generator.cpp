#include "generator.hpp"
#include "swagger/v1/model/MemoryGeneratorConfig.h"

namespace openperf::memory::generator {

// Constructors & Destructor
generator::generator(const model::MemoryGenerator& generator_model)
{
    _model = generator_model;
    if (_model.isRunning())
        _worker.start();
}

// Methods : public
const model::MemoryGenerator& generator::model() const
{
    return _model;
}

// Methods : private
void generator::set_running(bool running)
{
    _model.setRunning(running);
}

void generator::update_config(const model::MemoryGeneratorConfig& config)
{
    model::MemoryGeneratorConfig own_config(config);
    _model.setConfig(std::make_shared<model::MemoryGeneratorConfig>(own_config));
    //TODO: delivery to worker
}

} // namespace openperf::memory::generator
