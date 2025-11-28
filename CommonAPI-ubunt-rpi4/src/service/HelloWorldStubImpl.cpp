#include "HelloWorldStubImpl.hpp"
#include <iostream>
#include <sstream>


void HelloWorldStubImpl::sayHello(
    const std::shared_ptr<CommonAPI::ClientId>,
    std::string _name,
    sayHelloReply_t _reply) {

    std::stringstream ss;
    ss << "Hello " << _name << "!";

    std::cout << "[SERVICE] sayHello('" << _name << "') → '" << ss.str() << "'" << std::endl;

    _reply(ss.str());
}
