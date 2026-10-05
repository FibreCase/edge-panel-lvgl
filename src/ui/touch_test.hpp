#pragma once
#include <lvgl.h>
#include <array>
namespace panel {
class TouchTest {
public:
    TouchTest() {
        root_=lv_screen_active();
        lv_obj_set_style_bg_color(root_,lv_color_hex(0x101713),0);
        lv_obj_set_scrollable(root_,false);
        status_=lv_label_create(root_);
        lv_obj_set_style_text_font(status_,&lv_font_montserrat_24,0);
        lv_obj_set_style_text_color(status_,lv_color_hex(0xE5EEE8),0);
        lv_obj_align(status_,LV_ALIGN_TOP_MID,0,130);
        const int positions[5][2]={{0,0},{1184,0},{0,624},{1184,624},{592,312}};
        for(size_t i=0;i<targets_.size();++i) {
            auto* button=lv_button_create(root_);targets_[i]=button;
            lv_obj_set_pos(button,positions[i][0],positions[i][1]);
            lv_obj_set_size(button,96,96);
            lv_obj_set_style_bg_color(button,lv_color_hex(0x385C49),0);
            auto* label=lv_label_create(button);
            lv_label_set_text_fmt(label,"%zu",i+1);
            lv_obj_set_style_text_font(label,&lv_font_montserrat_24,0);
            lv_obj_center(label);
            lv_obj_add_event_cb(button,clicked,LV_EVENT_CLICKED,this);
        }
        tick();
    }
    ~TouchTest() { lv_obj_clean(root_); }
    void tick() {
        lv_point_t point{};bool down=false;
        auto* input=lv_indev_get_next(nullptr);
        if(input) { lv_indev_get_point(input,&point);down=lv_indev_get_state(input)==LV_INDEV_STATE_PRESSED; }
        unsigned count=0;for(bool hit:hits_) count+=hit;
        lv_label_set_text_fmt(status_,"Touch all 5 targets: %u / 5\nLogical position: %d, %d (%s)",count,point.x,point.y,down?"pressed":"released");
    }
private:
    static void clicked(lv_event_t* event) {
        auto& self=*static_cast<TouchTest*>(lv_event_get_user_data(event));
        auto* target=static_cast<lv_obj_t*>(lv_event_get_current_target(event));
        for(size_t i=0;i<self.targets_.size();++i) if(target==self.targets_[i]) {
            self.hits_[i]=true;lv_obj_set_style_bg_color(target,lv_color_hex(0x527C40),0);
        }
        self.tick();
    }
    lv_obj_t *root_,*status_;
    std::array<lv_obj_t*,5> targets_{};
    std::array<bool,5> hits_{};
};
}
