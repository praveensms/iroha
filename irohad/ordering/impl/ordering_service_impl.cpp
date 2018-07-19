/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ordering/impl/ordering_service_impl.hpp"

#include <algorithm>
#include <iterator>

#include <boost/range/adaptor/indirected.hpp>

#include "ametsuchi/ordering_service_persistent_state.hpp"
#include "ametsuchi/peer_query.hpp"
#include "datetime/time.hpp"
#include "network/ordering_service_transport.hpp"

namespace iroha {
  namespace ordering {
    OrderingServiceImpl::OrderingServiceImpl(
        std::shared_ptr<ametsuchi::PeerQuery> wsv,
        size_t max_size,
        rxcpp::observable<TimeoutType> proposal_timeout,
        std::shared_ptr<network::OrderingServiceTransport> transport,
        std::shared_ptr<ametsuchi::OrderingServicePersistentState>
            persistent_state,
        std::shared_ptr<shared_model::interface::ProposalFactory> factory,
        bool is_async)
        : wsv_(wsv),
          max_size_(max_size),
          transport_(transport),
          persistent_state_(persistent_state),
          factory_(factory),
          log_(logger::log("OrderingServiceImpl")) {
      // restore state of ordering service from persistent storage
      proposal_height_ = persistent_state_->loadProposalHeight().value();

      rxcpp::observable<ProposalEvent> timer =
          proposal_timeout.map([](auto) { return ProposalEvent::kTimerEvent; });

      auto subscribe = [&](auto merge_strategy) {
        handle_ = merge_strategy(rxcpp::observable<>::from(
                                     timer, transactions_.get_observable()))
                      .subscribe([this](auto &&v) {
                        auto check_queue = [&] {
                          switch (v) {
                            case ProposalEvent::kTimerEvent:
                              return not queue_.empty();
                            case ProposalEvent::kTransactionEvent:
                              return queue_.unsafe_size() >= max_size_;
                            default:
                              BOOST_ASSERT_MSG(false, "Unknown value");
                          }
                        };
                        if (check_queue()) {
                          this->generateProposal();
                        }
                      });
      };

      if (is_async) {
        subscribe([](auto observable) {
          return observable.merge(rxcpp::synchronize_new_thread());
        });
      } else {
        subscribe([](auto observable) { return observable.merge(); });
      }
    }

    void OrderingServiceImpl::onTransaction(
        std::shared_ptr<shared_model::interface::Transaction> transaction) {
      queue_.push(transaction);
      log_->info("Queue size is {}", queue_.unsafe_size());

      // on_next calls should not be concurrent
      std::lock_guard<std::mutex> lk(mutex_);
      transactions_.get_subscriber().on_next(ProposalEvent::kTransactionEvent);
    }

    void OrderingServiceImpl::generateProposal() {
      log_->info("Start proposal generation");
      std::vector<std::shared_ptr<shared_model::interface::Transaction>> txs;

      // extract transactions until the queue is empty
      // or maximum limit of transactions in proposal is achieved
      while (txs.size() < max_size_) {
        std::shared_ptr<shared_model::interface::Transaction> tx;
        if (not queue_.try_pop(tx)) {
          break;
        }
        txs.push_back(std::move(tx));
      }
      auto tx_range = txs | boost::adaptors::indirected;
      auto proposal = factory_->createProposal(
          proposal_height_++, iroha::time::now(), tx_range);

      proposal.match(
          [this](expected::Value<
                 std::unique_ptr<shared_model::interface::Proposal>> &v) {
            // Save proposal height to the persistent storage.
            // In case of restart it reloads state.
            if (persistent_state_->saveProposalHeight(proposal_height_)) {
              publishProposal(std::move(v.value));
            } else {
              // TODO(@l4l) 23/03/18: publish proposal independent of psql
              // status IR-1162
              log_->warn(
                  "Proposal height cannot be saved. Skipping proposal publish");
            }
          },
          [this](expected::Error<std::string> &e) {
            log_->warn("Failed to initialize proposal: {}", e.error);
          });
    }

    void OrderingServiceImpl::publishProposal(
        std::unique_ptr<shared_model::interface::Proposal> proposal) {
      auto peers = wsv_->getLedgerPeers();
      if (peers) {
        std::vector<std::string> addresses;
        std::transform(peers->begin(),
                       peers->end(),
                       std::back_inserter(addresses),
                       [](auto &p) { return p->address(); });
        transport_->publishProposal(std::move(proposal), addresses);
      } else {
        log_->error("Cannot get the peer list");
      }
    }

    OrderingServiceImpl::~OrderingServiceImpl() {
      handle_.unsubscribe();
    }
  }  // namespace ordering
}  // namespace iroha
