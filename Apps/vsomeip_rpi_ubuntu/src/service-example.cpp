#include <iostream>
#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID   0x1234
#define SAMPLE_INSTANCE_ID  0x5678
#define SAMPLE_METHOD_ID    0x0421   

std::shared_ptr<vsomeip::application> app;

void on_message(const std::shared_ptr<vsomeip::message> &req) {

    auto payload = req->get_payload();
    std::string received(reinterpret_cast<const char*>(payload->get_data()),
                         payload->get_length());

    std::cout << "SERVER: Received: " << received << std::endl;

    // Prepare response message
    auto resp = vsomeip::runtime::get()->create_response(req);

    std::string answer = received + " -> who is there?";
    std::vector<vsomeip::byte_t> data(answer.begin(), answer.end());

    auto pl = vsomeip::runtime::get()->create_payload();
    pl->set_data(data);
    resp->set_payload(pl);

    app->send(resp);
}

int main() {
    app = vsomeip::runtime::get()->create_application("World");
    app->init();

    app->register_message_handler(SAMPLE_SERVICE_ID,
                                  SAMPLE_INSTANCE_ID,
                                  SAMPLE_METHOD_ID,
                                  on_message);

    app->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app->start();
}
