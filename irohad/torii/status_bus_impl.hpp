/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORII_STATUS_BUS_IMPL
#define TORII_STATUS_BUS_IMPL

#include "torii/status_bus.hpp"

#include <atomic>
#include <mutex>
#include <thread>

#include <tbb/concurrent_queue.h>
#include "logger/logger.hpp"

namespace iroha {
  namespace torii {
    /**
     * StatusBus implementation
     */
    class StatusBusImpl : public StatusBus {
     public:
      StatusBusImpl();
      ~StatusBusImpl();

      void publish(StatusBus::Objects) override;
      rxcpp::observable<StatusBus::Objects> statuses() override;

     private:
      void update();

      std::atomic<bool> is_active_;
      tbb::concurrent_queue<StatusBus::Objects> q_;
      rxcpp::subjects::subject<StatusBus::Objects> subject_;
      StatusBus::Objects obj_;
      logger::Logger log_;
      std::thread worker_;
    };
  }  // namespace torii
}  // namespace iroha

#endif  // TORII_STATUS_BUS_IMPL
