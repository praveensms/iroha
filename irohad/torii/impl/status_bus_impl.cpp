/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "torii/status_bus_impl.hpp"

namespace iroha {
  namespace torii {
    StatusBusImpl::StatusBusImpl()
        : is_active_(true), log_(logger::log("StatusBus")), worker_([this] {
            do {
              this->update();
            } while (this->is_active_);
          }) {}

    StatusBusImpl::~StatusBusImpl() {
      is_active_ = false;
      worker_.join();
    }

    void StatusBusImpl::publish(StatusBus::Objects resp) {
      log_->info("Publish {}", resp->toString());
      q_.push(std::move(resp));
    }

    void StatusBusImpl::update() {
      while (not q_.empty() and q_.try_pop(obj_)) {
        subject_.get_subscriber().on_next(obj_);
      }
    }

    rxcpp::observable<StatusBus::Objects> StatusBusImpl::statuses() {
      return subject_.get_observable().tap([this](StatusBus::Objects s) {
        this->log_->info("Statuses {}", s->toString());
      });
    }
  }  // namespace torii

}  // namespace iroha
