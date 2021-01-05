#ifndef DUI_SCROLLBAR_HPP_
#define DUI_SCROLLBAR_HPP_

#include <algorithm>
#include <optional>
#include <SDL.h>
#include "Button.hpp"
#include "Group.hpp"
#include "ScrollBarStyle.hpp"
#include "Target.hpp"

namespace dui {

/// Returns delta
inline std::optional<int>
scrollBarSliderCaret(Target target,
                     std::string_view id,
                     const SDL_Rect& r,
                     const BoxStyle& style = themeFor<Box>())
{
  box(target, r, style);
  static int lastPos;
  auto action = target.checkMouse(id, r);
  if (action == MouseAction::GRAB || action == MouseAction::HOLD) {
    lastPos = r.x;
    return {0};
  }
  if (action != MouseAction::DRAG) {
    return {};
  }
  auto pos = target.lastMousePos().x;
  int delta = pos - lastPos;
  lastPos += delta;
  if (delta > 0 ? pos < r.x : pos > r.x) {
    return {0};
  }
  return delta;
}

inline bool
scrollBarSlider(Target target,
                std::string_view id,
                int* value,
                int min,
                int max,
                const SDL_Rect& r,
                const ScrollBarSliderStyle& style = themeFor<ScrollBarSlider>())
{
  SDL_assert(value != nullptr);
  auto g = panel(target, id, r, Layout::NONE, style.panel);

  // TODO handle min>=max
  int distance = max - min;
  int cursorWidth = std::max(r.w / distance, style.minCursor);
  int cursorMax = r.w - cursorWidth;
  int cursorPos =
    std::clamp((*value - min) * cursorMax / distance, 0, cursorMax);
  SDL_Rect boxRect{cursorPos - 1, -1, cursorWidth, r.h};
  if (auto result = scrollBarSliderCaret(g, "caret", boxRect, style.cursor)) {
    int delta = *result * distance / cursorMax;
    if (delta == 0) {
      return false;
    }
    *value = std::clamp(*value + delta, min, max);
    return true;
  }
  g.end();
  auto action = target.checkMouse(id, r);
  if (action != MouseAction::ACTION) {
    return false;
  }
  SDL_Point mPos = target.lastMousePos();
  int delta = std::max(distance / 8, 1);
  if (mPos.x - r.x < boxRect.x) {
    if (*value == min) {
      return false;
    }
    if (*value - delta <= min) {
      *value = min;
    } else {
      *value -= delta;
    }
  } else {
    if (*value == max) {
      return false;
    }
    if (*value + delta >= max) {
      *value = max;
    } else {
      *value += delta;
    }
  }
  return true;
}

inline bool
scrollBar(Target target,
          std::string_view id,
          int* value,
          int min,
          int max,
          SDL_Rect r = {0},
          const ScrollBarStyle& style = themeFor<ScrollBar>())
{
  if (r.w == 0) {
    r.w = makeInputSize({r.w, r.h},
                        style.buttons.font,
                        0,
                        style.buttons.border + style.buttons.padding)
            .x;
  }
  auto& buttonStyle = style.buttons;
  int buttonWidth = buttonStyle.padding.left + buttonStyle.padding.right +
                    buttonStyle.border.left + buttonStyle.border.right + 8;
  if (r.w < buttonWidth * 4) {
    r.w = buttonWidth * 4;
  }
  int buttonHeight = buttonStyle.padding.top + buttonStyle.padding.bottom +
                     buttonStyle.border.top + buttonStyle.border.bottom + 8;
  if (buttonHeight < r.h) {
    buttonHeight = r.h;
  } else {
    r.h = buttonHeight;
  }
  bool action = false;
  Group g = group(target, id, r, Layout::NONE);
  if (button(g, "prev", "<")) {
    *value = *value > min ? *value - 1 : min;
    action = true;
  }
  if (button(g, "next", ">", {r.w - buttonWidth, 0})) {
    *value = *value < max ? *value + 1 : max;
    action = true;
  }
  action |= scrollBarSlider(
    g,
    "bar",
    value,
    min,
    max,
    {buttonWidth - 1, 0, r.w - buttonWidth * 2 + 2, buttonHeight},
    style.bar);
  return action;
}

} // namespace dui

#endif // DUI_SCROLLBAR_HPP_