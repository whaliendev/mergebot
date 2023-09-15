//
// Created by whalien on 28/03/23.
//
#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <gtest/gtest.h>
#include <spdlog/async.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <iostream>

void InitLogger();

int main(int argc, char** argv) {
  InitLogger();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

void InitLogger() {
  try {
    // output to console for debugging purpose
    auto consoleSink =
        std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>(
            spdlog::color_mode::always);
#ifdef NDEBUG
    consoleSink->set_level(spdlog::level::info);
#else
    consoleSink->set_level(spdlog::level::debug);
#endif
    consoleSink->set_pattern("[%Y-%m-%d %T] [%t] %^[%l]%$ %@: %v");

    spdlog::sinks_init_list sinkList = {
        consoleSink,
    };
    spdlog::init_thread_pool(4096, 1);
    auto defaultLogger = std::make_shared<spdlog::async_logger>(
        "async_logger", sinkList, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);

    // global logger level should be lower than two sinks
#ifndef NDEBUG
    defaultLogger->set_level(spdlog::level::debug);
#else
    defaultLogger->set_level(spdlog::level::info);
#endif

    spdlog::register_logger(defaultLogger);
    spdlog::set_default_logger(defaultLogger);
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "spdlog initialization failed: " << ex.what() << '\n';
    spdlog::shutdown();
    exit(1);
  }
}
