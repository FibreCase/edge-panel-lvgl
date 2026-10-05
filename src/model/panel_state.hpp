#pragma once
#include <string>
#include <vector>
namespace panel {
enum class MessageType { Text, Image, Notify };
struct Message { int id; MessageType type; std::string content, source, time; };
struct Weather { std::string description, temperature, rain, aqi, category; };
struct Event { std::string name, date, time, location; };
struct PanelState {
    Weather weather;
    Event event;
    std::vector<Message> messages;
    bool online = true;
    unsigned revision = 0;
};
}
