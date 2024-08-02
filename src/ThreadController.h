#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace ThreadController {

    class ThreadController {
    public:
        void reachCheckpoint()
        {
            std::unique_lock<std::mutex> lock(mtx);
            reached = true;
            cv.notify_all();
        }

        void waitForCheckpoint()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return reached; });
        }

    private:
        std::mutex mtx;
        std::condition_variable cv;
        bool reached = false;
    };

    // void worker(ThreadController& controller, int id)
    // {
    //     std::cout << "Thread " << id << " waiting at checkpoint.\n";
    //     controller.waitForCheckpoint();
    //     std::cout << "Thread " << id << " resuming work.\n";
    //     // Äàëüíåéøàÿ ðàáîòà ïîòîêà
    // }

    // void checkpoint(ThreadController& controller)
    // {
    //     std::this_thread::sleep_for(std::chrono::seconds(2));
    //     std::cout << "Checkpoint reached. Resuming all threads.\n";
    //     controller.reachCheckpoint();
    // }
}