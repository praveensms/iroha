/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORII_STATUS_BUS_IMPL
#define TORII_STATUS_BUS_IMPL

#include "torii/status_bus.hpp"

#include <atomic>
#include <thread>

#include <tbb/concurrent_queue.h>

namespace iroha {
  namespace torii {
    class StatusBusImpl : public StatusBus {
     public:
      StatusBusImpl();
      ~StatusBusImpl();

      void publish(StatusBus::Objects) override;
      rxcpp::observable<StatusBus::Objects> statuses() override;

      rxcpp::subjects::subject<StatusBus::Objects> statuses_;

      std::atomic<bool> is_active_;
      tbb::concurrent_queue<StatusBus::Objects> q_;
      rxcpp::subjects::subject<StatusBus::Objects> subject_;
      std::thread worker_;
    };
  }  // namespace torii
}  // namespace iroha

#endif  // TORII_STATUS_BUS_IMPL
