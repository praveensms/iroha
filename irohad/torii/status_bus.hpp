/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORII_STATUS_BUS
#define TORII_STATUS_BUS

#include <rxcpp/rx.hpp>
#include "interfaces/transaction_responses/tx_response.hpp"

namespace iroha {
  namespace torii {
    /**
     * Interface of bus for transaction statuses
     */
    class StatusBus {
     public:
      virtual ~StatusBus() = default;

      using Objects =
          std::shared_ptr<shared_model::interface::TransactionResponse>;

      /**
       * Shares object among the bus users
       * @param object to share
       */
      virtual void publish(Objects) = 0;

      /**
       * @return observable over objects in bus
       */
      virtual rxcpp::observable<Objects> statuses() = 0;
    };
  }  // namespace torii
}  // namespace iroha

#endif  // TORII_STATUS_BUS
