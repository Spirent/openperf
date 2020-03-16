#include "Worker.hpp"

#include <zmq.h>
#include <iostream>

namespace openperf::memory::generator {

// Functions
// void thread_loop(void* context)
//{
//    std::cout << "Thread " << std::hex << std::this_thread::get_id()
//              << " started" << std::endl;
//    auto socket = std::unique_ptr<void, op_socket_deleter>(
//        op_socket_get_client(context, ZMQ_REP, endpoint_prefix));
//
//    bool running = false;
//    GeneratorConfig config;
//
//    for (;;) {
//        int recv = zmq_recv(
//            socket.get(), &config, sizeof(config), running ? ZMQ_NOBLOCK : 0);
//        if (recv < 0 && errno != EAGAIN) {
//            std::cerr << "Shutdown" << std::endl;
//            break;
//        }
//
//        std::cout << "Thread work" << std::endl;
//        if (running) {
//            // for (auto& task : _tasks) {
//            //    task.get().run();
//            //}
//        }
//    }
//
//    std::cout << "Thread " << std::hex << std::this_thread::get_id()
//              << " finished" << std::endl;
//}

// Class: Worker
Worker::Worker()
    : _paused(true)
    , _stopped(true)
    , _context(zmq_ctx_new())
{
    _zmqSocket = ZmqSocketPointer(
        op_socket_get_server(_context, ZMQ_PAIR, endpoint_prefix));
}

Worker::~Worker()
{
    if (_thread.joinable()) {
        _thread.detach();
        _stopped = true;
        _paused = false;
        update();
    }

    zmq_ctx_shutdown(_context);
}

void Worker::addTask(std::unique_ptr<TaskBase> task)
{
    _tasks.emplace_front(std::move(task));
}

void Worker::start()
{
    std::cout << "Start method worker: " << _stopped << std::endl;
    if (!_stopped) return;
    _stopped = false;
    std::cout << "Start worker" << std::endl;

    // if (_thread.joinable()) _thread.detach();
    _thread = std::thread([this]() { loop(); });
    update();
}

void Worker::stop()
{
    if (_stopped) return;
    _stopped = true;
    _thread.detach();
    update();
}

void Worker::pause()
{
    if (_paused) return;
    _paused = true;
    update();
}

void Worker::resume()
{
    if (!_paused) return;
    _paused = false;
    update();
}

void Worker::loop()
{
    std::cout << "Thread " << std::hex << std::this_thread::get_id()
              << " started" << std::endl;
    auto socket = std::unique_ptr<void, op_socket_deleter>(
        op_socket_get_client(_context, ZMQ_PAIR, endpoint_prefix));

    ZmqMessage msg{.running = false, .stopping = false};

    for (;;) {
        std::cout << "Thread bf r: " << msg.running << ", s: " << msg.stopping
                  << std::endl;
        int recv = zmq_recv(
            socket.get(), &msg, sizeof(msg), msg.running ? ZMQ_NOBLOCK : 0);
        if (recv < 0 && errno != EAGAIN) {
            std::cerr << "Shutdown" << std::endl;
            break;
        }
        std::cout << "Thread af r: " << msg.running << ", s: " << msg.stopping
                  << std::endl;

        if (msg.stopping) {
            std::cout << "Gracesfully shutdown" << std::endl;
            break;
        }

        std::cout << "Thread work" << std::endl;
        if (msg.running) {
            for (auto& task : _tasks) { task->run(); }
        }
    }

    std::cout << "Thread " << std::hex << std::this_thread::get_id()
              << " finished" << std::endl;
}

void Worker::update()
{
    ZmqMessage msg{.running = !_paused, .stopping = _stopped};

    zmq_send(_zmqSocket.get(), &msg, sizeof(msg), 0);
    std::cout << "Worker::update()" << std::endl;
}

} // namespace openperf::memory::generator
