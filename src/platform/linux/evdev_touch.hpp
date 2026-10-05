#pragma once
#include "platform/linux/coordinates.hpp"
#include <linux/input.h>
#include <deque>
#include <string>
#include <vector>
namespace panel {
struct TouchSample { Point point; bool pressed; };
class EvdevTouch {
public:
    EvdevTouch(const std::string& path,TouchTransform transform);
    ~EvdevTouch();
    void poll();
    TouchSample next();
    bool queued() const { return !queue_.empty(); }
private:
    struct Slot { int id=-1,x=0,y=0; };
    void resync();
    void commit();
    int fd_=-1,slot_=0,selected_=-1;
    bool multitouch_=false,down_=false,dropped_=false;
    int raw_x_=0,raw_y_=0;
    TouchTransform transform_;
    std::vector<Slot> slots_;
    std::deque<TouchSample> queue_;
    TouchSample current_{{0,0},false};
};
}
