#include <iostream>
#include <thread>
#include <condition_variable>
#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID   0x1234
#define SAMPLE_INSTANCE_ID  0x5678
#define SAMPLE_METHOD_ID    0x0421

std::shared_ptr<vsomeip::application> app;
std::mutex mtx;
std::condition_variable cv;

void on_availability(vsomeip::service_t, vsomeip::instance_t, bool available) {
    if(available) {
        cv.notify_one();
    }
}

void on_message(const std::shared_ptr<vsomeip::message> &resp) {
    auto payload = resp->get_payload();
    std::string received(reinterpret_cast<const char*>(payload->get_data()),
                         payload->get_length());

    std::cout << "CLIENT: Received: " << received << std::endl;
}

void sender() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);  // wait for availability

    auto request = vsomeip::runtime::get()->create_request();
    request->set_service(SAMPLE_SERVICE_ID);
    request->set_instance(SAMPLE_INSTANCE_ID);
    request->set_method(SAMPLE_METHOD_ID);

    std::string msg = "NOK";
    std::vector<vsomeip::byte_t> data(msg.begin(), msg.end());

    auto pl = vsomeip::runtime::get()->create_payload();
    pl->set_data(data);

    request->set_payload(pl);

    app->send(request);
}

int main() {
    app = vsomeip::runtime::get()->create_application("Hello");
    app->init();

    app->register_availability_handler(SAMPLE_SERVICE_ID,
                                       SAMPLE_INSTANCE_ID,
                                       on_availability);

    app->register_message_handler(SAMPLE_SERVICE_ID,
                                  SAMPLE_INSTANCE_ID,
                                  SAMPLE_METHOD_ID,
                                  on_message);

    app->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);

    std::thread t(sender);

    app->start();
}
