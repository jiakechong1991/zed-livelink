#include <iostream>
#include <chrono>

int main() {
    auto now = std::chrono::high_resolution_clock::now(); // 获取当前时间点
    auto now_as_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now); // 将时间点转换为纳秒
    auto value = now_as_ns.time_since_epoch(); // 获取自纪元以来的时间长度
    uint64_t value_as_uint64 = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::nano>>(value).count();
    std::cout << "Current timestamp in nanoseconds: " << value_as_uint64 << std::endl;
    return 0;
}