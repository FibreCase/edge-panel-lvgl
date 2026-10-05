#pragma once
#include <lvgl.h>
#include "services/mock_provider.hpp"
#include "ui/theme/color_scheme.hpp"
#include <map>
#include <string>
#include <vector>
namespace panel {
class Panel {
public:
    Panel(MockProvider& provider, const std::string& font_path);
    ~Panel();
    void tick();
    void self_test();
    void select_message(size_t index);
    // 固定种子色相并停止随时间的轮换，供测试与截图复现使用。
    void set_seed_hue(double hue);
private:
    // 界面对象与配色角色的绑定，配色变化时按角色重新着色。
    enum class Target { Background, Text };
    struct Binding { lv_obj_t* obj; theme::Rgb theme::ColorScheme::* role; Target target; };
    const lv_font_t* font(int size);
    lv_obj_t* box(lv_obj_t* parent, int x, int y, int w, int h, theme::Rgb theme::ColorScheme::* role, int radius = 24);
    lv_obj_t* label(lv_obj_t* parent, const char* text, int x, int y, int w, int size, theme::Rgb theme::ColorScheme::* role);
    lv_obj_t* button(const char* text, int x, int w, lv_event_cb_t callback);
    void bind(lv_obj_t* obj, theme::Rgb theme::ColorScheme::* role, Target target);
    void update_scheme(double hue);
    void apply_scheme();
    void regenerate_image();
    void render();
    static void next(lv_event_t*);
    static void refresh(lv_event_t*);
    static void connection(lv_event_t*);
    MockProvider& provider_;
    std::string font_path_;
    std::map<int, lv_font_t*> fonts_;
    double seed_hue_ = 0.0;
    theme::ColorScheme scheme_;
    std::vector<Binding> bindings_;
    std::vector<lv_obj_t*> buttons_;
    bool rotate_ = true;
    int last_minute_ = -1;
    lv_obj_t *root_, *clock_, *date_, *temperature_, *kind_, *body_, *source_, *counter_, *image_, *refresh_button_, *connection_button_, *next_button_;
    lv_image_dsc_t image_descriptor_{};
    std::vector<uint32_t> image_pixels_;
    size_t selected_ = 0;
    std::string last_time_;
};
}
