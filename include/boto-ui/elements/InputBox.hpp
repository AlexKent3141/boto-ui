#ifndef BOTO_INPUTBOX_HPP
#define BOTO_INPUTBOX_HPP

#include <string_view>
#include "Control.hpp"
#include "Group.hpp"
#include "Panel.hpp"

namespace boto {

/// Eval the input size, accordingly to parameters
inline SDL_Point
makeInputSize(SDL_Point defaultSz,
              const Font& font,
              int scale,
              const EdgeSize& padding)
{
  if (defaultSz.x != 0 && defaultSz.y != 0) {
    return defaultSz;
  }
  // TODO allow customization for this
  auto clientSz = measure('m', font, scale);
  clientSz.x *= 16;

  auto elementSz = elementSize(padding, clientSz);

  if (defaultSz.x == 0) {
    defaultSz.x = elementSz.x;
  }
  if (defaultSz.y == 0) {
    defaultSz.y = elementSz.y;
  }
  return defaultSz;
}

/// Eval the input rec, accordingly to parameters
inline SDL_Rect
makeInputRect(SDL_Rect r, const ControlStyle& style)
{
  auto sz = makeInputSize({r.w, r.h},
                          style.text.font,
                          style.text.scale,
                          style.padding + style.decoration.border);
  return {r.x, r.y, sz.x, sz.y};
}

/// Represents the changes over the input text to an input box
struct TextChange
{
  std::string_view insert; ///< Text to be inserted
  size_t index;            ///< start position
  size_t erase;            ///< number of bytes to dele before inserting
};

/// Base for input boxes
inline TextChange
textBoxBase(Target target,
            std::string_view id,
            std::string_view value,
            SDL_Rect r,
            const InputBoxStyle& style)
{
  static size_t cursorPos = 0;
  static size_t maxPos = 0;
  r = makeInputRect(r, style.normal);
  auto g =
    panel(target, id, r, {style.normal.decoration, {0}, {0, Layout::NONE}});
  auto& state = g.state().eventTarget.state();
  if (state.event == Event::GRAB) {
    maxPos = cursorPos = value.size();
  }

  bool active = state.status.test(Status::FOCUSED);
  if (active && cursorPos > value.size()) {
    maxPos = cursorPos = value.size();
  }
  auto& currentStyle = active ? style.active : style.normal;

  // This creates an auto scroll effect if value text don't fit in the box;
  auto clientSz =
    clientSize(currentStyle.padding + EdgeSize::all(1), {r.w, r.h});
  auto contentSz =
    measure(value, currentStyle.text.font, currentStyle.text.scale);
  int deltaX = contentSz.x - clientSz.x;
  if (deltaX < 0) {
    deltaX = 0;
  } else if (active && deltaX + 8 > int(cursorPos) * 8) {
    // TODO Use proper scrolling here
    deltaX = cursorPos * 8;
    if (deltaX > 8) {
      deltaX -= 8;
    } else {
      deltaX = 0;
    }
  }
  text(g, value, {-deltaX, 0}, currentStyle.text);
  if (!active) {
    return {};
  }

  if ((target.ticks() / 512) % 2) {
    // Show cursor
    element(g,
            {int(cursorPos) * 8 - deltaX, 0, 1, clientSz.y},
            currentStyle.text.color);
  }
  if (state.event == Event::INPUT) {
    auto insert = target.input();
    auto index = cursorPos;
    cursorPos += insert.size();
    maxPos += insert.size();
    return {insert, index, 0};
  }
  if (state.event == Event::BACKSPACE && !value.empty()) {
    cursorPos -= 1;
    maxPos -= 1;
    return {{}, cursorPos, 1};
  }
  return {};
}

/// A text box
/// @ingroup elements
inline bool
textBox(Target target,
        std::string_view id,
        char* value,
        size_t maxSize,
        const SDL_Rect& r,
        const InputBoxStyle& style)
{
  auto len = strlen(value);
  auto change = textBoxBase(target, id, {value, len}, r, style);
  if (change.erase == 0 && change.insert.empty()) {
    return false;
  }
  int offset = int(change.insert.size()) - int(change.erase);
  if (offset != 0) {
    size_t target = change.index + change.insert.size();
    int maxCount = maxSize - target;
    if (maxCount > 0) {
      size_t source = target - offset;
      SDL_memmove(&value[target],
                  &value[source],
                  std::min(len - source + 1, size_t(maxCount)));
    }
  }
  if (!change.insert.empty()) {
    int maxCount = maxSize - 1 - change.index;
    if (maxCount > 0) {
      SDL_memcpy(&value[change.index],
                 change.insert.data(),
                 std::min(change.insert.size(), size_t(maxCount)));
    }
  }
  return true;
}
inline bool
textBox(Target target,
        std::string_view id,
        char* value,
        size_t maxSize,
        const SDL_Rect& r = {0})
{
  return textBox(target, id, value, maxSize, r, target.styleFor<TextBox>());
}

/// A text box
/// @ingroup elements
inline bool
textBox(Target target,
        std::string_view id,
        std::string* value,
        const SDL_Rect& r = {0})
{
  auto change = textBoxBase(target, id, *value, r, target.styleFor<TextBox>());
  if (change.erase == 0 && change.insert.empty()) {
    return false;
  }
  value->replace(change.index, change.erase, change.insert);
  return true;
}

/// Base class for input boxes not backed by strings
class BufferedInputBox
{
  Target target;
  std::string_view id;
  SDL_Rect rect;
  const InputBoxStyle& style;

  bool active;
  bool refillBuffer;

public:
  /// Amount to increment the backing value by
  int incAmount = 0;
  static constexpr int BUF_SZ = 256; ///< Buffer size
  char buffer[BUF_SZ];               ///< Buffer

  /// Ctor
  BufferedInputBox(Target target,
                   std::string_view id,
                   SDL_Rect r,
                   const InputBoxStyle& style)
    : target(target)
    , id(id)
    , rect(makeInputRect(r, style.normal))
    , style(style)
  {
    auto& state = target.check(id, rect, boto::RequestEvent::INPUT);
    active = state.status == Status::FOCUSED;
    refillBuffer = !active || state.event == Event::GRAB;
  }

  /// If true you must convert the backing value to string and fill this' buffer
  bool wantsRefill() const { return refillBuffer; }

  /**
   * @brief Finished processing and return if content changed
   *
   * @return true content changed
   * @return false content not changed
   */
  bool end()
  {
    if (!active) {
      textBox(target, id, buffer, BUF_SZ, rect, style);
      return false;
    }
    static char editBuffer[BUF_SZ];
    if (refillBuffer) {
      SDL_strlcpy(editBuffer, buffer, BUF_SZ);
    }
    if (!textBox(target, id, editBuffer, BUF_SZ, rect, style)) {
      return incAmount != 0;
    }
    SDL_strlcpy(buffer, editBuffer, BUF_SZ);
    return true;
  }
};

/// An int Box
/// @ingroup elements
inline bool
numberBox(Target target, std::string_view id, int* value, SDL_Rect r = {0})
{
  SDL_assert(value != nullptr);
  BufferedInputBox bufferedBox{target, id, r, target.styleFor<IntBox>()};
  if (bufferedBox.wantsRefill()) {
    if (bufferedBox.incAmount != 0) {
      *value += bufferedBox.incAmount;
    }
    SDL_itoa(*value, bufferedBox.buffer, 10);
  }
  if (bufferedBox.end()) {
    auto newValue = SDL_atoi(bufferedBox.buffer);
    if (newValue != *value) {
      *value = newValue;
      return true;
    }
  }
  return false;
}

/// A double box
/// @ingroup elements
inline bool
numberBox(Target target, std::string_view id, double* value, SDL_Rect r = {0})
{
  SDL_assert(value != nullptr);
  BufferedInputBox bufferedBox{target, id, r, target.styleFor<DoubleBox>()};
  if (bufferedBox.wantsRefill()) {
    if (bufferedBox.incAmount != 0) {
      *value += bufferedBox.incAmount;
    }
    auto text = bufferedBox.buffer;
    int n = SDL_snprintf(text, bufferedBox.BUF_SZ, "%f", *value);
    for (int i = n - 1; i > 0; --i) {
      if (text[i] != '0' && text[i] != '.') {
        text[i + 1] = 0;
        break;
      }
    }
  }
  if (bufferedBox.end()) {
    auto newValue = SDL_strtod(bufferedBox.buffer, nullptr);
    if (newValue != *value) {
      *value = newValue;
      return true;
    }
  }
  return false;
}
/// A float box
/// @ingroup elements
inline bool
numberBox(Target target, std::string_view id, float* value, SDL_Rect r = {0})
{
  SDL_assert(value != nullptr);
  BufferedInputBox bufferedBox{target, id, r, target.styleFor<FloatBox>()};
  if (bufferedBox.wantsRefill()) {
    if (bufferedBox.incAmount != 0) {
      *value += bufferedBox.incAmount;
    }
    auto text = bufferedBox.buffer;
    int n = SDL_snprintf(text, bufferedBox.BUF_SZ, "%f", *value);
    for (int i = n - 1; i > 0; --i) {
      if (text[i] != '0' && text[i] != '.') {
        text[i + 1] = 0;
        break;
      }
    }
  }
  if (bufferedBox.end()) {
    auto newValue = SDL_strtod(bufferedBox.buffer, nullptr);
    if (newValue != *value) {
      *value = newValue;
      return true;
    }
  }
  return false;
}
} // namespace boto

#endif // BOTO_INPUTBOX_HPP
