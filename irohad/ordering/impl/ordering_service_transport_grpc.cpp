/**
 * Copyright Soramitsu Co., Ltd. 2018 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ordering/impl/ordering_service_transport_grpc.hpp"

#include "backend/protobuf/transaction.hpp"
#include "builders/protobuf/proposal.hpp"
#include "interfaces/common_objects/transaction_sequence_common.hpp"
#include "network/impl/grpc_channel_builder.hpp"
#include "validators/default_validator.hpp"

using namespace iroha::ordering;

void OrderingServiceTransportGrpc::subscribe(
    std::shared_ptr<iroha::network::OrderingServiceNotification> subscriber) {
  subscriber_ = subscriber;
}

grpc::Status OrderingServiceTransportGrpc::onTransaction(
    ::grpc::ServerContext *context,
    const iroha::protocol::Transaction *request,
    ::google::protobuf::Empty *response) {
  log_->info("OrderingServiceTransportGrpc::onTransaction");
  if (subscriber_.expired()) {
    log_->error("No subscriber");
  } else {
    auto batch_result =
        shared_model::interface::TransactionBatch::createTransactionBatch<shared_model::validation::DefaultTransactionValidator>(
            std::make_shared<shared_model::proto::Transaction>(
                iroha::protocol::Transaction(*request)));
    batch_result.match(
        [this](iroha::expected::Value<shared_model::interface::TransactionBatch>
                   &batch) {
          subscriber_.lock()->onTransactions(std::move(batch.value));
        },
        [this](const iroha::expected::Error<std::string> &error) {
          log_->error(
              "Could not create batch from received single transaction: {}",
              error.error);
        });
  }

  return ::grpc::Status::OK;
}

grpc::Status OrderingServiceTransportGrpc::onBatch(
    ::grpc::ServerContext *context,
    const protocol::TxList *request,
    ::google::protobuf::Empty *response) {
  log_->info("OrderingServiceTransportGrpc::onBatch");
  if (subscriber_.expired()) {
    log_->error("No subscriber");
  } else {
    //
    auto txs =
        std::vector<std::shared_ptr<shared_model::interface::Transaction>>(
            request->transactions_size());
    std::transform(
        std::begin(request->transactions()),
        std::end(request->transactions()),
        std::begin(txs),
        [](const auto &tx) {
          return std::make_shared<shared_model::proto::Transaction>(tx);
        });

    auto batch_result =
        shared_model::interface::TransactionBatch::createTransactionBatch(
            txs,
            shared_model::validation::SignedTransactionsCollectionValidator<
                shared_model::validation::DefaultTransactionValidator,
                shared_model::validation::BatchOrderValidator>());
    batch_result.match(
        [this](iroha::expected::Value<shared_model::interface::TransactionBatch>
                   &batch) {
          subscriber_.lock()->onTransactions(std::move(batch.value));
        },
        [this](const iroha::expected::Error<std::string> &error) {
          log_->error(
              "Could not create batch from received transaction list: {}",
              error.error);
        });
    }
  return ::grpc::Status::OK;
}

void OrderingServiceTransportGrpc::publishProposal(
    std::unique_ptr<shared_model::interface::Proposal> proposal,
    const std::vector<std::string> &peers) {
  log_->info("OrderingServiceTransportGrpc::publishProposal");
  std::unordered_map<std::string,
                     std::unique_ptr<proto::OrderingGateTransportGrpc::Stub>>
      peers_map;
  for (const auto &peer : peers) {
    peers_map[peer] =
        network::createClient<proto::OrderingGateTransportGrpc>(peer);
  }

  for (const auto &peer : peers_map) {
    auto call = new AsyncClientCall;
    auto proto = static_cast<shared_model::proto::Proposal *>(proposal.get());
    log_->debug("Publishing proposal: '{}'",
                proto->getTransport().DebugString());
    call->response_reader = peer.second->AsynconProposal(
        &call->context, proto->getTransport(), &cq_);

    call->response_reader->Finish(&call->reply, &call->status, call);
  }
}

OrderingServiceTransportGrpc::OrderingServiceTransportGrpc()
    : network::AsyncGrpcClient<google::protobuf::Empty>(
          logger::log("OrderingServiceTransportGrpc")) {}
