/*
Copyright 2017 Soramitsu Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <torii/command_client.hpp>
#include <torii/command_service_handler.hpp>
#include <network/grpc_call.hpp>
#include <block.pb.h>
#include <grpc++/grpc++.h>
#include <thread>

namespace torii {

  using iroha::protocol::Transaction;
  using iroha::protocol::ToriiResponse;

  CommandSyncClient::CommandSyncClient(const std::string& ip, const int port)
    : stub_(iroha::protocol::CommandService::NewStub(
    grpc::CreateChannel(ip + ":" + std::to_string(port), grpc::InsecureChannelCredentials())))
  {}

  CommandSyncClient::~CommandSyncClient() {
    cq_.Shutdown();
  }

  /**
   * requests tx to a torii server and returns response (blocking, sync)
   * @param tx
   * @return ToriiResponse
   */
  ToriiResponse CommandSyncClient::Torii(const Transaction& tx) {
    ToriiResponse response;

    std::unique_ptr<grpc::ClientAsyncResponseReader<iroha::protocol::ToriiResponse>> rpc(
      stub_->AsyncTorii(&context_, tx, &cq_)
    );

    using State = network::UntypedCall<torii::CommandServiceHandler>::State;

    rpc->Finish(&response, &status_, (void *)static_cast<int>(State::ResponseSent));

    void* got_tag;
    bool ok = false;

    if (!cq_.Next(&got_tag, &ok)) {  // CompletionQueue::Next() is blocking.
      throw std::runtime_error("CompletionQueue::Next() returns error");
    }

    assert(got_tag == (void *)static_cast<int>(State::ResponseSent));
    assert(ok);

    if (status_.ok()) {
      return response;
    }

    response.set_code(iroha::protocol::ResponseCode::FAIL);
    response.set_message("RPC failed");
    return response;
  }

  /**
   * manages state of an async call.
   */
  struct ToriiAsyncClientCall {
    iroha::protocol::ToriiResponse response;
    grpc::ClientContext context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<iroha::protocol::ToriiResponse>> response_reader;
    CommandAsyncClient::Callback callback;
  };

  /**
   * requests tx to a torii server and returns response (non-blocking)
   * @param tx
   * @param callback
   */
  void CommandAsyncClient::Torii(
    const Transaction& tx,
    const std::function<void(ToriiResponse& response)>& callback)
  {
    auto call = new ToriiAsyncClientCall;
    call->callback = callback;
    call->response_reader = stub_->AsyncTorii(&call->context, tx, &cq_);
    call->response_reader->Finish(&call->response, &call->status, (void*)call);
  }

  /**
   * sets ip and port and calls listenToriiNonBlocking() in a new thread.
   * @param ip
   * @param port
   */
  CommandAsyncClient::CommandAsyncClient(const std::string& ip, const int port)
    : stub_(iroha::protocol::CommandService::NewStub(
    grpc::CreateChannel(ip + ":" + std::to_string(port), grpc::InsecureChannelCredentials())))
  {
    listener_ = std::thread(&CommandAsyncClient::listen, this);
  }

  CommandAsyncClient::~CommandAsyncClient() {
    cq_.Shutdown();
    listener_.join();
  }

  /**
   * starts response listener of a non-blocking torii client.
   */
  void CommandAsyncClient::listen() {
    void* got_tag;
    bool ok = false;

    while (cq_.Next(&got_tag, &ok)) {
      if (!got_tag || !ok) {
        break;
      }

      auto call = static_cast<ToriiAsyncClientCall*>(got_tag);

      if (call->status.ok()) {
        call->callback(call->response);
      } else {
        ToriiResponse responseFailure;
        responseFailure.set_code(iroha::protocol::ResponseCode::FAIL);
        responseFailure.set_message("RPC failed");
        call->callback(responseFailure);
      }

      delete call;
    }
  }

}  // namespace torii
