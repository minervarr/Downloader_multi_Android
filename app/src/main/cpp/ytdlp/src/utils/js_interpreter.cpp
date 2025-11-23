#include "ytdlp/utils/js_interpreter.hpp"
#include <quickjs.h>
#include <memory>
#include <cstring>

namespace ytdlp::utils {

JSInterpreter::JSInterpreter()
    : runtime_(nullptr), context_(nullptr), last_error_() {
    // Create QuickJS runtime (global state)
    runtime_ = JS_NewRuntime();
    if (!runtime_) {
        throw JSExecutionError("Failed to create JavaScript runtime");
    }

    // Create execution context
    context_ = JS_NewContext(runtime_);
    if (!context_) {
        JS_FreeRuntime(runtime_);
        throw JSExecutionError("Failed to create JavaScript context");
    }
}

JSInterpreter::~JSInterpreter() {
    if (context_) {
        JS_FreeContext(context_);
    }
    if (runtime_) {
        JS_FreeRuntime(runtime_);
    }
}

JSInterpreter::JSInterpreter(JSInterpreter&& other) noexcept
    : runtime_(other.runtime_), context_(other.context_),
      last_error_(std::move(other.last_error_)) {
    other.runtime_ = nullptr;
    other.context_ = nullptr;
}

JSInterpreter& JSInterpreter::operator=(JSInterpreter&& other) noexcept {
    if (this != &other) {
        if (context_) {
            JS_FreeContext(context_);
        }
        if (runtime_) {
            JS_FreeRuntime(runtime_);
        }

        runtime_ = other.runtime_;
        context_ = other.context_;
        last_error_ = std::move(other.last_error_);

        other.runtime_ = nullptr;
        other.context_ = nullptr;
    }
    return *this;
}

bool JSInterpreter::check_exception(void* js_value_ptr) {
    JSValue js_value = *static_cast<JSValue*>(js_value_ptr);

    if (JS_IsException(js_value)) {
        JSValue exception = JS_GetException(context_);

        // Get error message
        const char* error_str = JS_ToCString(context_, exception);
        if (error_str) {
            last_error_ = error_str;
            JS_FreeCString(context_, error_str);
        } else {
            last_error_ = "Unknown JavaScript error";
        }

        // Get stack trace if available
        JSValue stack = JS_GetPropertyStr(context_, exception, "stack");
        if (!JS_IsUndefined(stack)) {
            const char* stack_str = JS_ToCString(context_, stack);
            if (stack_str) {
                last_error_ += "\n";
                last_error_ += stack_str;
                JS_FreeCString(context_, stack_str);
            }
        }
        JS_FreeValue(context_, stack);
        JS_FreeValue(context_, exception);

        return true;
    }

    return false;
}

std::optional<std::string> JSInterpreter::value_to_string(void* js_value_ptr) {
    JSValue js_value = *static_cast<JSValue*>(js_value_ptr);

    if (JS_IsString(js_value)) {
        // Direct string value
        const char* str = JS_ToCString(context_, js_value);
        if (str) {
            std::string result(str);
            JS_FreeCString(context_, str);
            return result;
        }
    } else if (JS_IsNumber(js_value)) {
        // Convert number to string
        JSValue str_value = JS_ToString(context_, js_value);
        if (!check_exception(&str_value)) {
            const char* str = JS_ToCString(context_, str_value);
            if (str) {
                std::string result(str);
                JS_FreeCString(context_, str);
                JS_FreeValue(context_, str_value);
                return result;
            }
        }
        JS_FreeValue(context_, str_value);
    } else {
        // Try generic toString conversion
        JSValue str_value = JS_ToString(context_, js_value);
        if (!check_exception(&str_value) && !JS_IsUndefined(str_value)) {
            const char* str = JS_ToCString(context_, str_value);
            if (str) {
                std::string result(str);
                JS_FreeCString(context_, str);
                JS_FreeValue(context_, str_value);
                return result;
            }
        }
        JS_FreeValue(context_, str_value);
    }

    return std::nullopt;
}

std::string JSInterpreter::execute(const std::string& code) {
    last_error_.clear();

    // Evaluate the JavaScript code
    JSValue result = JS_Eval(context_, code.c_str(), code.length(),
                             "<eval>", JS_EVAL_TYPE_GLOBAL);

    // Check for exceptions
    if (check_exception(&result)) {
        JS_FreeValue(context_, result);
        throw JSExecutionError("JavaScript execution failed: " + last_error_);
    }

    // Convert result to string
    auto result_str = value_to_string(&result);
    JS_FreeValue(context_, result);

    if (!result_str) {
        throw JSExecutionError("JavaScript result is not convertible to string");
    }

    return *result_str;
}

void JSInterpreter::evaluate(const std::string& code) {
    last_error_.clear();

    JSValue result = JS_Eval(context_, code.c_str(), code.length(),
                             "<eval>", JS_EVAL_TYPE_GLOBAL);

    if (check_exception(&result)) {
        JS_FreeValue(context_, result);
        throw JSExecutionError("JavaScript evaluation failed: " + last_error_);
    }

    JS_FreeValue(context_, result);
}

std::string JSInterpreter::call_function(const std::string& function_name,
                                         const std::string& argument) {
    last_error_.clear();

    // Get global object
    JSValue global = JS_GetGlobalObject(context_);

    // Get function from global object
    JSValue func = JS_GetPropertyStr(context_, global, function_name.c_str());
    JS_FreeValue(context_, global);

    if (!JS_IsFunction(context_, func)) {
        JS_FreeValue(context_, func);
        throw JSExecutionError("'" + function_name + "' is not a function");
    }

    // Create string argument
    JSValue arg = JS_NewString(context_, argument.c_str());

    // Call function
    JSValue result = JS_Call(context_, func, JS_UNDEFINED, 1, &arg);

    JS_FreeValue(context_, arg);
    JS_FreeValue(context_, func);

    // Check for exceptions
    if (check_exception(&result)) {
        JS_FreeValue(context_, result);
        throw JSExecutionError("Function call failed: " + last_error_);
    }

    // Convert result to string
    auto result_str = value_to_string(&result);
    JS_FreeValue(context_, result);

    if (!result_str) {
        throw JSExecutionError("Function result is not convertible to string");
    }

    return *result_str;
}

std::string JSInterpreter::get_last_error() const {
    return last_error_;
}

} // namespace ytdlp::utils
