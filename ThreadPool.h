#include <future>
#include <thread>
#include <array>
#include <deque>
#include <vector>
#include <memory>
#include <iostream>

class ThreadPool {
 public:
    ThreadPool() : ThreadPool(1) {};
    ThreadPool(int worker);
    ~ThreadPool();
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&& ... args)
        -> std::future< typename std::result_of< F( Args... ) >::type>;
 private:
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;
    std::mutex task_mutex;
    std::condition_variable cv;
    bool closed;
    int count;
};

ThreadPool::ThreadPool(int worker) : closed(false), count(0){
    workers.reserve(worker);
    for(int i=0; i<worker; i++) {
        workers.emplace_back(
            [this]{
                // std::cout << "worker waiting for init" << std::endl;
                std::unique_lock<std::mutex> lock(this->task_mutex);
                // int thread_count = ++this->count;
                // std::cout << "worker " << thread_count << " started" << std::endl;
                while(true) {
                    while (this->tasks.empty()) {
                        if (this->closed) {
                            // std::cout << "worker " << thread_count << " closed" << std::endl;
                            return;
                        }
                        // std::cout << "worker " << thread_count << " waiting for notify" << std::endl;
                        this->cv.wait(lock);
                    }
                    // std::cout << "worker " << thread_count << " get a task" << std::endl;
                    auto task = this->tasks.front();
                    this->tasks.pop_front();
                    lock.unlock();
                    task();
                    // std::cout << "worker " << thread_count << " finished task" << std::endl;
                    lock.lock();
                }
            }
        );
    }
}



template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&& ... args)
    -> std::future< typename std::result_of< F( Args... ) >::type>
{

    using return_type = typename std::result_of< F( Args...) >::type;
    auto task = std::make_shared<std::packaged_task<return_type()> > (
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    auto result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(task_mutex);
        if (!closed) {
            tasks.emplace_back([task]{ (*task)();});
            cv.notify_one();
        }
    }

    return result;
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(task_mutex);
        closed = true;
    }
    cv.notify_all();
    for (auto && worker : workers) {
        worker.join();
    }
}
