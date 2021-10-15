/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _BF_RT_UTILS_HPP
#define _BF_RT_UTILS_HPP

#include <queue>
#include <mutex>
#include <future>
#include <thread>
#include <condition_variable>
#include <functional>
#include <cstring>

#include <osdep/p4_sde_osdep.h>

#define LOG_CRIT(...) LOG_COMMON(BF_LOG_CRIT, __VA_ARGS__)
#define LOG_ERROR(...) LOG_COMMON(BF_LOG_ERR, __VA_ARGS__)
#define LOG_WARN(...) LOG_COMMON(BF_LOG_WARN, __VA_ARGS__)
#define LOG_TRACE(...) LOG_COMMON(BF_LOG_INFO, __VA_ARGS__)
#define LOG_DBG(...) LOG_COMMON(BF_LOG_DBG, __VA_ARGS__)

#ifdef ENABLE_BF_RT_PRINT
#include <cstdio>
#define LOG_COMMON(LOG_LEVEL, ...) printf("\n" #LOG_LEVEL " : " __VA_ARGS__)
#else
#define LOG_COMMON(LOG_LEVEL, ...)                                \
  do {                                                            \
    if (bf_sys_log_is_log_enabled(BF_MOD_BFRT, LOG_LEVEL) == 1) { \
      bf_sys_log_and_trace(BF_MOD_BFRT, LOG_LEVEL, __VA_ARGS__);  \
    }                                                             \
  } while (0);
#endif

#define BF_RT_ASSERT bf_sys_assert
#define BF_RT_DBGCHK bf_sys_dbgchk

namespace bfrt {

// This is a class to initialize a generalized thread pool with a specific
// number of worker threads given at time of creation of the thread pool.
// The thread pool uses a thread safe queue to manage the producer-consumer
// pipeline. This thread safe queue is just a wrapper on std::queue<T>.
// The queue is templatized over std::function<void()> and thus holds function
// objects. It is the job of the submitTask function to package the function F
// and the variadic arguments that are passed to it by the user into a
// std::function<void()> function object and enqueue it in the queue.
// Any producer can first create the thread pool and then use the submitTask
// function to submit tasks which will be enqueued in the queue. The worker
// thread/threads are waiting for any tasks to appear in the queue and pick
// them up as soon as the tasks are enqueued. During the destruction of the
// thread pool, the "shutdown_" flag is set which indicates to the worker
// threads that they need to stop processing the tasks in the queue and return.
// These worker threads are then subsequently joined.
// The implementation of this thread pool is inspired from the following.
// https://github.com/mtrebi/thread-pool
class BfRtThreadPool {
 private:
  // Wrapper class over std::queue to make it thread safe
  template <typename T>
  class ThreadSafeQueue {
   public:
    ThreadSafeQueue(){};

    void enqueue(T &t) {
      std::lock_guard<std::mutex> lg(mtx_);
      queue_.push(t);
    }

    bool dequeue(T *t) {
      std::lock_guard<std::mutex> lg(mtx_);
      if (queue_.empty()) {
        return false;
      }
      *t = queue_.front();
      queue_.pop();
      return true;
    }

    bool empty() const {
      std::lock_guard<std::mutex> lg(mtx_);
      return queue_.empty();
    }

    size_t size() const {
      std::lock_guard<std::mutex> lg(mtx_);
      return queue_.size();
    }

   private:
    std::queue<T> queue_;
    // Lock to protect against concurrent accesses to std::queue
    mutable std::mutex mtx_;
  };  // ThreadSafeQueue

  // Functor which will be executed by each worker thread
  class WorkerThread {
   public:
    WorkerThread(const int &id, BfRtThreadPool *thread_pool)
        : id_(id), thread_pool_(thread_pool) {}

    void operator()() {
      // continue processing tasks from the queue until the thread pool is
      // shutdown
      while (!thread_pool_->shutdown_) {
        bool is_dequeued = false;
        std::function<void()> fn;
        {
          std::unique_lock<std::mutex> lock(thread_pool_->mtx_);
          if (thread_pool_->queue_.empty()) {
            // Wait until there is work to be performed
            thread_pool_->cond_var_.wait(lock);
          }
        }
        // Get a task from the queue
        is_dequeued = thread_pool_->queue_.dequeue(&fn);
        if (is_dequeued) {
          // Perform the task
          fn();
        }
      }
    }

   private:
    const int id_{0};
    BfRtThreadPool *thread_pool_{nullptr};
  };  // WorkerThread

  std::vector<std::thread> threads_;  // Vector to keep track of threads
  bool shutdown_{false};              // Flag to shutdown the pool
  ThreadSafeQueue<std::function<void()>> queue_;

  // We use condition variable so that the worker threads can be signalled
  // whenever there are tasks to be performed in the queue and thus they
  // don't need to spin on the queue waiting for tasks to show up
  std::mutex mtx_;
  std::condition_variable cond_var_;

 public:
  BfRtThreadPool(const size_t &num_threads = 1)
      : threads_(std::vector<std::thread>(num_threads)) {
    for (size_t i = 0; i < threads_.size(); i++) {
      threads_[i] = std::thread(WorkerThread(i, this));
    }
  }
  ~BfRtThreadPool() {
    // Stop processing any more tasks
    shutdown_ = true;
    // Wake up all threads so that break from their respective while loops
    // and return
    cond_var_.notify_all();
    for (size_t i = 0; i < threads_.size(); i++) {
      if (threads_[i].joinable()) {
        // Wait for the thread to finish
        threads_[i].join();
      }
    }
  }

  size_t getQueueSize() { return queue_.size(); }

  // This function is responsible for packaging the function 'F' and the
  // variadic arguments 'args' that are passed to it into a generic
  // function object 'std::function<void()>' and enqueue it in the queue.
  // These packaged function objects will subsquently be processed by the
  // worker threads
  template <typename F, typename... Args>
  auto submitTask(F &&f, Args &&... args) -> std::future<decltype(f(args...))> {
    // Construct a function object
    std::function<decltype(f(args...))()> fn_obj =
        std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    // Make a packaged task. We need a task object so that we retrieve the
    // result once the worker thread has finished execution
    auto task_ptr =
        std::make_shared<std::packaged_task<decltype(f(args...))()>>(fn_obj);

    // Construct a generic void function
    std::function<void()> fn_wrapper = [task_ptr]() { (*task_ptr)(); };

    // Enqueue the generic void function
    queue_.enqueue(fn_wrapper);

    // Wake up any one thread waiting
    cond_var_.notify_one();

    // Return the future for the packaged_task shared state
    return task_ptr->get_future();
  }

  // Delete the copy constructor and the assignment operator
  BfRtThreadPool(const BfRtThreadPool &) = delete;
  BfRtThreadPool(BfRtThreadPool &&) = delete;
  BfRtThreadPool &operator=(const BfRtThreadPool &) = delete;
  BfRtThreadPool &operator=(BfRtThreadPool &&) = delete;
};  // BfRtThreadPool

class BfRtEndiannessHandler {
 public:
  BfRtEndiannessHandler() = delete;

  static void toHostOrder(const size_t &size,
                          const uint8_t *value_ptr,
                          uint64_t *out_data) {
    uint64_t local_value = 0;
    uint8_t *dst = reinterpret_cast<uint8_t *>(&local_value);
    if (size > 8) {
      LOG_ERROR(
          "ERROR: %s:%d Trying to convert a network order byte stream of more "
          "than 8 bytes (%zu) into 64 bit data",
          __func__,
          __LINE__,
          size);
      BF_RT_DBGCHK(0);
      return;
    }
    local_value = 0;
    // Before memcpy, move the dst pointer offset deep enough
    dst += (8 - size);
    std::memcpy(dst, value_ptr, size);
    local_value = be64toh(local_value);

    *out_data = local_value;
  }

  static void toNetworkOrder(const size_t &size,
                             const uint64_t &in_data,
                             uint8_t *value_ptr) {
    if (size > 8) {
      LOG_ERROR(
          "ERROR: %s:%d Trying to convert a 64 bit data into a network order "
          "byte stream of more than 8 bytes (%zu)",
          __func__,
          __LINE__,
          size);
      BF_RT_DBGCHK(0);
      return;
    }
    auto local_val = in_data;
    local_val = htobe64(local_val);
    uint8_t *src = reinterpret_cast<uint8_t *>(&local_val);
    // Before memcpy, move the src pointer offset deep enough
    src += (8 - size);
    std::memcpy(value_ptr, src, size);
  }
};

}  // namespace bfrt

#endif  // _BF_RT_UTILS_HPP
