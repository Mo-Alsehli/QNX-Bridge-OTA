#include <CommonAPI/CommonAPI.hpp>
#include <iostream>
#include <thread>
#include <v0/commonapi/examples/HelloWorldProxy.hpp>

int main() {
    CommonAPI::Runtime::setProperty("LibraryBase", "HelloWorld");

    auto runtime = CommonAPI::Runtime::get();
    auto proxy = runtime->buildProxy<v0::commonapi::examples::HelloWorldProxy>(
        "local", "commonapi.examples.HelloWorld", "client-sample");

    std::cout << "[CLIENT] Waiting for service..." << std::endl;
 while (!proxy->isAvailable())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::cout << "[CLIENT] Service available." << std::endl;

    CommonAPI::CallStatus status;
    std::string message;

    for (;;) {
        proxy->sayHello("Mohamed Magdi", status, message);

        std::cout << "[CLIENT] Response: " << message << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}