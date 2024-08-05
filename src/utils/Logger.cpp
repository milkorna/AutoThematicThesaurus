#include <Logger.h>

// Initialize static members
bool Logger::enabled = true;                             // Logging is enabled by default.
LogLevel Logger::globalLogLevel = LogLevel::Debug;       // Default global log level is set to Debug.
std::map<std::string, LogLevel> Logger::moduleLogLevels; // Empty initial module-specific log levels.
std::set<std::string> Logger::disabledModules;           // No modules are disabled initially.
std::ofstream Logger::logFile;                           // Log file stream.

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
void Logger::setModuleLogLevel(const std::string& module, LogLevel level)
{
    moduleLogLevels[module] = level;
}

// Disables logging for a specific module.
void Logger::disableModuleLogging(const std::string& module)
{
    disabledModules.insert(module);
}

// Enables logging for a previously disabled module.
void Logger::enableModuleLogging(const std::string& module)
{
    disabledModules.erase(module);
}

// Logs a message if the specified log level is at or above the configured log level.
void Logger::log(const std::string& module, LogLevel level, const std::string& message)
{
    if (enabled && disabledModules.find(module) == disabledModules.end()) {
        LogLevel effectiveLevel = globalLogLevel;
        if (moduleLogLevels.find(module) != moduleLogLevels.end()) {
            effectiveLevel = moduleLogLevels[module];
        }

        if (level >= effectiveLevel) {
            if (logFile.is_open()) {
                logFile << "[" << std::to_string(static_cast<int>(level)) << "] " << module << ": " << message
                        << std::endl;
            } else {
                std::cerr << "Log file not open!" << std::endl;
            }
        }
    }
}

// Initialize the log file
void Logger::initializeLogFile(const std::string& filePath)
{
    logFile.open(filePath, std::ios::out | std::ios::app);
    if (!logFile) {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
    }
}
