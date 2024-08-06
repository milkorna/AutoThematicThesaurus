#include <fcntl.h>
#include <string>
#include <unistd.h>

// \class OutputRedirector
// \brief This class redirects the standard output (stdout) and standard error (stderr) to a specified log file.
//        It saves the original file descriptors for stdout and stderr and restores them upon destruction or when
//        explicitly requested.
class OutputRedirector {
public:
    // \brief Constructor that initializes the OutputRedirector with a log file.
    // \param log_file      The path to the log file where stdout and stderr will be redirected.
    OutputRedirector(const std::string& log_file)
    {
        // Open the log file with write-only access, create if it doesn't exist, and truncate if it does.
        log_fd = open(log_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        // Save the current stdout and stderr file descriptors.
        saved_stdout = dup(STDOUT_FILENO);
        saved_stderr = dup(STDERR_FILENO);

        // Redirect stdout and stderr to the log file.
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
    }

    // \brief Destructor that restores the original stdout and stderr file descriptors.
    ~OutputRedirector()
    {
        restore();
    }

    // \brief Restores the original stdout and stderr file descriptors.
    void restore()
    {
        if (log_fd != -1) {
            // Restore the original stdout and stderr.
            dup2(saved_stdout, STDOUT_FILENO);
            dup2(saved_stderr, STDERR_FILENO);

            // Close the saved file descriptors and the log file descriptor.
            close(saved_stdout);
            close(saved_stderr);
            close(log_fd);

            // Invalidate the log file descriptor to indicate restoration is complete.
            log_fd = -1;
        }
    }

private:
    int saved_stdout; ///< File descriptor to save the original stdout.
    int saved_stderr; ///< File descriptor to save the original stderr.
    int log_fd = -1;  ///< File descriptor for the log file.
};