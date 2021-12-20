#ifndef BOTO_CORE_EVENTDISPATCHER_HPP
#define BOTO_CORE_EVENTDISPATCHER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <SDL_assert.h>
#include <SDL_keycode.h>
#include <SDL_rect.h>
#include "core/Command.hpp"
#include "core/Event.hpp"
#include "core/Status.hpp"
#include "util/CookieBase.hpp"

namespace boto {
/**
 * @brief Separate group for its sub element
 *
 */
constexpr char groupNameSeparator = '/';

/**
 * @brief The state of an event target
 *
 */
struct EventTargetState
{
  size_t idLength;    ///< @brief just an aux. Ignore
  SDL_Rect rect;      ///< @brief The event target area (absolute)
  StatusFlags status; ///< @brief the event target status
  Event event;        ///< @brief the event target event
};

/**
 * @brief Component responsible to handle and dispatch events for the UI
 *
 */
class EventDispatcher
{
private:
  struct EventTargetUnStack
  {
    void operator()(EventDispatcher* dispatcher) { dispatcher->popTarget(); }
  };

public:
  /// @name EventTriggers Event triggers
  /// @{

  /// @brief Change position for pointer
  void movePointer(const SDL_Point& pos) { pointerPos = pos; }

  /// @brief Mark given mouse button as pressed on this frame
  void pressPointer(unsigned button)
  {
    SDL_assert(button < 32);
    pointerPressed |= 1 << button;
  }

  /// @brief Mark given mouse button as released on this frame
  void releasePointer(unsigned button)
  {
    SDL_assert(button < 32);
    pointerReleased |= 1 << button;
  }

  /// @brief Insert a keyboard command
  void command(Command cmd);

  /// @brief Insert text
  void input(std::string_view text);
  /// @}

  /// @brief Reset Distatcher (call once per turn)
  void reset();

  /**
   * @brief Try to focus on the given element
   *
   * @return true if it is possible to focus
   * @return false if focus already changed this frame
   */
  bool tryFocus(std::string_view qualifiedId);

  /// @name accessors Accessors
  /// @{

  /**
   * @brief Return current pointer position
   *
   * @return const SDL_Point
   */
  const SDL_Point& pointerPosition() const { return pointerPos; }

  /**
   * @brief Check if the n-th button is pressed
   *
   * @param button the button number. 0 is the left, 1 middle and 2 is right
   * @return true
   * @return false
   */
  bool isPointerPressed(unsigned button) const
  {
    SDL_assert(button < 32);
    return !pointerReleased && pointerPressed & unsigned(1 << button);
  }

  /**
   * @brief Checks if given id is focused
   *
   * @param id the qualified id
   * @return true
   * @return false
   */
  bool isActive(std::string_view id) const { return idFocus == id; }

  /// @brief Gets the current command
  Command command() const { return nextCommand; }

  /// @brief Gets the current input
  std::string_view input() const { return inputBuffer; }

  /**
   * @brief If true, the state wants the mouse events
   */
  bool wantsMouse() const { return hadHover || !idGrabbed.empty(); }

  /**
   * @brief If true, the state wants the keyboard events
   * @return true
   * @return false
   */
  bool wantsKeyboard() const { return !idFocus.empty(); }
  /// @}

  class EventTarget;
  /**
   * @brief Check events for the specified element
   *
   * @param req events you accept. Events after it will be ignored
   * @param rect the area occupied by the element (absolute)
   * @param id the unique id representing the event target
   * @return EventTarget
   */
  EventTarget check(RequestEvent req,
                    const SDL_Rect& rect,
                    std::string_view id = {});

private:
  SDL_Point pointerPos;
  Uint32 pointerPressed;
  Uint32 pointerReleased;

  std::string idCurrent;
  bool hadHover;
  std::string idGrabbed;
  std::string idFocus;
  std::string idNextFocus;
  std::string idLosingFocus;

  Command nextCommand;
  std::string inputBuffer;

  std::vector<EventTargetState> elementStack;

  void popTarget();

  StatusFlags checkHover(RequestEvent req, const SDL_Rect& rect, Event& event);

  StatusFlags checkGrabOver(RequestEvent req, Event& event);
  StatusFlags checkGrabOut(RequestEvent req, Event& event);
  StatusFlags checkGrabCommand(RequestEvent req, Event& event);

  StatusFlags checkFocus(RequestEvent req, Event& event);
  StatusFlags gainFocus(RequestEvent req, Event& event);
  StatusFlags loseFocus(RequestEvent req, Event& event);
  StatusFlags checkFocusCommand(RequestEvent req, Event& event);

  StatusFlags checkInputCommand(Event& event);

  StatusFlags checkActionCommand(Event& event);
};

/**
 * @brief An element able to receive events
 *
 */
class EventDispatcher::EventTarget
  : public CookieBase<EventDispatcher, EventDispatcher::EventTargetUnStack>
{
  size_t index;

  EventTarget(EventDispatcher* c, size_t index)
    : CookieBase(c)
    , index(index)
  {}

  friend class EventDispatcher;

  EventTargetState& state() { return get()->elementStack[index]; }

public:
  EventTarget() = default;

  const EventTargetState& state() const { return get()->elementStack[index]; }

  StatusFlags status() const { return state().status; }
  Event event() const { return state().event; }
  std::string_view input() const { return get()->input(); }

  const SDL_Rect& rect() const { return state().rect; }

  void shrinkWidth(int w);

  void shrinkHeight(int h);

  void shrink(int w, int h);

  void discard();
};

inline void
EventDispatcher::command(Command cmd)
{
  nextCommand = cmd;
}

inline void
EventDispatcher::input(std::string_view text)
{
  if (nextCommand == Command::SPACE) {
    nextCommand = Command::NONE;
    inputBuffer += ' ';
    if (text == " ") {
      return;
    }
  } else if (nextCommand == Command::BACKSPACE) {
    if (inputBuffer.empty()) {
      return;
    }
    nextCommand = Command::NONE;
    inputBuffer.pop_back();
  }
  inputBuffer += text;
}

inline void
EventDispatcher::reset()
{
  SDL_assert(elementStack.empty());
  hadHover = false;
  pointerPressed = pointerReleased = 0;
  if (idNextFocus == idFocus) {
    idNextFocus.clear();
  }
  if (idLosingFocus == idFocus) {
    idFocus.clear();
  } else {
    idLosingFocus.clear();
  }
  nextCommand = Command::NONE;
  inputBuffer.clear();
}

inline bool
EventDispatcher::tryFocus(std::string_view qualifiedId)
{
  if (!idNextFocus.empty() && idNextFocus != idFocus) {
    return false;
  }
  idNextFocus = qualifiedId;
  if (!idFocus.empty()) {
    idLosingFocus = idFocus;
  }
  return true;
}

inline EventDispatcher::EventTarget
EventDispatcher::check(RequestEvent req,
                       const SDL_Rect& rect,
                       std::string_view id)
{
  if (elementStack.empty()) {
    idCurrent = id;
  } else {
    idCurrent += groupNameSeparator;
    idCurrent += id;
  }
  if (req == RequestEvent::NONE) {
    elementStack.emplace_back(EventTargetState{id.size(), rect});
    return {this, elementStack.size() - 1};
  }
  SDL_assert(!id.empty() || req == RequestEvent::HOVER);

  Event event = Event::NONE;
  elementStack.emplace_back(EventTargetState{
    id.size(),
    rect,
    checkHover(req, rect, event),
    event,
  });
  return {this, elementStack.size() - 1};
}

inline StatusFlags
EventDispatcher::checkHover(RequestEvent req,
                            const SDL_Rect& rect,
                            Event& event)
{
  if (hadHover ||
      !(elementStack.empty() ||
        elementStack.back().status.test(Status::HOVERED)) ||
      !SDL_PointInRect(&pointerPos, &rect)) {
    if (req == RequestEvent::HOVER) {
      return Status::NONE;
    }
    return checkGrabOut(req, event);
  }
  if (req == RequestEvent::HOVER) {
    return Status::HOVERED;
  }
  return Status::HOVERED | checkGrabOver(req, event);
}

inline StatusFlags
EventDispatcher::checkGrabOver(RequestEvent req, Event& event)
{
  if (pointerReleased != 0) {
    if (idGrabbed == idCurrent) {
      event = Event::ACTION;
      idGrabbed.clear();
    }
    return checkFocus(req, event);
  }
  if (pointerPressed != 1) {
    if (idGrabbed != idCurrent) {
      return req == RequestEvent::GRAB ? Status::NONE : gainFocus(req, event);
    }
    if (pointerPressed == 0) {
      return Status::GRABBED | checkGrabCommand(req, event);
    }
    event = Event::CANCEL;
    idGrabbed.clear();
    return checkFocus(req, event);
  }
  event = Event::GRAB;
  idGrabbed = idCurrent;
  if (req == RequestEvent::GRAB) {
    return Status::GRABBED;
  }
  return Status::GRABBED | gainFocus(req, event);
}

inline StatusFlags
EventDispatcher::checkGrabOut(RequestEvent req, Event& event)
{
  if (idGrabbed != idCurrent) {
    return pointerPressed == 0 ? checkFocus(req, event) : loseFocus(req, event);
  }
  if (pointerReleased == 0 && pointerPressed == 0) {
    return Status::GRABBED | checkFocus(req, event);
  }
  event = Event::CANCEL;
  idGrabbed.clear();
  if (req == RequestEvent::GRAB || idFocus != idCurrent) {
    return Status::NONE;
  }
  if (pointerPressed != 0) {
    return loseFocus(req, event);
  }
  return checkFocus(req, event);
}

inline StatusFlags
EventDispatcher::checkGrabCommand(RequestEvent req, Event& event)
{
  if (nextCommand == Command::ESCAPE) {
    event = Event::CANCEL;
  }
  return req == RequestEvent::GRAB ? checkActionCommand(event)
                                   : checkFocus(req, event);
}

inline StatusFlags
EventDispatcher::checkFocus(RequestEvent req, Event& event)
{
  if (idFocus == idCurrent) {
    if (idLosingFocus == idCurrent) {
      return Status::FOCUSED;
    }
    idNextFocus = idCurrent;
    return Status::FOCUSED | checkFocusCommand(req, event);
  }
  if (idLosingFocus == idCurrent) {
    event = Event::FOCUS_LOST;
    return Status::NONE;
  }
  if (idNextFocus == idCurrent) {
    idFocus = idCurrent;
    event = Event::FOCUS_GAINED;
    return Status::FOCUSED;
  }
  return Status::NONE;
}

inline StatusFlags
EventDispatcher::gainFocus(RequestEvent req, Event& event)
{
  if (idFocus == idCurrent || idNextFocus == idCurrent) {
    return checkFocus(req, event);
  }
  if (!idNextFocus.empty()) {
    if (elementStack.empty() ||
        !elementStack.back().status.test(Status::FOCUSED)) {
      return Status::NONE;
    }
    auto& superElement = elementStack.back();
    if (superElement.event != Event::NONE &&
        superElement.event != Event::FOCUS_GAINED) {
      idNextFocus = idCurrent;
      idLosingFocus = idFocus;
      return Status::NONE;
    }
  }
  idNextFocus = idCurrent;
  if (event != Event::NONE || !idLosingFocus.empty() ||
      (!idFocus.empty() && idFocus == idGrabbed)) {
    return Status::NONE;
  }
  idLosingFocus = idFocus;
  idFocus = idCurrent;
  event = Event::FOCUS_GAINED;
  return Status::FOCUSED;
}
inline StatusFlags
EventDispatcher::loseFocus(RequestEvent req, Event& event)
{
  if (idFocus != idCurrent) {
    return checkFocus(req, event);
  }
  if (event == Event::NONE) {
    idFocus.clear();
    event = Event::FOCUS_LOST;
    return Status::NONE;
  }
  idLosingFocus = idCurrent;
  auto status = checkFocus(req, event);
  return status;
}

inline StatusFlags
EventDispatcher::checkFocusCommand(RequestEvent req, Event& event)
{
  return req == RequestEvent::FOCUS ? checkActionCommand(event)
                                    : checkInputCommand(event);
}

inline StatusFlags
EventDispatcher::checkInputCommand(Event& event)
{
  switch (nextCommand) {
  case Command::ENTER:
    if (inputBuffer.empty()) {
      event = Event::END_LINE;
    } else {
      event = Event::NONE;
      return Status::NONE;
    }
    break;
  case Command::SPACE:
    if (inputBuffer.empty()) {
      event = Event::SPACE;
    } else {
      event = Event::INPUT;
      inputBuffer += ' ';
    }
    break;
  case Command::BACKSPACE:
    if (inputBuffer.empty()) {
      event = Event::BACKSPACE;
    } else {
      event = Event::INPUT;
      inputBuffer.pop_back();
    }
    break;
  case Command::ESCAPE:
    event = Event::CANCEL;
    break;
  default:
    if (!inputBuffer.empty()) {
      event = Event::INPUT;
    }
    break;
  }
  return checkActionCommand(event);
}

inline StatusFlags
EventDispatcher::checkActionCommand(Event& event)
{
  if (event != Event::NONE) {
    idGrabbed.clear();
    return Status::NONE;
  }
  switch (nextCommand) {
  case Command::NONE:
    return Status::NONE;
  case Command::ACTION:
  case Command::ENTER:
  case Command::SPACE:
    event = Event::ACTION;
    break;
  default:
    break;
  }
  idGrabbed.clear();
  return Status::NONE;
}

inline void
EventDispatcher::popTarget()
{
  auto element = elementStack.back();
  elementStack.pop_back();
  if (element.status.test(Status::HOVERED)) {
    hadHover = true;
  }
  if (elementStack.empty()) {
    idCurrent.clear();
    return;
  }

  auto sz = element.idLength + 1;
  SDL_assert_paranoid(idCurrent.size() > sz);
  idCurrent.resize(idCurrent.size() - sz);

  bool hadGrab = element.status.test(Status::GRABBED);
  bool hadFocus = element.status.test(Status::FOCUSED);
  if (!hadGrab && !hadFocus) {
    return;
  }
  auto& superElement = elementStack.back();
  if (hadGrab) {
    superElement.status.reset(Status::GRABBED);
    if (superElement.event == Event::GRAB) {
      superElement.event = Event::NONE;
    }
  }
  if (hadFocus) {
    superElement.status.reset(Status::FOCUSED);
    if (superElement.event == Event::FOCUS_GAINED) {
      superElement.event = Event::NONE;
    } else {
      superElement.event = Event::FOCUS_LOST;
    }
  }
}

inline void
EventDispatcher::EventTarget::shrinkWidth(int w)
{
  auto& rect = state().rect;
  rect.w = w;
  if (get()->pointerPos.x - rect.x >= w) {
    discard();
  }
}

inline void
EventDispatcher::EventTarget::shrinkHeight(int h)
{
  auto& rect = state().rect;
  rect.h = h;
  if (get()->pointerPos.y - rect.y >= h) {
    discard();
  }
}

inline void
EventDispatcher::EventTarget::shrink(int w, int h)
{
  auto& rect = state().rect;
  rect.w = w;
  rect.h = h;
  if (get()->pointerPos.y - rect.y >= h) {
    discard();
  }
}

inline void
EventDispatcher::EventTarget::discard()
{
  auto& elState = state();

  if (!elState.status.test(Status::GRABBED)) {
    elState.status.reset(Status::HOVERED);
  } else if (elState.event == Event::GRAB) {
    elState.status.reset(Status::HOVERED | Status::GRABBED);
    elState.event = Event::NONE;
  }
}

} // namespace boto

#endif // BOTO_CORE_EVENTDISPATCHER_HPP
