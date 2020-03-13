#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include "swagger/v1/model/MemoryGenerator.h"
#include "modules/memory/worker.hpp"

namespace openperf::memory::generator {

using json = nlohmann::json;
namespace model = swagger::v1::model;

class generator
{
private:
    model::MemoryGenerator  _model;
    
    worker _worker;

public:
    // Constructors & Destructor
    generator(const model::MemoryGenerator&);
    //generator(const json&);
    ~generator() {};

    // Methods
    const model::MemoryGenerator& model() const;
    inline bool is_running() const { return _model.isRunning(); }
    
    inline void start() { set_running(true); }
    inline void stop() { set_running(false); }

    void update_config(const model::MemoryGeneratorConfig&);

private:
    void set_running(bool);
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_HPP_
