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
        : is_active_(true), worker_([this] {
            StatusBus::Objects obj;
            while (this->is_active_) {
              while (not this->q_.empty() and this->q_.try_pop(obj)) {
                this->subject_.get_subscriber().on_next(obj);
              }
            }
          }) {}

    StatusBusImpl::~StatusBusImpl() {
      is_active_ = false;
      worker_.join();
    }

    void StatusBusImpl::publish(StatusBus::Objects resp) {
      q_.push(std::move(resp));
    }

    rxcpp::observable<StatusBus::Objects> StatusBusImpl::statuses() {
      return subject_.get_observable();
    }
  }  // namespace torii

}  // namespace iroha
