#pragma once
#include "platform/platform.hpp"
#include "platform/linux/evdev_touch.hpp"
#include <lvgl.h>
#include <xf86drmMode.h>
#include <array>
#include <chrono>
#include <memory>
#include <vector>
namespace panel {
struct DrmOptions {
    std::string device="/dev/dri/card0",touch_device;
    uint32_t connector=0,crtc=0;
    int rotation=90;
    bool no_touch=false;
    TouchTransform touch;
};
class DrmPlatform final:public Platform {
public:
    explicit DrmPlatform(const DrmOptions& options);
    ~DrmPlatform() override;
    bool poll() override;
private:
    struct Buffer { uint32_t handle=0,fb=0,pitch=0;uint64_t size=0;uint8_t* map=nullptr; };
    void setup(const DrmOptions&);
    void cleanup() noexcept;
    void create_buffer(Buffer&);
    void present();
    static uint32_t ticks();
    static void flush(lv_display_t*,const lv_area_t*,uint8_t*);
    static void input(lv_indev_t*,lv_indev_data_t*);
    static void flipped(int,unsigned int,unsigned int,unsigned int,void*);
    int fd_=-1,rotation_=90,front_=0;
    uint32_t connector_=0,crtc_=0;
    drmModeModeInfo mode_{};
    drmModeCrtc* saved_=nullptr;
    std::vector<uint32_t> saved_connectors_;
    std::array<Buffer,2> buffers_{};
    bool mode_set_=false,pending_=false,dirty_=false;
    std::chrono::steady_clock::time_point flip_started_;
    std::vector<uint32_t> pixels_,draw_buffer_;
    std::unique_ptr<EvdevTouch> touch_;
    lv_display_t* display_=nullptr;
    lv_indev_t* pointer_=nullptr;
};
}
