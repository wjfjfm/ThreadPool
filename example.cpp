#include <iostream>

#include "ThreadPool.h"

int main() {
    ThreadPool pool(4); // 指定四个工作线程

    // 将任务在池中入队，并返回一个 std::future
    auto f = pool.enqueue([](int life) {
        return life + 1;
    }, 42);

    // 从 future 中获得执行结果
    std::cout << "Waiting..." << std::endl;
    f.wait();
    std::cout << f.get() << std::endl;
    return 0;
}
