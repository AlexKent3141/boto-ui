#ifndef DUI_TARGET_HPP_
#define DUI_TARGET_HPP_

#include <SDL.h>
#include "State.hpp"
#include "TargetStyle.hpp"

namespace dui {

constexpr int
makeLen(int len, int delta, bool autoPos, int elementSpacing)
{
  if (len == 0) {
    len = delta;
    if (autoPos && len >= elementSpacing) {
      len -= elementSpacing;
    }
  }
  return len;
}

/**
 * @brief
 *
 */
class Target
{
  State* state;
  std::string_view id;
  SDL_Rect* rect = nullptr;
  SDL_Point* topLeft = nullptr;
  SDL_Point* bottomRight = nullptr;
  bool* locked = nullptr;
  TargetStyle style;

public:
  Target()
    : state(nullptr)
  {}

  Target(State* state,
         std::string_view id,
         SDL_Rect& rect,
         SDL_Point& topLeft,
         SDL_Point& bottomRight,
         bool& locked,
         TargetStyle style)
    : state(state)
    , id(id)
    , rect(&rect)
    , topLeft(&topLeft)
    , bottomRight(&bottomRight)
    , locked(&locked)
    , style(style)
  {}

  /**
   * @brief Check the mouse action/status for element in this group
   *
   * @param id element id
   * @param r the element local rect (Use State.checkMouse() for global rect)
   * @return MouseAction
   */
  MouseAction checkMouse(std::string_view id, SDL_Rect r);

  /**
   * @brief Check if given contained element is active
   *
   * @param id the id to check
   * @return true
   * @return false
   */
  bool isActive(std::string_view id) const { return state->isActive(id); }

  /**
   * @brief Check the text action/status for element in this group
   *
   * @param id the element id
   * @return TextAction
   */
  TextAction checkText(std::string_view id) const
  {
    return state->checkText(id);
  }

  /**
   * @brief Get the last input text
   *
   * To check if the text was for the current element and frame, use checkText()
   * or Frame.checkText().
   *
   * @return std::string_view
   */
  std::string_view lastText() const { return state->lastText(); }
  /**
   * @brief Get the last key down
   *
   * To check if the text was for the current element and frame, use checkText()
   * or Frame.checkText().
   *
   * @return std::string_view
   */
  SDL_Keysym lastKeyDown() const { return state->lastKeyDown(); }

  /**
   * @brief Last mouse position
   *
   * @return SDL_Point the last mouse pos
   */
  SDL_Point lastMousePos() const
  {
    auto pos = state->lastMousePos();
    pos.x -= topLeft->x;
    pos.y -= topLeft->y;
    return pos;
  }

  /**
   * @brief Advances the caret with the given offset
   *
   * @param p a point where x and y are the horizontal and vertical offsets,
   * respectively.
   *
   * The new caret will also depend on the layout:
   * - VERTICAL layout means only the y is updated so elements are positioned
   * vertically;
   * - HORIZONTAL layout means only the x is updated so elements are positioned
   * horizontally;
   * - NONE layout means none are updated and elements all begin in the same
   * position;
   *
   * Keep in mind that elements have their own offset (their rect.x and rect.y),
   * that is added to the caret ones.
   */
  void advance(const SDL_Point& p);

  State& getState() const { return *state; }

  SDL_Point getCaret() const
  {
    auto caret = *topLeft;
    if (style.layout == Layout::VERTICAL) {
      caret.y = bottomRight->y;
    } else if (style.layout == Layout::HORIZONTAL) {
      caret.x = bottomRight->x;
    }
    return caret;
  }

  bool isLocked() const { return *locked; }

  Layout getLayout() const { return style.layout; }

  const SDL_Rect& getRect() const { return *rect; }

  SDL_Point size() const { return {width(), height()}; }

  int width() const
  {
    return makeLen(rect->w,
                   bottomRight->x - topLeft->x,
                   style.layout == Layout::HORIZONTAL,
                   style.elementSpacing);
  }

  int contentWidth() const { return bottomRight->x - topLeft->x; }

  int height() const
  {
    return makeLen(rect->h,
                   bottomRight->y - topLeft->y,
                   style.layout == Layout::VERTICAL,
                   style.elementSpacing);
  }

  int contentHeight() const { return bottomRight->y - topLeft->y; }

  void lock(std::string_view id, SDL_Rect r)
  {
    SDL_assert(!*locked);
    *locked = true;
    auto caret = getCaret();
    r.x += caret.x;
    r.y += caret.y;
    state->beginGroup(id, r);
  }

  void unlock(std::string_view id, SDL_Rect r)
  {
    SDL_assert(*locked);
    auto caret = getCaret();
    r.x += caret.x;
    r.y += caret.y;
    state->endGroup(id, r);
    *locked = false;
  }

  operator bool() const { return state; }
};

inline MouseAction
Target::checkMouse(std::string_view id, SDL_Rect r)
{
  SDL_assert(!*locked);
  SDL_Point caret = getCaret();
  r.x += caret.x;
  r.y += caret.y;
  return state->checkMouse(id, r);
}

inline void
Target::advance(const SDL_Point& p)
{
  SDL_assert(!*locked);
  if (style.layout == Layout::VERTICAL) {
    bottomRight->x = std::max(p.x + topLeft->x, bottomRight->x);
    bottomRight->y += p.y + style.elementSpacing;
  } else if (style.layout == Layout::HORIZONTAL) {
    bottomRight->x += p.x + style.elementSpacing;
    bottomRight->y = std::max(p.y + topLeft->y, bottomRight->y);
  } else {
    bottomRight->x = std::max(p.x + topLeft->x, bottomRight->x);
    bottomRight->y = std::max(p.y + topLeft->y, bottomRight->y);
  }
}

template<class T>
struct Targetable
{
  Layout getLayout() const { return target().getLayout(); }

  SDL_Rect getRect() const { return target().getRect(); }

  SDL_Point size() const { return {source().width(), source().height()}; }

  int width() const { return target().width(); }

  int height() const { return target().height(); }

private:
  const T& source() const { return static_cast<const T&>(*this); }
  const Target target() const { return const_cast<T&>(source()); }
};

} // namespace dui

#endif // DUI_TARGET_HPP_
