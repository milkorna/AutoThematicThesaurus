#include <iostream>
#include <string>
#include <map>
#include <set>

// LogLevel is an enumeration to represent the severity of the logs.
enum class LogLevel
{
    Debug,   // For detailed information typically of interest only when diagnosing problems.
    Info,    // For informational messages that highlight the progress of the application.
    Warning, // For potentially harmful situations.
    Error    // For error events that might still allow the application to continue running.
};

// Logger is a utility class that provides a global logging facility.
class Logger
{
private:
    static bool enabled;                                    // Controls if logging is globally enabled or disabled.
    static LogLevel globalLogLevel;                         // The global log level threshold.
    static std::map<std::string, LogLevel> moduleLogLevels; // Log levels specific to modules.
    static std::set<std::string> disabledModules;           // Set of modules with logging disabled.

public:
    // Enables or disables logging globally.
    static void enableLogging(bool enable)
    {
        Logger::enabled = enable;
    }

    // Sets the global log level.
    static void setGlobalLogLevel(LogLevel level)
    {
        Logger::globalLogLevel = level;
    }

    // Sets the log level for a specific module.
    static void setModuleLogLevel(const std::string &module, LogLevel level)
    {
        moduleLogLevels[module] = level;
    }

    // Disables logging for a specific module.
    static void disableModuleLogging(const std::string &module)
    {
        disabledModules.insert(module);
    }

    // Enables logging for a previously disabled module.
    static void enableModuleLogging(const std::string &module)
    {
        disabledModules.erase(module);
    }

    // Logs a message if the specified log level is at or above the configured log level.
    static void log(const std::string &module, LogLevel level, const std::string &message)
    {
        if (Logger::enabled && disabledModules.find(module) == disabledModules.end())
        {
            LogLevel effectiveLevel = globalLogLevel;
            if (moduleLogLevels.find(module) != moduleLogLevels.end())
            {
                effectiveLevel = moduleLogLevels[module]; // Override global level with module-specific level.
            }

            if (level >= effectiveLevel)
            {
                std::cout << "[" << toString(level) << "] " << module << ": " << message << std::endl;
            }
        }
    }

    // Converts LogLevel to a readable string.
    static std::string toString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARNING";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }
};

// Static member initializations
bool Logger::enabled = true;                             // Logging is enabled by default.
LogLevel Logger::globalLogLevel = LogLevel::Debug;       // Default global log level is set to Debug.
std::map<std::string, LogLevel> Logger::moduleLogLevels; // Empty initial module-specific log levels.
std::set<std::string> Logger::disabledModules;           // No modules are disabled initially.
