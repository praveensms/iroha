/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "torii/status_bus_impl.hpp"

namespace iroha {
  namespace torii {
    StatusBusImpl::StatusBusImpl(bool is_sync)
        : is_sync_(is_sync), is_active_(true) {
      if (not is_sync_) {
        worker_ = std::thread([this] {
          do {
            this->update();
          } while (this->is_active_);
        });
      }
    }

    StatusBusImpl::~StatusBusImpl() {
      is_active_ = false;
      if (not is_sync_) {
        worker_.join();
      }
    }

    void StatusBusImpl::publish(StatusBus::Objects resp) {
      q_.push(std::move(resp));
      if (is_sync_) {
        std::lock_guard<std::mutex> lock(m_);
        update();
      }
    }

    void StatusBusImpl::update() {
      while (not q_.empty() and q_.try_pop(obj_)) {
        subject_.get_subscriber().on_next(obj_);
      }
    }

    rxcpp::observable<StatusBus::Objects> StatusBusImpl::statuses() {
      return subject_.get_observable();
    }
  }  // namespace torii

}  // namespace iroha
