#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include "HelloWorldStubImpl.hpp"

int main() {
    CommonAPI::Runtime::setProperty("LibraryBase", "HelloWorld");

    auto runtime = CommonAPI::Runtime::get();
    auto service = std::make_shared<HelloWorldStubImpl>();

    bool registered =
        runtime->registerService("local",
                                 "commonapi.examples.HelloWorld",
                                 service,
                                 "service-sample");

    if (!registered) {
        std::cerr << "Failed to register service!" << std::endl;
        return 1;
    }

    std::cout << "[SERVICE] HelloWorld Service started." << std::endl;

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}
