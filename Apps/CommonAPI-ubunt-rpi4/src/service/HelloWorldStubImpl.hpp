#pragma once
#include <v0/commonapi/examples/HelloWorldStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>

class HelloWorldStubImpl
    : public v0::commonapi::examples::HelloWorldStubDefault {

public:
    HelloWorldStubImpl() = default;
    virtual ~HelloWorldStubImpl() = default;

    virtual void sayHello(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        std::string _name,
        sayHelloReply_t _reply) override;
};
