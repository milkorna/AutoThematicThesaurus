#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// \class ThreadController
// \brief This class provides mechanisms for thread synchronization using condition variables and mutexes.
//        It allows threads to wait for a checkpoint and ensures all threads reach the checkpoint before proceeding.

class ThreadController {
public:
    // \brief Call this function to signal that threads have reached the checkpoint.
    //        It will notify all waiting threads.
    void reachCheckpoint()
    {
        std::unique_lock<std::mutex> lock(mtx);
        reached = true;
        cv.notify_all();
    }

    // \brief Call this function to make threads wait until they reach the checkpoint.
    void waitForCheckpoint()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return reached; });
    }

    // \brief Call this function to pause threads until all threads reach this point.
    void pauseUntilAllThreadsReach()
    {
        std::unique_lock<std::mutex> lock(mtx);
        waiting_threads++;
        if (waiting_threads == total_threads) {
            reached = true;
            cv.notify_all();
        } else {
            cv.wait(lock, [this] { return reached; });
        }
    }

    // \brief Set the total number of threads participating in the barrier.
    // \param num_threads       The total number of threads.
    void setTotalThreads(int num_threads)
    {
        std::unique_lock<std::mutex> lock(mtx);
        total_threads = num_threads;
    }

private:
    std::mutex mtx;             ///< Mutex for synchronizing access to shared data.
    std::condition_variable cv; ///< Condition variable for signaling threads.
    bool reached = false;       ///< Flag indicating if the checkpoint has been reached.
    int waiting_threads = 0;    ///< Counter for the number of threads waiting at the checkpoint.
    int total_threads = 0;      ///< Total number of threads participating in the barrier.
};
