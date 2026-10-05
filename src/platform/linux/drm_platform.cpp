#include "platform/linux/drm_platform.hpp"
#include <xf86drm.h>
#include <drm_fourcc.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>
namespace panel {
namespace {
void fail(const std::string& operation) { throw std::runtime_error(operation+": "+std::strerror(errno)); }
template<typename T,void(*Free)(T*)> using DrmPtr=std::unique_ptr<T,decltype(Free)>;
}
DrmPlatform::DrmPlatform(const DrmOptions& options):rotation_(options.rotation),pixels_(1280*720),draw_buffer_(1280*80) {
    try { setup(options); } catch(...) { cleanup();throw; }
}
DrmPlatform::~DrmPlatform() { cleanup(); }
void DrmPlatform::setup(const DrmOptions& options) {
    if(rotation_!=90&&rotation_!=270) throw std::invalid_argument("DRM rotation must be 90 or 270");
    if(!options.no_touch) {
        if(options.touch_device.empty()) throw std::invalid_argument("Set --touch-device /dev/input/eventX, or use --no-touch for display testing");
        auto transform=options.touch;transform.rotation=rotation_;
        touch_=std::make_unique<EvdevTouch>(options.touch_device,transform);
    }
    fd_=open(options.device.c_str(),O_RDWR|O_CLOEXEC);
    if(fd_<0) fail("Open DRM "+options.device);
    uint64_t dumb=0;
    if(drmGetCap(fd_,DRM_CAP_DUMB_BUFFER,&dumb)<0||!dumb) throw std::runtime_error("DRM device does not support dumb scanout buffers");
    if(drmSetMaster(fd_)<0) fail("Acquire DRM master (stop flutter-pi/compositor first)");
    DrmPtr<drmModeRes,drmModeFreeResources> resources(drmModeGetResources(fd_),drmModeFreeResources);
    if(!resources) fail("Read DRM resources");
    DrmPtr<drmModeConnector,drmModeFreeConnector> connector(nullptr,drmModeFreeConnector);
    for(int i=0;i<resources->count_connectors;++i) {
        DrmPtr<drmModeConnector,drmModeFreeConnector> candidate(drmModeGetConnector(fd_,resources->connectors[i]),drmModeFreeConnector);
        if(!candidate||candidate->connection!=DRM_MODE_CONNECTED) continue;
        if(options.connector&&candidate->connector_id!=options.connector) continue;
        // Auto-selection is deliberately restricted to the attached DSI panel.
        if(!options.connector&&candidate->connector_type!=DRM_MODE_CONNECTOR_DSI) continue;
        for(int j=0;j<candidate->count_modes;++j) {
            const auto& mode=candidate->modes[j];
            if(mode.hdisplay==720&&mode.vdisplay==1280) {
                connector_=candidate->connector_id;mode_=mode;
                connector=std::move(candidate);break;
            }
        }
        if(connector) break;
    }
    if(!connector) throw std::runtime_error("No connected DSI connector with a 720x1280 mode; check --drm-device/--connector");
    // Prefer its current encoder/CRTC; otherwise select a compatible pipeline.
    std::vector<uint32_t> encoders;
    if(connector->encoder_id) encoders.push_back(connector->encoder_id);
    for(int i=0;i<connector->count_encoders;++i) if(connector->encoders[i]!=connector->encoder_id) encoders.push_back(connector->encoders[i]);
    for(auto id:encoders) {
        DrmPtr<drmModeEncoder,drmModeFreeEncoder> encoder(drmModeGetEncoder(fd_,id),drmModeFreeEncoder);
        if(!encoder) continue;
        for(int pass=0;pass<2&&!crtc_;++pass) for(int i=0;i<resources->count_crtcs;++i) {
            auto id_crtc=resources->crtcs[i];
            if(!(encoder->possible_crtcs&(1u<<i))) continue;
            if(options.crtc&&id_crtc!=options.crtc) continue;
            if(pass==0&&id_crtc!=encoder->crtc_id) continue;
            DrmPtr<drmModeCrtc,drmModeFreeCrtc> candidate(drmModeGetCrtc(fd_,id_crtc),drmModeFreeCrtc);
            if(!candidate) continue;
            // Do not take a pipeline already driving an unrelated output.
            if(pass==1&&candidate->mode_valid&&id_crtc!=encoder->crtc_id) continue;
            crtc_=id_crtc;break;
        }
        if(crtc_) break;
    }
    if(!crtc_) throw std::runtime_error("No compatible available CRTC; check --crtc");
    saved_=drmModeGetCrtc(fd_,crtc_);
    if(!saved_) fail("Save current CRTC");
    for(int i=0;i<resources->count_connectors;++i) {
        DrmPtr<drmModeConnector,drmModeFreeConnector> c(drmModeGetConnector(fd_,resources->connectors[i]),drmModeFreeConnector);
        if(!c||!c->encoder_id) continue;
        DrmPtr<drmModeEncoder,drmModeFreeEncoder> e(drmModeGetEncoder(fd_,c->encoder_id),drmModeFreeEncoder);
        if(e&&e->crtc_id==crtc_) saved_connectors_.push_back(c->connector_id);
    }
    for(auto& buffer:buffers_) create_buffer(buffer);
    if(drmModeSetCrtc(fd_,crtc_,buffers_[0].fb,0,0,&connector_,1,&mode_)<0) fail("Set 720x1280 DRM mode");
    mode_set_=true;
    lv_tick_set_cb(ticks);
    display_=lv_display_create(1280,720);
    if(!display_) throw std::runtime_error("Cannot create LVGL display");
    lv_display_set_color_format(display_,LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_user_data(display_,this);
    lv_display_set_buffers(display_,draw_buffer_.data(),nullptr,draw_buffer_.size()*4,LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display_,flush);
    if(touch_) {
        pointer_=lv_indev_create();
        if(!pointer_) throw std::runtime_error("Cannot create LVGL touch input");
        lv_indev_set_type(pointer_,LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(pointer_,display_);
        lv_indev_set_user_data(pointer_,this);
        lv_indev_set_read_cb(pointer_,input);
    }
    std::cerr<<"DRM "<<options.device<<" connector="<<connector_<<" CRTC="<<crtc_
             <<" physical=720x1280 logical=1280x720 rotation="<<rotation_<<"\n";
}
void DrmPlatform::create_buffer(Buffer& buffer) {
    drm_mode_create_dumb create{};create.width=720;create.height=1280;create.bpp=32;
    if(drmIoctl(fd_,DRM_IOCTL_MODE_CREATE_DUMB,&create)<0) fail("Create DRM dumb buffer");
    buffer.handle=create.handle;buffer.pitch=create.pitch;buffer.size=create.size;
    uint32_t handles[4]={buffer.handle},pitches[4]={buffer.pitch},offsets[4]={0};
    if(drmModeAddFB2(fd_,720,1280,DRM_FORMAT_XRGB8888,handles,pitches,offsets,&buffer.fb,0)<0) fail("Create XRGB8888 framebuffer");
    drm_mode_map_dumb map{};map.handle=buffer.handle;
    if(drmIoctl(fd_,DRM_IOCTL_MODE_MAP_DUMB,&map)<0) fail("Map DRM dumb buffer");
    void* address=mmap(nullptr,buffer.size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_,map.offset);
    if(address==MAP_FAILED) fail("mmap DRM buffer");
    buffer.map=static_cast<uint8_t*>(address);
    std::memset(buffer.map,0,buffer.size);
}
uint32_t DrmPlatform::ticks() {
    auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return static_cast<uint32_t>(ms);
}
void DrmPlatform::flush(lv_display_t* display,const lv_area_t* area,uint8_t* data) {
    auto& self=*static_cast<DrmPlatform*>(lv_display_get_user_data(display));
    const int width=area->x2-area->x1+1;
    for(int y=area->y1;y<=area->y2;++y)
        std::memcpy(self.pixels_.data()+y*1280+area->x1,data+(y-area->y1)*width*4,width*4);
    // Flush callbacks only update CPU staging memory, never the scanned/pending buffer.
    if(lv_display_flush_is_last(display)) self.dirty_=true;
    lv_display_flush_ready(display);
}
void DrmPlatform::present() {
    if(pending_||!dirty_) return;
    auto& back=buffers_[1-front_];
    rotate_frame(pixels_.data(),back.map,back.pitch,rotation_);
    if(drmModePageFlip(fd_,crtc_,back.fb,DRM_MODE_PAGE_FLIP_EVENT,this)<0) fail("DRM page flip");
    pending_=true;dirty_=false;flip_started_=std::chrono::steady_clock::now();
}
void DrmPlatform::flipped(int,unsigned int,unsigned int,unsigned int,void* data) {
    auto& self=*static_cast<DrmPlatform*>(data);
    self.front_=1-self.front_;self.pending_=false;
}
bool DrmPlatform::poll() {
    pollfd event{fd_,POLLIN,0};
    int result=::poll(&event,1,0);
    if(result<0&&errno!=EINTR) fail("Poll DRM");
    if(event.revents&(POLLERR|POLLHUP|POLLNVAL)) throw std::runtime_error("DRM device disconnected");
    if(event.revents&POLLIN) {
        drmEventContext context{};context.version=2;context.page_flip_handler=flipped;
        if(drmHandleEvent(fd_,&context)<0) fail("Read DRM flip event");
    }
    if(pending_&&std::chrono::steady_clock::now()-flip_started_>std::chrono::seconds(2))
        throw std::runtime_error("DRM page flip timed out");
    if(touch_) touch_->poll();
    present();return true;
}
void DrmPlatform::input(lv_indev_t* device,lv_indev_data_t* data) {
    auto& self=*static_cast<DrmPlatform*>(lv_indev_get_user_data(device));
    auto sample=self.touch_->next();
    data->point.x=sample.point.x;data->point.y=sample.point.y;
    data->state=sample.pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
    data->continue_reading=self.touch_->queued();
}
void DrmPlatform::cleanup() noexcept {
    if(pointer_) { lv_indev_delete(pointer_);pointer_=nullptr; }
    if(display_) { lv_display_delete(display_);display_=nullptr; }
    touch_.reset();
    if(fd_>=0) {
        // Complete the last flip before restoring the saved pipeline, bounded to 250 ms.
        if(pending_) {
            pollfd event{fd_,POLLIN,0};
            if(::poll(&event,1,250)>0&&(event.revents&POLLIN)) {
                drmEventContext context{};context.version=2;context.page_flip_handler=flipped;
                drmHandleEvent(fd_,&context);
            }
        }
        if(mode_set_&&saved_) {
            int result;
            if(saved_->mode_valid) result=drmModeSetCrtc(fd_,saved_->crtc_id,saved_->buffer_id,saved_->x,saved_->y,
                saved_connectors_.data(),static_cast<int>(saved_connectors_.size()),&saved_->mode);
            else result=drmModeSetCrtc(fd_,crtc_,0,0,0,nullptr,0,nullptr);
            if(result<0) std::cerr<<"Could not restore original DRM mode: "<<std::strerror(errno)<<"\n";
        }
        for(auto& buffer:buffers_) {
            if(buffer.map) munmap(buffer.map,buffer.size);
            if(buffer.fb) drmModeRmFB(fd_,buffer.fb);
            if(buffer.handle) { drm_mode_destroy_dumb destroy{};destroy.handle=buffer.handle;drmIoctl(fd_,DRM_IOCTL_MODE_DESTROY_DUMB,&destroy); }
            buffer={};
        }
        drmDropMaster(fd_);close(fd_);fd_=-1;
    }
    if(saved_) { drmModeFreeCrtc(saved_);saved_=nullptr; }
}
}
