#include <iostream>
#include <deque>
#include <csignal>
#include <mutex>
#include <thread>
#include <random>
#include <chrono>
#include "boost/unordered/unordered_flat_map.hpp"
#include "boost/container/stable_vector.hpp"

using operation_id = int;
using callback_t = std::function<void(operation_id)>;

struct wheel
{
  static void poll_clock_thread()
  {
    command_t command;

    if (!to_wheel_.empty()) {
      to_wheel_.front()();
      to_wheel_.pop_front();
    }
  }

  static void poll_local_thread()
  {
    if (from_wheel_ == nullptr) {
      return;
    }

    event_t event;

    if (!from_wheel_->empty()) {
      from_wheel_->front()();
      from_wheel_->pop_front();
    }
  }

  static operation_id get_id()
  {
    if (from_wheel_ == nullptr) {
      // initialize wheel -> thread queue
      from_wheel_ = &thread_queues_.emplace_back();
    }

    return ++id_;
  }

  static void schedule(operation_id op, callback_t callback)
  {
    operations_.try_emplace(op, callback);

    auto marshalled = [callback_queue = from_wheel_](operation_id op) {
      callback_queue->push_back([op]{
        if (auto it = operations_.find(op); it != std::end(operations_)) {
          it->second.callback(op);
          operations_.erase(it);
        }
      });
    };

    to_wheel_.push_back([=] { marshalled(op); } );
  }

  static void cancel(operation_id id)
  {
    operations_.erase(id);
  }


private:
  struct operation_t
  {
    callback_t callback;
  };

  using command_t = std::function<void()>;
  using event_t = std::function<void()>;
  using event_queue_t = std::deque<event_t>;

  inline static std::atomic<operation_id> id_{0};

  inline static std::deque<command_t> to_wheel_{};

  inline static thread_local event_queue_t* from_wheel_ = nullptr;
  inline static thread_local boost::unordered_flat_map<operation_id, operation_t> operations_{};

  inline static boost::container::stable_vector<event_queue_t> thread_queues_{};

};

struct timer
{
  void expires_after() {
    cancel();
    id = wheel::get_id();
    wheel::schedule(*id, [this](auto id) { elapsed(id); });
  }

  void cancel()
  {
    if (id) {
      wheel::cancel(*id);
      id.reset();
    }
  }

  void elapsed(operation_id) {}

  std::optional<operation_id> id;
};

struct user
{
  user()
  {
    for (int i = 0; i < 250; ++i) {
      timers.push_back(timer{});
    }
  }

  void tick() {
    auto index = timer_dist(gen);
    bool op_start = index % 2 == 0;

    if (op_start) {
      timers.at(index).expires_after();
    } else {
      timers.at(index).cancel();
    }
  }

  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::uniform_int_distribution<int> timer_dist{0, 249};
  std::vector<timer> timers;
};

std::atomic<bool> running_ = true;

void signal_handler(int /* signal */)
{
  running_ = false;
}

int main()
{
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  int last_id = 0;
  user u;

  while(running_) {
    u.tick();
    wheel::poll_clock_thread();
    wheel::poll_local_thread();
  }

  std::cout << "exit " << last_id << "\n";

  return EXIT_SUCCESS;
}
