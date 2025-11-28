#include <CommonAPI/CommonAPI.hpp>
#include "commonapi/examples/HelloWorldProxy.hpp"

int main() {
    auto runtime = CommonAPI::Runtime::get();

    std::shared_ptr<commonapi::examples::HelloWorldProxy<>> proxy;
    bool connected = false;

    while (!connected) {
        runtime->buildProxy(
            proxy,
            "local",
            "commonapi.examples.HelloWorld"
        );

        if (proxy && proxy->isAvailable()) {
            connected = true;
        } else {
            std::cout << "Waiting for service..." << std::endl;
            sleep(1);
        }
    }

    std::string name = "RaspberryPi";
    std::string response;

    proxy->sayHello(name, response);

    std::cout << "Received response: " << response << std::endl;

    return 0;
}
