#include "platform/linux/evdev_touch.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
namespace panel {
namespace {
void axis_info(int fd,int code,input_absinfo& info) {
    if(ioctl(fd,EVIOCGABS(code),&info)<0) throw std::runtime_error("Cannot read touch axis: "+std::string(std::strerror(errno)));
}
}
EvdevTouch::EvdevTouch(const std::string& path,TouchTransform transform):transform_(transform) {
    fd_=open(path.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);
    if(fd_<0) throw std::runtime_error("Cannot open touch device "+path+": "+std::strerror(errno));
    try {
        input_absinfo x{},y{},slot{};
        multitouch_=ioctl(fd_,EVIOCGABS(ABS_MT_POSITION_X),&x)==0;
        if(multitouch_) {
            axis_info(fd_,ABS_MT_POSITION_Y,y);axis_info(fd_,ABS_MT_SLOT,slot);
            if(slot.minimum!=0||slot.maximum<0||slot.maximum>63) throw std::runtime_error("Touch requires type-B slots in range 0..63");
            slots_.resize(slot.maximum+1);slot_=slot.value;
        } else { axis_info(fd_,ABS_X,x);axis_info(fd_,ABS_Y,y); }
        if(x.maximum<=x.minimum||y.maximum<=y.minimum) throw std::runtime_error("Touch ABS ranges are invalid");
        transform_.xmin=x.minimum;transform_.xmax=x.maximum;
        transform_.ymin=y.minimum;transform_.ymax=y.maximum;
        resync();queue_.clear();
        char name[256]{};
        ioctl(fd_,EVIOCGNAME(sizeof(name)),name);
        std::cerr<<"Touch "<<path<<" ("<<name<<") "<<(multitouch_?"type-B":"single-touch")
                 <<" ABS x="<<x.minimum<<".."<<x.maximum<<" y="<<y.minimum<<".."<<y.maximum
                 <<" space="<<(transform_.logical?"logical":"physical")<<"\n";
    } catch(...) { close(fd_);fd_=-1;throw; }
}
EvdevTouch::~EvdevTouch() { if(fd_>=0) close(fd_); }
void EvdevTouch::resync() {
    if(multitouch_) {
        for(int code:{ABS_MT_TRACKING_ID,ABS_MT_POSITION_X,ABS_MT_POSITION_Y}) {
            std::vector<int32_t> values(slots_.size()+1);values[0]=code;
            if(ioctl(fd_,EVIOCGMTSLOTS(values.size()*sizeof(int32_t)),values.data())<0)
                throw std::runtime_error("Cannot resynchronize multitouch slots");
            for(size_t i=0;i<slots_.size();++i) {
                if(code==ABS_MT_TRACKING_ID) slots_[i].id=values[i+1];
                else if(code==ABS_MT_POSITION_X) slots_[i].x=values[i+1];
                else slots_[i].y=values[i+1];
            }
        }
        input_absinfo slot{};axis_info(fd_,ABS_MT_SLOT,slot);slot_=slot.value;
    } else {
        input_absinfo x{},y{};axis_info(fd_,ABS_X,x);axis_info(fd_,ABS_Y,y);
        raw_x_=x.value;raw_y_=y.value;
        unsigned char keys[(KEY_MAX+8)/8]{};
        if(ioctl(fd_,EVIOCGKEY(sizeof(keys)),keys)<0) throw std::runtime_error("Cannot read touch key state");
        down_=(keys[BTN_TOUCH/8]&(1u<<(BTN_TOUCH%8)))!=0;
    }
    selected_=-1;commit();
}
void EvdevTouch::commit() {
    Point p=current_.point;bool pressed=down_;
    if(multitouch_) {
        if(selected_<0||slots_[selected_].id<0) {
            selected_=-1;
            for(size_t i=0;i<slots_.size();++i) if(slots_[i].id>=0) { selected_=static_cast<int>(i);break; }
        }
        pressed=selected_>=0;
        if(pressed) p=transform_.map(slots_[selected_].x,slots_[selected_].y);
    } else if(pressed) p=transform_.map(raw_x_,raw_y_);
    // Preserve down/up boundaries, while coalescing motion for the same state.
    if(!queue_.empty()&&queue_.back().pressed==pressed) queue_.back()={p,pressed};
    else {
        if(queue_.size()>=64) { queue_.clear();queue_.push_back({p,false}); }
        queue_.push_back({p,pressed});
    }
}
void EvdevTouch::poll() {
    input_event events[64];
    for(int batch=0;batch<16;++batch) {
        ssize_t bytes=read(fd_,events,sizeof(events));
        if(bytes<0&&(errno==EAGAIN||errno==EWOULDBLOCK)) return;
        if(bytes<0&&errno==EINTR) continue;
        if(bytes<=0) throw std::runtime_error("Touch device disconnected or read failed");
        if(bytes%sizeof(input_event)!=0) throw std::runtime_error("Invalid evdev packet size");
        for(size_t i=0;i<static_cast<size_t>(bytes)/sizeof(input_event);++i) {
            const auto& event=events[i];
            if(event.type==EV_SYN&&event.code==SYN_DROPPED) {
                dropped_=true;queue_.clear();queue_.push_back({current_.point,false});continue;
            }
            if(dropped_) {
                if(event.type==EV_SYN&&event.code==SYN_REPORT) { resync();dropped_=false; }
                continue;
            }
            if(event.type==EV_SYN&&event.code==SYN_REPORT) { commit();continue; }
            if(event.type==EV_KEY&&event.code==BTN_TOUCH) down_=event.value!=0;
            if(event.type!=EV_ABS) continue;
            if(multitouch_) {
                if(event.code==ABS_MT_SLOT) slot_=event.value;
                else if(slot_>=0&&static_cast<size_t>(slot_)<slots_.size()) {
                    auto& slot=slots_[slot_];
                    if(event.code==ABS_MT_TRACKING_ID) slot.id=event.value;
                    else if(event.code==ABS_MT_POSITION_X) slot.x=event.value;
                    else if(event.code==ABS_MT_POSITION_Y) slot.y=event.value;
                }
            } else {
                if(event.code==ABS_X) raw_x_=event.value;
                else if(event.code==ABS_Y) raw_y_=event.value;
            }
        }
    }
}
TouchSample EvdevTouch::next() {
    if(!queue_.empty()) { current_=queue_.front();queue_.pop_front(); }
    return current_;
}
}
