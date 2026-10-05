#include "platform/sdl/sdl_platform.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace panel {
SdlPlatform::SdlPlatform() : pixels_(1280*720), draw_buffer_(1280*80) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) throw std::runtime_error(SDL_GetError());
    window_ = SDL_CreateWindow("Edge Panel · LVGL mock", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
    if(!window_) throw std::runtime_error(SDL_GetError());
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if(!renderer_) throw std::runtime_error(SDL_GetError());
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 1280, 720);
    if(!texture_) throw std::runtime_error(SDL_GetError());
    lv_tick_set_cb(SDL_GetTicks);
    display_ = lv_display_create(1280, 720);
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_user_data(display_, this);
    lv_display_set_buffers(display_, draw_buffer_.data(), nullptr, draw_buffer_.size()*4, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display_, flush);
    pointer_ = lv_indev_create();
    lv_indev_set_type(pointer_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(pointer_, display_);
    lv_indev_set_user_data(pointer_, this);
    lv_indev_set_read_cb(pointer_, input);
}
SdlPlatform::~SdlPlatform() {
    lv_indev_delete(pointer_);
    lv_display_delete(display_);
    SDL_DestroyTexture(texture_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}
void SdlPlatform::flush(lv_display_t* display, const lv_area_t* area, uint8_t* data) {
    auto& self = *static_cast<SdlPlatform*>(lv_display_get_user_data(display));
    int width = area->x2-area->x1+1;
    for(int y=area->y1; y<=area->y2; ++y)
        std::memcpy(self.pixels_.data()+y*1280+area->x1, data+(y-area->y1)*width*4, width*4);
    if(lv_display_flush_is_last(display)) {
        SDL_UpdateTexture(self.texture_, nullptr, self.pixels_.data(), 1280*4);
        SDL_RenderClear(self.renderer_);
        SDL_RenderCopy(self.renderer_, self.texture_, nullptr, nullptr);
        SDL_RenderPresent(self.renderer_);
    }
    lv_display_flush_ready(display);
}
void SdlPlatform::input(lv_indev_t* device, lv_indev_data_t* data) {
    auto& self = *static_cast<SdlPlatform*>(lv_indev_get_user_data(device));
    data->point.x = self.x_; data->point.y = self.y_;
    data->state = self.pressed_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
bool SdlPlatform::poll() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) return false;
        if(event.type == SDL_MOUSEMOTION) { x_=event.motion.x; y_=event.motion.y; }
        if((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_LEFT) {
            x_=event.button.x; y_=event.button.y; pressed_=event.type == SDL_MOUSEBUTTONDOWN;
        }
    }
    return true;
}
void SdlPlatform::screenshot(const std::string& path) {
    lv_refr_now(display_);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels_.data(), 1280, 720, 32, 1280*4, SDL_PIXELFORMAT_XRGB8888);
    if(!surface) throw std::runtime_error(SDL_GetError());
    int result = SDL_SaveBMP(surface, path.c_str());
    SDL_FreeSurface(surface);
    if(result != 0) throw std::runtime_error(SDL_GetError());
}
}
