#ifndef LOGGER_H
#define LOGGER_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

// LogLevel is an enumeration to represent the severity of the logs.
enum class LogLevel {
    Debug,   // For detailed information typically of interest only when diagnosing problems.
    Info,    // For informational messages that highlight the progress of the application.
    Warning, // For potentially harmful situations.
    Error    // For error events that might still allow the application to continue running.
};

// Logger is a utility class that provides a global logging facility.
class Logger {
private:
    static bool enabled;                                    // Controls if logging is globally enabled or disabled.
    static LogLevel globalLogLevel;                         // The global log level threshold.
    static std::map<std::string, LogLevel> moduleLogLevels; // Log levels specific to modules.
    static std::set<std::string> disabledModules;           // Set of modules with logging disabled.
    static std::ofstream logFile;                           // File stream for logging.

public:
    // Enables or disables logging globally.
    static void enableLogging(bool enable);

    // Sets the global log level.
    static void setGlobalLogLevel(LogLevel level);

    // Sets the log level for a specific module.
    static void setModuleLogLevel(const std::string& module, LogLevel level);

    // Disables logging for a specific module.
    static void disableModuleLogging(const std::string& module);

    // Enables logging for a previously disabled module.
    static void enableModuleLogging(const std::string& module);

    // Logs a message if the specified log level is at or above the configured log level.
    static void log(const std::string& module, LogLevel level, const std::string& message);

    // Converts LogLevel to a readable string.
    static std::string toString(LogLevel level);

    // Initialize the log file
    static void initializeLogFile(const std::string& filePath);

    static void flushLogs();
};

#endif // LOGGER_H
