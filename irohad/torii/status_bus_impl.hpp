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

namespace iroha {
  namespace torii {
    /**
     * StatusBus implementation that support syncronous and asynchronous way of
     * passing objects
     */
    class StatusBusImpl : public StatusBus {
     public:
      /**
       * @param is_sync defines wether status bus should be synchronous
       */
      StatusBusImpl(bool is_sync = false);
      ~StatusBusImpl();

      void publish(StatusBus::Objects) override;
      rxcpp::observable<StatusBus::Objects> statuses() override;

     protected:
      void update();

     private:
      const bool is_sync_;
      std::atomic<bool> is_active_;
      tbb::concurrent_queue<StatusBus::Objects> q_;
      rxcpp::subjects::subject<StatusBus::Objects> subject_;
      StatusBus::Objects obj_;
      std::thread worker_;
      std::mutex m_;
    };
  }  // namespace torii
}  // namespace iroha

#endif  // TORII_STATUS_BUS_IMPL
