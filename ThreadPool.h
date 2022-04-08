#include <future>
#include <thread>
#include <array>
#include <deque>
#include <vector>
#include <memory>

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
};

ThreadPool::ThreadPool(int worker) : closed(false){
    workers.reserve(worker);
    for(int i=0; i<worker; i++) {
        workers.emplace_back(
            [this]{
                std::unique_lock<std::mutex> lock(this->task_mutex);
                while(!this->closed) {
                    this->cv.wait(lock);
                    if (!tasks.empty()) {
                        auto task = this->tasks.front();
                        this->tasks.pop_front();
                        lock.unlock();
                        task();
                    }
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
            cv.notify_all();
        }
    }

    return result;
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(task_mutex);
        closed = true;
        for (auto && worker : workers) {
            worker.join();
        }
    } // unlock
}
