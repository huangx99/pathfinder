#pragma once

#include "pathfinder/foundation/error.h"
#include <vector>
#include <optional>
#include <cassert>

namespace pathfinder::foundation {

// Forward declaration
template <typename T>
class Result;

// Result<void> specialization for operations that don't return a value
template <>
class Result<void> {
public:
    // Create a successful result
    static Result ok() {
        return Result{true, {}};
    }

    // Create a failed result with a single error
    static Result fail(ErrorDetail error) {
        return Result{false, {std::move(error)}};
    }

    // Create a failed result with multiple errors
    static Result fail(std::vector<ErrorDetail> errors) {
        return Result{false, std::move(errors)};
    }

    // Check if the result is successful
    bool is_ok() const { return ok_; }

    // Check if the result is an error
    bool is_error() const { return !ok_; }

    // Get the errors (only valid when is_error())
    const std::vector<ErrorDetail>& errors() const { return errors_; }

    // Add a warning (doesn't change ok status)
    Result& add_warning(ErrorDetail warning) {
        warnings_.push_back(std::move(warning));
        return *this;
    }

    // Get warnings
    const std::vector<ErrorDetail>& warnings() const { return warnings_; }

private:
    Result(bool ok, std::vector<ErrorDetail> errors)
        : ok_(ok), errors_(std::move(errors)) {}

    bool ok_;
    std::vector<ErrorDetail> errors_;
    std::vector<ErrorDetail> warnings_;
};

// Result<T> for operations that return a value
template <typename T>
class Result {
public:
    // Create a successful result with a value
    static Result ok(T value) {
        return Result{true, std::move(value), {}};
    }

    // Create a failed result with a single error
    static Result fail(ErrorDetail error) {
        return Result{false, T{}, {std::move(error)}};
    }

    // Create a failed result with multiple errors
    static Result fail(std::vector<ErrorDetail> errors) {
        return Result{false, T{}, std::move(errors)};
    }

    // Check if the result is successful
    bool is_ok() const { return ok_; }

    // Check if the result is an error
    bool is_error() const { return !ok_; }

    // Get the value (only valid when is_ok())
    const T& value() const {
        assert(ok_ && "Cannot get value from error result");
        return value_;
    }

    // Get the value (only valid when is_ok())
    T& value() {
        assert(ok_ && "Cannot get value from error result");
        return value_;
    }

    // Get the errors (only valid when is_error())
    const std::vector<ErrorDetail>& errors() const { return errors_; }

    // Add a warning (doesn't change ok status)
    Result& add_warning(ErrorDetail warning) {
        warnings_.push_back(std::move(warning));
        return *this;
    }

    // Get warnings
    const std::vector<ErrorDetail>& warnings() const { return warnings_; }

private:
    Result(bool ok, T value, std::vector<ErrorDetail> errors)
        : ok_(ok), value_(std::move(value)), errors_(std::move(errors)) {}

    bool ok_;
    T value_;
    std::vector<ErrorDetail> errors_;
    std::vector<ErrorDetail> warnings_;
};

} // namespace pathfinder::foundation
