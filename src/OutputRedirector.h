#include <fcntl.h>
#include <string>
#include <unistd.h>

class OutputRedirector {
public:
    OutputRedirector(const std::string& log_file)
    {
        log_fd = open(log_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        saved_stdout = dup(STDOUT_FILENO);
        saved_stderr = dup(STDERR_FILENO);

        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
    }

    ~OutputRedirector()
    {
        restore();
    }

    void restore()
    {
        if (log_fd != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            dup2(saved_stderr, STDERR_FILENO);

            close(saved_stdout);
            close(saved_stderr);
            close(log_fd);

            log_fd = -1;
        }
    }

private:
    int saved_stdout;
    int saved_stderr;
    int log_fd = -1;
};