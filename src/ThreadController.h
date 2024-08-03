#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace ThreadController {

    class ThreadController {
    public:
        // Call this function to make threads reach the checkpoint
        void reachCheckpoint()
        {
            std::unique_lock<std::mutex> lock(mtx);
            reached = true;
            cv.notify_all();
        }

        // Call this function to make threads wait until they reach the checkpoint
        void waitForCheckpoint()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return reached; });
        }

        // Call this function to pause threads until all threads reach this point
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

        // Set the total number of threads participating in the barrier
        void setTotalThreads(int num_threads)
        {
            std::unique_lock<std::mutex> lock(mtx);
            total_threads = num_threads;
        }

    private:
        std::mutex mtx;
        std::condition_variable cv;
        bool reached = false;
        int waiting_threads = 0;
        int total_threads = 0;
    };
}