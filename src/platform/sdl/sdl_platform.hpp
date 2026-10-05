#pragma once
#include <SDL.h>
#include "platform/platform.hpp"
#include <lvgl.h>
#include <string>
#include <vector>
namespace panel {
class SdlPlatform final:public Platform {
public:
    SdlPlatform();
    ~SdlPlatform() override;
    bool poll() override;
    void screenshot(const std::string& path) override;
private:
    static void flush(lv_display_t*, const lv_area_t*, uint8_t*);
    static void input(lv_indev_t*, lv_indev_data_t*);
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    lv_display_t* display_ = nullptr;
    lv_indev_t* pointer_ = nullptr;
    std::vector<uint32_t> pixels_, draw_buffer_;
    int x_ = 0, y_ = 0;
    bool pressed_ = false;
};
}
