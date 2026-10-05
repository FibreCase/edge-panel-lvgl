#include "services/mock_provider.hpp"
namespace panel {
MockProvider::MockProvider() {
    state_.weather = {"晴间多云", "24", "未来两小时无降水", "42", "优"};
    state_.event = {"风力发电场电气设计", "2026-10-05", "18:00", "主楼 B412"};
    state_.messages = {
        {1, MessageType::Text, "今天也留一点时间，给生活。\n记得带上水杯，出门走走。", "留言", "14:20"},
        {2, MessageType::Image, "山间的片刻", "图片", "13:45"},
        {3, MessageType::Notify, "备份已完成，所有文件已同步。", "家庭服务器", "13:30"}
    };
}
void MockProvider::refresh() {
    if(!state_.online) return;
    ++state_.revision;
    state_.weather.temperature = state_.revision % 2 ? "25" : "24";
}
void MockProvider::toggle_connection() { state_.online = !state_.online; }
}
