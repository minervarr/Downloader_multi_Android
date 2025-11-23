/**
 * JavaScript Interpreter Header
 *
 * Provides a JavaScript interpreter using QuickJS for executing
 * JavaScript code (used for YouTube signature decryption, etc.).
 */

#ifndef YTDLP_UTILS_JS_INTERPRETER_HPP
#define YTDLP_UTILS_JS_INTERPRETER_HPP

#include <string>
#include <optional>
#include <stdexcept>

// Forward declarations for QuickJS types (avoid including quickjs.h in header)
struct JSRuntime;
struct JSContext;

namespace ytdlp::utils {

// ============================================================================
// Exception Class
// ============================================================================

/**
 * Exception thrown when JavaScript execution fails
 */
class JSExecutionError : public std::runtime_error {
public:
    explicit JSExecutionError(const std::string& message)
        : std::runtime_error(message) {}
};

// ============================================================================
// JavaScript Interpreter Class
// ============================================================================

/**
 * JavaScript interpreter using QuickJS
 *
 * This class provides a simple interface to execute JavaScript code
 * and call JavaScript functions from C++. It's primarily used for
 * YouTube signature decryption.
 */
class JSInterpreter {
public:
    /**
     * Construct JavaScript interpreter
     * @throws JSExecutionError if runtime/context creation fails
     */
    JSInterpreter();

    /**
     * Destructor - cleans up QuickJS resources
     */
    ~JSInterpreter();

    // Disable copy (QuickJS resources are not copyable)
    JSInterpreter(const JSInterpreter&) = delete;
    JSInterpreter& operator=(const JSInterpreter&) = delete;

    // Enable move
    JSInterpreter(JSInterpreter&& other) noexcept;
    JSInterpreter& operator=(JSInterpreter&& other) noexcept;

    /**
     * Execute JavaScript code and return result as string
     * @param code JavaScript code to execute
     * @return Result converted to string
     * @throws JSExecutionError if execution fails
     */
    std::string execute(const std::string& code);

    /**
     * Evaluate JavaScript code (discard result)
     * @param code JavaScript code to evaluate
     * @throws JSExecutionError if execution fails
     */
    void evaluate(const std::string& code);

    /**
     * Call JavaScript function with string argument
     * @param function_name Name of function in global scope
     * @param argument String argument to pass to function
     * @return Result converted to string
     * @throws JSExecutionError if function doesn't exist or execution fails
     */
    std::string call_function(const std::string& function_name,
                               const std::string& argument);

    /**
     * Get last error message
     * @return Last error message (empty if no error)
     */
    std::string get_last_error() const;

private:
    JSRuntime* runtime_;    // QuickJS runtime
    JSContext* context_;    // QuickJS context
    std::string last_error_; // Last error message

    /**
     * Check if JSValue is an exception and store error message
     * @param js_value_ptr Pointer to JSValue to check
     * @return True if exception occurred
     */
    bool check_exception(void* js_value_ptr);

    /**
     * Convert JSValue to string
     * @param js_value_ptr Pointer to JSValue to convert
     * @return String value or nullopt if conversion fails
     */
    std::optional<std::string> value_to_string(void* js_value_ptr);
};

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_JS_INTERPRETER_HPP
