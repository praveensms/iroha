/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_POSTGRES_COMMAND_EXECUTOR_HPP
#define IROHA_POSTGRES_COMMAND_EXECUTOR_HPP

#include "ametsuchi/command_executor.hpp"

#include "ametsuchi/impl/postgres_wsv_common.hpp"

namespace iroha {
  namespace ametsuchi {

    class PostgresCommandExecutor : public CommandExecutor {
     public:
      explicit PostgresCommandExecutor(pqxx::nontransaction &transaction);

      void setCreatorAccountId(
          const shared_model::interface::types::AccountIdType
              &creator_account_id) override;

      CommandResult operator()(
          const shared_model::interface::AddAssetQuantity &command) override;

      CommandResult operator()(
          const shared_model::interface::AddPeer &command) override;

      CommandResult operator()(
          const shared_model::interface::AddSignatory &command) override;

      CommandResult operator()(
          const shared_model::interface::AppendRole &command) override;

      CommandResult operator()(
          const shared_model::interface::CreateAccount &command) override;

      CommandResult operator()(
          const shared_model::interface::CreateAsset &command) override;

      CommandResult operator()(
          const shared_model::interface::CreateDomain &command) override;

      CommandResult operator()(
          const shared_model::interface::CreateRole &command) override;

      CommandResult operator()(
          const shared_model::interface::DetachRole &command) override;

      CommandResult operator()(
          const shared_model::interface::GrantPermission &command) override;

      CommandResult operator()(
          const shared_model::interface::RemoveSignatory &command) override;

      CommandResult operator()(
          const shared_model::interface::RevokePermission &command) override;

      CommandResult operator()(
          const shared_model::interface::SetAccountDetail &command) override;

      CommandResult operator()(
          const shared_model::interface::SetQuorum &command) override;

      CommandResult operator()(
          const shared_model::interface::SubtractAssetQuantity &command)
          override;

      CommandResult operator()(
          const shared_model::interface::TransferAsset &command) override;

     private:
      pqxx::nontransaction &transaction_;

      shared_model::interface::types::AccountIdType creator_account_id_;

      using ExecuteType = decltype(makeExecuteResult(transaction_));
      ExecuteType execute_;

      static expected::Error<CommandError> makeCommandError(
          const std::string &error_message,
          const std::string &command_name) noexcept {
        return expected::makeError(CommandError{command_name, error_message});
      }

      /**
       * Transforms result which contains pqxx to CommandResult,
       * which will have error message generated by error_generator
       * appended to error received from given result
       * @param result which can be received by calling execute_
       * @param error_generator function which must generate error message
       * to be used as a return error.
       * Function is passed instead of string to avoid overhead of string
       * construction in successful case.
       * @return CommandResult with combined error message
       * in case of result contains error
       */
      template <typename Function>
      CommandResult makeCommandResult(
          expected::Result<pqxx::result, std::string> &&result,
          std::string command_name,
          Function &&error_generator) const noexcept {
        return result.match(
            [](expected::Value<pqxx::result> v) -> CommandResult { return {}; },
            [&error_generator,
             &command_name](expected::Error<std::string> e) -> CommandResult {
              return makeCommandError(error_generator() + "\n" + e.error,
                                      command_name);
            });
      }

      /**
       * Transforms result which contains pqxx to CommandResult,
       * which will have error message generated by error_generator
       * appended to error received from given result
       * @param result which can be received by calling execute_
       * @param error_generator functions which must generate error message
       * to be used as a return error.
       * Functions are passed instead of string to avoid overhead of string
       * construction in successful case.
       * @return CommandResult with combined error message
       * in case of result contains error
       */
      CommandResult makeCommandResultByValue(
          expected::Result<pqxx::result, std::string> &&result,
          std::string command_name,
          std::vector<std::function<std::string()>> &error_generator) const noexcept {
        return result.match(
            [&error_generator,
             &command_name](expected::Value<pqxx::result> v) -> CommandResult {
              size_t code = v.value[0].at("result").template as<size_t>();
              for (size_t i = 0; i < error_generator.size(); i++) {
                // Since success code is 0 and error codes starts with 1
                if (code == i + 1) {
                  return makeCommandError(error_generator[i](), command_name);
                }
              }
              return {};
            },
            [&command_name](expected::Error<std::string> e) -> CommandResult {
              return makeCommandError(e.error, command_name);
            });
      }
    };
  }  // namespace ametsuchi
}  // namespace iroha

#endif  // IROHA_POSTGRES_COMMAND_EXECUTOR_HPP
