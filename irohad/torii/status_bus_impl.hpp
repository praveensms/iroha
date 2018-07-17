/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORII_STATUS_BUS_IMPL
#define TORII_STATUS_BUS_IMPL

#include "torii/status_bus.hpp"

#include <mutex>

namespace iroha {
  namespace torii {
    class StatusBusImpl : public StatusBus {
     public:
      StatusBusImpl();

      void publish(StatusBus::Objects) override;
      rxcpp::observable<StatusBus::Objects> statuses() override;

      rxcpp::subjects::subject<StatusBus::Objects> statuses_;
      rxcpp::observe_on_one_worker coordinatior;

      std::mutex m_;
    };
  }  // namespace torii
}  // namespace iroha

#endif  // TORII_STATUS_BUS_IMPL
