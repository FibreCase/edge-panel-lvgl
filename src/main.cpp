#ifdef EDGE_PANEL_BACKEND_DRM
#include "platform/linux/drm_platform.hpp"
#else
#include "platform/sdl/sdl_platform.hpp"
#endif
#include "ui/panel.hpp"
#include "ui/touch_test.hpp"
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
namespace {
volatile std::sig_atomic_t stopped=0;
void stop(int) { stopped=1; }
struct LvRuntime { LvRuntime() { lv_init(); } ~LvRuntime() { lv_deinit(); } };
uint32_t unsigned_option(const std::string& text) {
    if(text.empty()||text.find_first_not_of("0123456789")!=std::string::npos) throw std::invalid_argument("Expected an unsigned integer");
    auto value=std::stoull(text);
    if(value>std::numeric_limits<uint32_t>::max()) throw std::invalid_argument("Integer out of range");
    return static_cast<uint32_t>(value);
}
}
std::string find_font() {
    if(const char* path=std::getenv("EDGE_PANEL_FONT")) return path;
    for(const auto* path:{"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                         "/usr/share/fonts/google-noto-sans-cjk-vf-fonts/NotoSansCJK-VF.ttc"})
        if(std::filesystem::exists(path)) return path;
    throw std::runtime_error("Chinese font missing. Install fonts-noto-cjk or set EDGE_PANEL_FONT to a CJK TTF/TTC file.");
}
int main(int argc,char** argv) {
    bool test=false,touch_test=false,has_seed=false;
    double seed_hue=0.0;
    size_t message_index=0;
    std::string screenshot;
#ifdef EDGE_PANEL_BACKEND_DRM
    panel::DrmOptions options;
    if(const char* device=std::getenv("EDGE_PANEL_DRM_DEVICE")) options.device=device;
    if(const char* touch=std::getenv("EDGE_PANEL_TOUCH_DEVICE")) options.touch_device=touch;
#endif
    try {
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            auto value=[&]() -> std::string {
                if(i+1>=argc) throw std::invalid_argument("Missing value for "+arg);
                return argv[++i];
            };
            if(arg=="--self-test") test=true;
            else if(arg=="--touch-test") touch_test=true;
            else if(arg=="--screenshot") screenshot=value();
            else if(arg=="--seed-hue") {
                try {
                    auto text=value();size_t parsed=0;
                    seed_hue=std::stod(text,&parsed);
                    if(parsed!=text.size()||!std::isfinite(seed_hue)) throw std::invalid_argument("Invalid hue");
                } catch(const std::exception&) { throw std::invalid_argument("Seed hue must be a finite number in degrees"); }
                has_seed=true;
            }
            else if(arg=="--message") {
                message_index=unsigned_option(value());
                if(message_index>2) throw std::invalid_argument("Message index must be 0, 1 or 2");
            }
#ifdef EDGE_PANEL_BACKEND_DRM
            else if(arg=="--drm-device") options.device=value();
            else if(arg=="--touch-device") options.touch_device=value();
            else if(arg=="--connector") options.connector=unsigned_option(value());
            else if(arg=="--crtc") options.crtc=unsigned_option(value());
            else if(arg=="--rotation") {
                auto rotation=unsigned_option(value());
                if(rotation!=90&&rotation!=270) throw std::invalid_argument("Rotation must be 90 or 270");
                options.rotation=static_cast<int>(rotation);
            }
            else if(arg=="--no-touch") options.no_touch=true;
            else if(arg=="--touch-swap-xy") options.touch.swap=true;
            else if(arg=="--touch-invert-x") options.touch.invert_x=true;
            else if(arg=="--touch-invert-y") options.touch.invert_y=true;
            else if(arg=="--touch-space") {
                auto space=value();
                if(space!="physical"&&space!="logical") throw std::invalid_argument("Touch space must be physical or logical");
                options.touch.logical=space=="logical";
            }
#endif
            else if(arg=="--help") {
                std::cout<<"edge-panel [--message 0|1|2] [--seed-hue degrees] [--touch-test]\n";
#ifdef EDGE_PANEL_BACKEND_DRM
                std::cout<<"DRM: --drm-device /dev/dri/card0 --connector ID --crtc ID\n"
                           "     --rotation 90|270 --touch-device /dev/input/eventX | --no-touch\n"
                           "     --touch-swap-xy --touch-invert-x --touch-invert-y --touch-space physical|logical\n"
                           "Environment: EDGE_PANEL_DRM_DEVICE, EDGE_PANEL_TOUCH_DEVICE\n";
#else
                std::cout<<"SDL: --self-test --screenshot output.bmp\n";
#endif
                std::cout<<"Environment: EDGE_PANEL_FONT=/path/to/chinese.ttf\n";return 0;
            } else throw std::invalid_argument("Unknown option: "+arg);
        }
#ifdef EDGE_PANEL_BACKEND_DRM
        if(test||!screenshot.empty()) throw std::invalid_argument("Self-test and screenshots require SDL; use theme-test/coordinates-test for headless DRM checks");
        if(touch_test&&options.no_touch) throw std::invalid_argument("Touch test requires a touch device");
#else
        if(touch_test&&test) throw std::invalid_argument("Use touch-test and self-test separately");
#endif
        auto font=find_font();
        LvRuntime runtime;
#ifdef EDGE_PANEL_BACKEND_DRM
        panel::DrmPlatform platform(options);
#else
        panel::SdlPlatform platform;
#endif
        panel::MockProvider provider;
        std::unique_ptr<panel::Panel> ui;
        std::unique_ptr<panel::TouchTest> diagnostic;
        if(touch_test) diagnostic=std::make_unique<panel::TouchTest>();
        else {
            ui=std::make_unique<panel::Panel>(provider,font);
            if(test) { ui->self_test();std::cout<<"Mock interactions passed\n"; }
            if(has_seed) ui->set_seed_hue(seed_hue);
            ui->select_message(message_index);
        }
        if(!screenshot.empty()) platform.screenshot(screenshot);
        std::signal(SIGINT,stop);std::signal(SIGTERM,stop);
        if(!test&&screenshot.empty()) while(!stopped&&platform.poll()) {
            if(ui) ui->tick();else diagnostic->tick();
            auto delay=std::clamp(lv_timer_handler(),1u,10u);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
