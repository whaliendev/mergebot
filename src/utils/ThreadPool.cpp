//
// Created by whalien on 09/02/23.
//

#include "mergebot/utils/ThreadPool.h"

#include <spdlog/spdlog.h>

pthread_mutex_t ThreadPool::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::notify = PTHREAD_COND_INITIALIZER;
std::vector<pthread_t> ThreadPool::threads;
std::vector<ThreadPoolTask> ThreadPool::queue;
int ThreadPool::thread_count = 0;
int ThreadPool::queue_size = 0;
int ThreadPool::head = 0;
int ThreadPool::tail = 0;
int ThreadPool::count = 0;
int ThreadPool::shutdown = 0;
int ThreadPool::started = 0;

int ThreadPool::threadpool_create(int _thread_count, int _queue_size) {
  bool err = false;
  if (_thread_count <= 0 || _thread_count > MAX_THREADS || _queue_size <= 0 ||
      _queue_size > MAX_QUEUE) {
    _thread_count = 4;
    _queue_size = 1024;
  }

  thread_count = 0;
  queue_size = _queue_size;
  head = tail = count = 0;
  shutdown = started = 0;

  threads.resize(_thread_count);
  queue.resize(_queue_size);

  /* Start worker threads */
  for (int i = 0; i < _thread_count; ++i) {
    if (pthread_create(&threads[i], nullptr, threadpool_thread,
                       reinterpret_cast<void *>(0)) != 0) {
      // threadpool_destroy(pool, 0);
      SPDLOG_ERROR("unable to create enough pthreads");
      return -1;
    }
    ++thread_count;
    ++started;
  }

  if (err) {
    return -1;
  }
  return 0;
}

int ThreadPool::threadpool_add(std::shared_ptr<void> args,
                               std::function<void(std::shared_ptr<void>)> fun) {
  int next, err = 0;
  if (pthread_mutex_lock(&lock) != 0) {
    SPDLOG_ERROR("fail to acquire lock to add tasks to thread pool");
    return THREADPOOL_LOCK_FAILURE;
  }
  do {
    next = (tail + 1) % queue_size;
    // 队列满
    // TODO(hwa): do we need to refactor to a blocking manner?
    if (count == queue_size) {
      SPDLOG_ERROR("task queue is full, fail to add tasks");
      err = THREADPOOL_QUEUE_FULL;
      break;
    }
    // 已关闭
    if (shutdown) {
      err = THREADPOOL_SHUTDOWN;
      SPDLOG_ERROR("thread pool accidentally closed, check the log");
      break;
    }
    queue[tail].fun = fun;
    queue[tail].args = args;
    tail = next;
    ++count;

    /* pthread_cond_broadcast */
    if (pthread_cond_signal(&notify) != 0) {
      err = THREADPOOL_LOCK_FAILURE;
      SPDLOG_ERROR("fail to notify all awaited threads");
      break;
    }
  } while (false);

  if (pthread_mutex_unlock(&lock) != 0) {
    SPDLOG_ERROR("fail to release task queue mutex lock");
    err = THREADPOOL_LOCK_FAILURE;
  }
  return err;
}

/// @brief destroy thread pool, release all the resources after program exits
/// \param shutdown_option gracefully or immediate
/// \return err code
int ThreadPool::threadpool_destroy(ShutDownOption shutdown_option) {
  SPDLOG_INFO("begin destroy thread pool");
  int i, err = 0;

  if (pthread_mutex_lock(&lock) != 0) {
    SPDLOG_ERROR("fail to acquire mutex lock to free resources");
    return THREADPOOL_LOCK_FAILURE;
  }
  do {
    if (shutdown) {
      err = THREADPOOL_SHUTDOWN;
      SPDLOG_ERROR("thread pool accidentally closed, check the log");
      break;
    }
    shutdown = shutdown_option;

    if ((pthread_cond_broadcast(&notify) != 0) ||
        (pthread_mutex_unlock(&lock) != 0)) {
      SPDLOG_ERROR(
          "fail to notify all the awaited threads or release mutex lock");
      err = THREADPOOL_LOCK_FAILURE;
      break;
    }

    for (i = 0; i < thread_count; ++i) {
      if (pthread_join(threads[i], nullptr) != 0) {
        SPDLOG_ERROR("join all threads failed while destroy thread pool");
        err = THREADPOOL_THREAD_FAILURE;
      }
    }
  } while (false);

  if (!err) {
    threadpool_free();
    SPDLOG_INFO("destroy thread pool successfully");
  }
  return err;
}

/// private func to unlock mutex and condition vars
int ThreadPool::threadpool_free() {
  if (started > 0) {
    return -1;
  }
  pthread_mutex_lock(&lock);
  pthread_mutex_destroy(&lock);
  pthread_cond_destroy(&notify);
  return 0;
}

/// @brief tasks to submit to thread pool.
///
/// we prefer to use `std::bind` to pass a bound task func
/// submit
/// \param args always use reinterpret_cast<void*>(0) to set args when
/// submitting to thread pool, as the args passed in don't matter.
/// \return
void *ThreadPool::threadpool_thread(void *args) {
  while (true) {
    ThreadPoolTask task;
    pthread_mutex_lock(&lock);
    while ((count == 0) && (!shutdown)) {
      pthread_cond_wait(&notify, &lock);
    }
    if ((shutdown == immediate_shutdown) ||
        ((shutdown == graceful_shutdown) && (count == 0))) {
      break;
    }
    task.fun = queue[head].fun;
    task.args = queue[head].args;
    queue[head].fun = nullptr;
    queue[head].args.reset();
    head = (head + 1) % queue_size;
    --count;
    pthread_mutex_unlock(&lock);
    (task.fun)(task.args);
  }
  --started;
  pthread_mutex_unlock(&lock);
  SPDLOG_DEBUG("This threadpool thread finishes!");
  pthread_exit(nullptr);
}
