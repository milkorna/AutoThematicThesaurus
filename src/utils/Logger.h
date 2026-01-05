#pragma once

#include <fstream>
#include <map>
#include <set>
#include <string>

/**
 * @brief Severity level for log messages
 * @details Ordered from lowest to highest severity
 */
enum class LogLevel {
    Debug,   ///< Detailed diagnostic information, typically for development
    Info,    ///< Informational messages highlighting application progress
    Warning, ///< Potentially harmful situations that may need attention
    Error    ///< Error events that allow application to continue running
};

/**
 * @brief Global logging utility with module-specific configuration
 * @details Provides centralized logging with configurable severity levels,
 * per-module control, and file output. Implements singleton pattern via static methods.
 */
class Logger {
  private:
    /// @brief Global flag to enable/disable all logging
    static bool enabled;

    /// @brief Default log level threshold for all modules
    static LogLevel globalLogLevel;

    /// @brief Map of module-specific log level overrides
    static std::map<std::string, LogLevel> moduleLogLevels;

    /// @brief Set of modules with logging completely disabled
    static std::set<std::string> disabledModules;

    /// @brief Output file stream for persistent logging
    static std::ofstream logFile;

  public:
    /**
     * @brief Enables or disables logging globally
     * @details When disabled, all log calls are ignored regardless of module or level
     *
     * @param enable True to enable logging, false to disable
     */
    static void enableLogging(bool enable);

    /**
     * @brief Sets the default log level threshold
     * @details Messages below this level are filtered out (unless module override exists)
     *
     * @param level Minimum LogLevel to output
     */
    static void setGlobalLogLevel(LogLevel level);

    /**
     * @brief Sets log level for a specific module
     * @details Overrides global log level for the specified module
     *
     * @param module Module name
     * @param level Minimum LogLevel for this module
     */
    static void setModuleLogLevel(const std::string& module, LogLevel level);

    /**
     * @brief Completely disables logging for a specific module
     * @details All messages from this module are ignored regardless of level
     *
     * @param module Module name to disable
     */
    static void disableModuleLogging(const std::string& module);

    /**
     * @brief Re-enables logging for a previously disabled module
     * @details Module will use its configured log level (module-specific or global)
     *
     * @param module Module name to enable
     */
    static void enableModuleLogging(const std::string& module);

    /**
     * @brief Logs a message with severity level and module source
     * @details Checks if message should be logged based on global/module configuration.
     * Outputs to log file if initialized, otherwise to stderr.
     *
     * @param module Source module name
     * @param level Message severity level
     * @param message Log message text
     */
    static void log(const std::string& module, LogLevel level, const std::string& message);

    /**
     * @brief Converts LogLevel enum to human-readable string
     *
     * @param level LogLevel to convert
     * @return String representation ("Debug", "Info", "Warning", "Error", or "Unknown")
     */
    [[nodiscard]] static std::string toString(LogLevel level);

    /**
     * @brief Initializes the log file for output
     * @details Opens file in append mode. Logs error to stderr if file open fails.
     *
     * @param filePath Path to log file
     */
    static void initializeLogFile(const std::string& filePath);

    /**
     * @brief Flushes and closes the log file
     * @details Ensures all pending log data is written before closing
     */
    static void flushLogs();
};