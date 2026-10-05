#include "ui/panel.hpp"
#include <ctime>
#include <stdexcept>
namespace panel {
Panel::Panel(MockProvider& provider, const std::string& font_path)
    : provider_(provider), font_path_(font_path), scheme_(theme::dark_scheme_from_seed_hue(seed_hue_)),
      image_pixels_(696*164) {
    root_=lv_screen_active();
    lv_obj_remove_style_all(root_);
    lv_obj_set_style_bg_opa(root_,LV_OPA_COVER,0);
    lv_obj_set_scrollable(root_,false);
    bind(root_,&theme::ColorScheme::surface,Target::Background);
    auto* time_card=box(root_,32,28,432,278,&theme::ColorScheme::primary_container);
    label(time_card,"此刻，慢一点",28,22,360,18,&theme::ColorScheme::on_primary_container);
    clock_=label(time_card,"14:30",22,73,400,92,&theme::ColorScheme::on_primary_container);
    date_=label(time_card,"",28,209,380,20,&theme::ColorScheme::on_primary_container);
    auto* weather_card=box(root_,32,322,432,296,&theme::ColorScheme::surface_container_high);
    label(weather_card,"今日天气 / 北京",28,22,370,18,&theme::ColorScheme::on_surface_variant);
    temperature_=label(weather_card,"",25,60,220,62,&theme::ColorScheme::on_surface);
    label(weather_card,provider.state().weather.description.c_str(),236,96,180,24,&theme::ColorScheme::on_surface);
    label(weather_card,provider.state().weather.rain.c_str(),28,180,376,20,&theme::ColorScheme::on_surface);
    auto* aqi=box(weather_card,28,233,376,40,&theme::ColorScheme::surface_container_highest,12);
    const auto& weather=provider.state().weather;
    auto aqi_text="空气质量  "+weather.aqi+" · "+weather.category;
    label(aqi,aqi_text.c_str(),16,8,330,17,&theme::ColorScheme::on_surface_variant);
    auto* event_card=box(root_,480,28,768,194,&theme::ColorScheme::surface_container_high);
    label(event_card,"下一件事",28,22,510,18,&theme::ColorScheme::on_surface_variant);
    label(event_card,provider.state().event.time.c_str(),596,18,150,32,&theme::ColorScheme::on_surface);
    label(event_card,provider.state().event.name.c_str(),28,71,650,30,&theme::ColorScheme::on_surface);
    auto event_detail=provider.state().event.date+"  /  "+provider.state().event.location;
    label(event_card,event_detail.c_str(),28,139,590,18,&theme::ColorScheme::on_surface_variant);
    auto* tag=box(event_card,628,141,112,30,&theme::ColorScheme::surface_container_highest,10);
    label(tag,"示例日程",14,5,100,15,&theme::ColorScheme::on_surface_variant);
    auto* message_card=box(root_,480,238,768,380,&theme::ColorScheme::surface_container_low);
    kind_=label(message_card,"",28,24,590,18,&theme::ColorScheme::primary);
    counter_=label(message_card,"",654,24,88,17,&theme::ColorScheme::primary);
    body_=label(message_card,"",28,91,696,30,&theme::ColorScheme::on_surface);
    lv_obj_set_style_text_line_space(body_,14,0);
    lv_obj_set_height(body_,180);
    source_=label(message_card,"",28,322,696,17,&theme::ColorScheme::on_surface_variant);
    // Locally generated mock image: no external image dependency.
    image_=lv_image_create(message_card);
    image_descriptor_.header.magic=LV_IMAGE_HEADER_MAGIC;
    image_descriptor_.header.cf=LV_COLOR_FORMAT_XRGB8888;
    image_descriptor_.header.w=696; image_descriptor_.header.h=164;
    image_descriptor_.header.stride=696*4;
    image_descriptor_.data_size=image_pixels_.size()*4;
    image_descriptor_.data=reinterpret_cast<const uint8_t*>(image_pixels_.data());
    lv_image_set_src(image_,&image_descriptor_);
    lv_obj_set_pos(image_,28,81);
    lv_obj_set_style_radius(image_,16,0);
    label(root_,"让信息安静地待在身边。",32,655,440,18,&theme::ColorScheme::on_surface_variant);
    refresh_button_=button("刷新示例",480,166,refresh);
    connection_button_=button("切换连接",662,166,connection);
    next_button_=button("下一条消息  →",844,224,next);
    label(root_,"MOCK",1092,655,156,16,&theme::ColorScheme::outline);
    apply_scheme();
    render(); tick();
}
Panel::~Panel() {
    lv_obj_clean(root_);
    for(auto& item:fonts_) lv_freetype_font_delete(item.second);
}
const lv_font_t* Panel::font(int size) {
    auto found=fonts_.find(size);
    if(found!=fonts_.end()) return found->second;
    auto* value=lv_freetype_font_create(font_path_.c_str(), LV_FREETYPE_FONT_RENDER_MODE_BITMAP, size, LV_FREETYPE_FONT_STYLE_NORMAL);
    if(!value) throw std::runtime_error("Cannot load Chinese font: "+font_path_);
    fonts_[size]=value;
    return value;
}
void Panel::bind(lv_obj_t* obj, theme::Rgb theme::ColorScheme::* role, Target target) {
    bindings_.push_back({obj,role,target});
}
lv_obj_t* Panel::box(lv_obj_t* parent, int x, int y, int w, int h, theme::Rgb theme::ColorScheme::* role, int radius) {
    auto* obj=lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj,x,y); lv_obj_set_size(obj,w,h);
    lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    lv_obj_set_style_radius(obj,radius,0);
    lv_obj_set_scrollable(obj,false);
    bind(obj,role,Target::Background);
    return obj;
}
lv_obj_t* Panel::label(lv_obj_t* parent, const char* text, int x, int y, int w, int size, theme::Rgb theme::ColorScheme::* role) {
    auto* obj=lv_label_create(parent);
    lv_label_set_text(obj,text); lv_obj_set_pos(obj,x,y); lv_obj_set_width(obj,w);
    lv_obj_set_style_text_font(obj,font(size),0);
    bind(obj,role,Target::Text);
    return obj;
}
lv_obj_t* Panel::button(const char* text, int x, int w, lv_event_cb_t callback) {
    auto* obj=box(root_,x,638,w,54,&theme::ColorScheme::secondary_container,16);
    lv_obj_set_clickable(obj,true);
    auto* title=label(obj,text,0,0,w,18,&theme::ColorScheme::on_secondary_container);
    lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_center(title);
    lv_obj_add_event_cb(obj,callback,LV_EVENT_CLICKED,this);
    buttons_.push_back(obj);
    return obj;
}
void Panel::update_scheme(double hue) {
    seed_hue_=hue;
    scheme_=theme::dark_scheme_from_seed_hue(hue);
    apply_scheme();
}
void Panel::set_seed_hue(double hue) {
    rotate_=false;
    update_scheme(hue);
}
void Panel::apply_scheme() {
    for(const auto& binding:bindings_) {
        lv_color_t color=lv_color_hex(scheme_.*binding.role);
        if(binding.target==Target::Text) lv_obj_set_style_text_color(binding.obj,color,0);
        else lv_obj_set_style_bg_color(binding.obj,color,0);
    }
    // 填充色调按钮的按下态：MD3 状态层为 onSecondaryContainer 以 12% 叠加。
    lv_color_t pressed=lv_color_hex(theme::blend(scheme_.secondary_container,scheme_.on_secondary_container,0.12));
    for(lv_obj_t* button:buttons_) lv_obj_set_style_bg_color(button,pressed,LV_STATE_PRESSED);
    regenerate_image();
}
void Panel::regenerate_image() {
    // 示例风景跟随配色：天空取反色表面并偏向主色，山脊由次色/主色向表面压暗。
    theme::Rgb sky=theme::blend(scheme_.inverse_surface,scheme_.primary,0.30);
    theme::Rgb haze=theme::blend(scheme_.secondary,scheme_.surface,0.35);
    theme::Rgb ridge_far=theme::blend(scheme_.secondary,scheme_.surface,0.45);
    theme::Rgb ridge_near=theme::blend(scheme_.primary,scheme_.surface,0.55);
    theme::Rgb sun=scheme_.tertiary;
    for(int y=0;y<164;++y) for(int x=0;x<696;++x) {
        theme::Rgb color=y<110 ? sky : haze;
        if((x-540)*(x-540)+(y-42)*(y-42)<22*22) color=sun;
        if(y>112-(x%230)/3) color=ridge_far;
        if(y>142-((x+70)%280)/4) color=ridge_near;
        image_pixels_[y*696+x]=color;
    }
    // 内置解码器直接引用 image_pixels_，不复制数据，因此原地改写后重绘即可。
    lv_obj_invalidate(image_);
}
void Panel::render() {
    const auto& state=provider_.state();
    std::string temp=state.weather.temperature+"°";
    lv_label_set_text(temperature_,temp.c_str());
    const auto& message=state.messages.at(selected_);
    const char* kind=message.type==MessageType::Text ? "留言" : message.type==MessageType::Image ? "图片" : "应用通知";
    lv_label_set_text(kind_,kind);
    lv_label_set_text_fmt(counter_,"%zu / %zu",selected_+1,state.messages.size());
    lv_label_set_text(body_,message.content.c_str());
    bool is_image=message.type==MessageType::Image;
    if(is_image) lv_obj_set_hidden(image_,false);
    else lv_obj_set_hidden(image_,true);
    lv_obj_set_y(body_,is_image ? 267 : 103);
    lv_obj_set_height(body_,is_image ? 40 : 180);
    lv_obj_set_style_text_font(body_,font(is_image ? 22 : 30),0);
    std::string source=message.source+"  /  "+message.time+(state.online ? "" : "  ·  离线缓存");
    lv_label_set_text(source_,source.c_str());
}
void Panel::tick() {
    std::time_t now=std::time(nullptr);
    std::tm local{}; localtime_r(&now,&local);
    char time[16]; std::strftime(time,sizeof(time),"%H:%M",&local);
    if(last_time_!=time) {
        last_time_=time; lv_label_set_text(clock_,time);
        static const char* weekdays[]={"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
        lv_label_set_text_fmt(date_,"%d 年 %02d 月 %02d 日 · %s",local.tm_year+1900,local.tm_mon+1,local.tm_mday,weekdays[local.tm_wday]);
    }
    // 种子色相每分钟前进 6°，一小时转满整个色环。
    if(rotate_ && local.tm_min!=last_minute_) {
        last_minute_=local.tm_min;
        update_scheme(theme::seed_hue_at(now));
    }
}
void Panel::next(lv_event_t* e) {
    auto& self=*static_cast<Panel*>(lv_event_get_user_data(e));
    self.selected_=(self.selected_+1)%self.provider_.state().messages.size(); self.render();
}
void Panel::refresh(lv_event_t* e) {
    auto& self=*static_cast<Panel*>(lv_event_get_user_data(e)); self.provider_.refresh(); self.render();
}
void Panel::connection(lv_event_t* e) {
    auto& self=*static_cast<Panel*>(lv_event_get_user_data(e)); self.provider_.toggle_connection(); self.render();
}
void Panel::select_message(size_t index) {
    if(index>=provider_.state().messages.size()) throw std::out_of_range("Message index out of range");
    selected_=index; render();
}
void Panel::self_test() {
    auto check=[](bool condition) { if(!condition) throw std::runtime_error("Mock interaction test failed"); };
    lv_obj_send_event(next_button_,LV_EVENT_CLICKED,nullptr);
    check(selected_==1 && !lv_obj_is_hidden(image_));
    lv_obj_send_event(next_button_,LV_EVENT_CLICKED,nullptr); check(selected_==2);
    lv_obj_send_event(next_button_,LV_EVENT_CLICKED,nullptr); check(selected_==0);
    lv_obj_send_event(refresh_button_,LV_EVENT_CLICKED,nullptr); check(provider_.state().weather.temperature=="25");
    lv_obj_send_event(connection_button_,LV_EVENT_CLICKED,nullptr); check(!provider_.state().online);
    lv_obj_send_event(refresh_button_,LV_EVENT_CLICKED,nullptr); check(provider_.state().revision==1);
    lv_obj_send_event(connection_button_,LV_EVENT_CLICKED,nullptr); check(provider_.state().online);
    // 动态配色：种子色相改变时，配色与界面背景都应跟着改变，且背景始终等于 surface。
    const bool rotating=rotate_; const double hue=seed_hue_;
    set_seed_hue(0.0);
    theme::Rgb warm_primary=scheme_.primary;
    uint32_t warm_surface=lv_color_to_u32(lv_obj_get_style_bg_color(root_,LV_PART_MAIN))&0xFFFFFF;
    set_seed_hue(180.0);
    check(scheme_.primary!=warm_primary);
    uint32_t cool_surface=lv_color_to_u32(lv_obj_get_style_bg_color(root_,LV_PART_MAIN))&0xFFFFFF;
    check(cool_surface!=warm_surface);
    check(cool_surface==scheme_.surface);
    rotate_=rotating;
    update_scheme(hue);
}
}
