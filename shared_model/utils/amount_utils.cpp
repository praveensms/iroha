/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/amount_utils.hpp"

#include <tuple>

#include <boost/format.hpp>
#include <boost/optional.hpp>

namespace shared_model {
  namespace detail {
    using AmountType = boost::multiprecision::uint256_t;
    const AmountType ten = 10;

    namespace {
      const std::string kPrecisionError = "increaseValuePrecision overflow";
      boost::optional<AmountType> increaseValuePrecision(
          const AmountType &value, int degree) {
        auto res = value * pow(ten, degree);
        if (res / pow(ten, degree) != value) {
          return {};
        }
        return {res};
      }
    }  // namespace

    boost::optional<std::tuple<AmountType,
                               AmountType,
                               shared_model::interface::types::PrecisionType>>
    adjustPair(const shared_model::interface::Amount &a,
               const shared_model::interface::Amount &b) {
      auto max_precision = std::max(a.precision(), b.precision());
      auto some_val_a =
          increaseValuePrecision(a.intValue(), max_precision - a.precision());
      if (not some_val_a) {
        return {};
      }
      auto some_val_b =
          increaseValuePrecision(b.intValue(), max_precision - b.precision());
      if (not some_val_b) {
        return {};
      }
      return {std::make_tuple(
          some_val_a.value(), some_val_b.value(), max_precision)};
    }

    /**
     * Sums up two amounts.
     * Result is returned
     * @param a left term
     * @param b right term
     */
    iroha::expected::PolymorphicResult<shared_model::interface::Amount,
                                       std::string>
    operator+(const shared_model::interface::Amount &a,
              const shared_model::interface::Amount &b) {
      auto p = adjustPair(a, b);
      if (not p) {
        return iroha::expected::makeError(
            std::make_shared<std::string>(kPrecisionError));
      }
      auto &val_a = std::get<0>(p.value()), &val_b = std::get<1>(p.value());
      auto max_precision = std::get<2>(p.value());
      auto sum = val_a + val_b;
      if (val_a < a.intValue() || val_b < b.intValue() || sum < val_a
          || sum < val_b) {
        return iroha::expected::makeError(std::make_shared<std::string>(
            (boost::format("addition overflows (%s + %s)") % a.intValue().str()
             % b.intValue().str())
                .str()));
      }
      std::string val = sum.str();
      if (max_precision != 0) {
        val.insert((val.rbegin() + max_precision).base(), '.');
      }
      return iroha::expected::makeValue(
          std::make_shared<shared_model::interface::Amount>(std::move(val)));
    }

    /**
     * Subtracts two amounts.
     * Result is returned
     * @param a left term
     * @param b right term
     */
    iroha::expected::PolymorphicResult<shared_model::interface::Amount,
                                       std::string>
    operator-(const shared_model::interface::Amount &a,
              const shared_model::interface::Amount &b) {
      // check if a greater than b
      if (a.intValue() < b.intValue()) {
        return iroha::expected::makeError(std::make_shared<std::string>(
            (boost::format("minuend is smaller than subtrahend (%s - %s)")
             % a.intValue().str() % b.intValue().str())
                .str()));
      }
      auto p = adjustPair(a, b);
      if (not p) {
        return iroha::expected::makeError(
            std::make_shared<std::string>(kPrecisionError));
      }
      auto &val_a = std::get<0>(p.value()), &val_b = std::get<1>(p.value());
      auto max_precision = std::get<2>(p.value());
      if (val_a < a.intValue() || val_b < b.intValue()) {
        return iroha::expected::makeError(
            std::make_shared<std::string>("new precision overflows number"));
      }
      auto diff = val_a - val_b;
      std::string val = diff.str();
      if (max_precision != 0 && val.size() > max_precision + 1) {
        auto ptr = val.rbegin() + max_precision;
        val.insert(ptr.base(), '.');
      }
      return iroha::expected::makeValue(
          std::make_shared<shared_model::interface::Amount>(std::move(val)));
    }

    /**
     * Make amount with bigger precision
     * Result is returned
     * @param a amount
     * @param b right term
     */
    iroha::expected::PolymorphicResult<shared_model::interface::Amount,
                                       std::string>
    makeAmountWithPrecision(const shared_model::interface::Amount &amount,
                            const int new_precision) {
      if (amount.precision() > new_precision) {
        return iroha::expected::makeError(std::make_shared<std::string>(
            (boost::format("new precision is smaller than current (%d < %d)")
             % new_precision % amount.precision())
                .str()));
      }
      auto val_amount = increaseValuePrecision(
          amount.intValue(), new_precision - amount.precision());
      if (not val_amount) {
        return iroha::expected::makeError(
            std::make_shared<std::string>(kPrecisionError));
      }
      if (val_amount.value() < amount.intValue()) {
        return iroha::expected::makeError(
            std::make_shared<std::string>("operation overflows number"));
      }
      std::string val = val_amount.value().str();
      if (new_precision != 0) {
        val.insert((val.rbegin() + new_precision).base(), '.');
      }
      return iroha::expected::makeValue(
          std::make_shared<shared_model::interface::Amount>(std::move(val)));
    }

    int compareAmount(const shared_model::interface::Amount &a,
                      const shared_model::interface::Amount &b) {
      if (a.precision() == b.precision()) {
        return (a.intValue() < b.intValue())
            ? -1
            : (a.intValue() > b.intValue()) ? 1 : 0;
      }
      // when different precisions transform to have the same scale
      auto p = adjustPair(a, b);
      auto &val_a = std::get<0>(p.value()), &val_b = std::get<1>(p.value());
      return (val_a < val_b) ? -1 : (val_a > val_b) ? 1 : 0;
    }
  }  // namespace detail
}  // namespace shared_model
