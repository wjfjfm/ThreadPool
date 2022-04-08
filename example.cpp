#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#include "ThreadPool.h"

int main() {
    ThreadPool pool(4); // 指定四个工作线程

    // 将任务在池中入队，并返回一个 std::future
    std::cout << "befor enqueue" << std::endl;
    std::vector<int> nums(1000, 1);
    std::array<std::future<int>,10> results;
    for(int i=0; i<10; i++) {
        results[i] = pool.enqueue([&n = nums, i]{
            int count = 0;
            for(int j=i*100; j<i*100 + 100; j++) count += n[j];
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return count;
        });
    }
    std::cout << "enqueue finished" <<  std::endl;

    // 从 future 中获得执行结果
    std::cout << "Waiting..." << std::endl;
    int result = 0;
    for(int i=0; i<10; i++){
        result += results[i].get();
    }

    std::cout << "The count of nums is: " << result << std::endl;

    return 0;
}
