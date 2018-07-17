/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "torii/status_bus_impl.hpp"

namespace iroha {
  namespace torii {
    StatusBusImpl::StatusBusImpl()
        : coordinatior(rxcpp::observe_on_new_thread()
                           .create_coordinator()
                           .get_scheduler()) {}

    void StatusBusImpl::publish(StatusBus::Objects resp) {
      std::lock_guard<std::mutex> lock(m_);
      statuses_.get_subscriber().on_next(std::move(resp));
    }

    rxcpp::observable<StatusBus::Objects> StatusBusImpl::statuses() {
      return statuses_.get_observable().subscribe_on(coordinatior);
    }
  }  // namespace torii

}  // namespace iroha
