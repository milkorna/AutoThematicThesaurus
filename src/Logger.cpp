#include "Logger.h"

// Enables or disables logging globally.
void Logger::enableLogging(bool enable)
{
    Logger::enabled = enable;
}

// Sets the global log level.
void Logger::setGlobalLogLevel(LogLevel level)
{
    Logger::globalLogLevel = level;
}

// Sets the log level for a specific module.
void Logger::setModuleLogLevel(const std::string &module, LogLevel level)
{
    moduleLogLevels[module] = level;
}

// Disables logging for a specific module.
void Logger::disableModuleLogging(const std::string &module)
{
    disabledModules.insert(module);
}

// Enables logging for a previously disabled module.
void Logger::enableModuleLogging(const std::string &module)
{
    disabledModules.erase(module);
}

// Logs a message if the specified log level is at or above the configured log level.
void Logger::log(const std::string &module, LogLevel level, const std::string &message)
{
    if (enabled && disabledModules.find(module) == disabledModules.end())
    {
        LogLevel effectiveLevel = globalLogLevel;
        if (moduleLogLevels.find(module) != moduleLogLevels.end())
        {
            effectiveLevel = moduleLogLevels[module];
        }

        if (level >= effectiveLevel)
        {
            std::cout << "[" << std::to_string(static_cast<int>(level)) << "] " << module << ": " << message << std::endl;
        }
    }
}

// Static member initializations
bool Logger::enabled = true;                             // Logging is enabled by default.
LogLevel Logger::globalLogLevel = LogLevel::Debug;       // Default global log level is set to Debug.
std::map<std::string, LogLevel> Logger::moduleLogLevels; // Empty initial module-specific log levels.
std::set<std::string> Logger::disabledModules;           // No modules are disabled initially.