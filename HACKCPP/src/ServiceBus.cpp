#include "ServiceBus.hpp"

ServiceBus& ServiceBus::getInstance() {
    static ServiceBus instance;
    return instance;
}
