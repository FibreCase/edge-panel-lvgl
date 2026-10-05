#pragma once
#include "model/panel_state.hpp"
namespace panel {
class MockProvider {
public:
    MockProvider();
    const PanelState& state() const { return state_; }
    void refresh();
    void toggle_connection();
private:
    PanelState state_;
};
}
