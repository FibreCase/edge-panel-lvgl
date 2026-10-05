#pragma once
#include <string>
#include <stdexcept>
namespace panel {
class Platform {
public:
    virtual ~Platform() = default;
    virtual bool poll() = 0;
    virtual void screenshot(const std::string&) { throw std::runtime_error("Screenshot is available only with SDL"); }
};
}
