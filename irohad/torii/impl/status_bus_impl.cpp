/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "torii/status_bus_impl.hpp"

namespace iroha {
  namespace torii {
    StatusBusImpl::StatusBusImpl()
        : worker_(rxcpp::observe_on_new_thread()), subject_(worker_) {}

    void StatusBusImpl::publish(StatusBus::Objects resp) {
      subject_.get_subscriber().on_next(resp);
    }

    rxcpp::observable<StatusBus::Objects> StatusBusImpl::statuses() {
      return subject_.get_observable();
    }
  }  // namespace torii
}  // namespace iroha
