#include "catch.hpp"
#include <cstring>
#include "core/DisplayList.hpp"
#include "core/StatusStreamAdaptor.hpp"
#include "elements/presenters/ElementPresenter.hpp"

using namespace std;
using namespace boto;

inline bool
operator==(const SDL_Rect& lhs, const SDL_Rect& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.w == rhs.w && lhs.h == rhs.h;
}
inline bool
operator==(const SDL_Color& lhs, const SDL_Color& rhs)
{
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

TEST_CASE("Color Presenter")
{
  DisplayList dList;
  presentElement(dList, {0, 0, 10, 10}, SDL_Color{1, 2, 3, 4});
  auto c = dList.visit([&](const DisplayListItem& el) {
    REQUIRE(el.action == DisplayListAction::COLOR_BOX);
    REQUIRE(el.rect == SDL_Rect{0, 0, 10, 10});
    REQUIRE(el.color == SDL_Color{1, 2, 3, 4});
  });
  REQUIRE(c == 1);
}

TEST_CASE("Texture Presenter")
{
  DisplayList dList;
  presentElement(dList, {0, 0, 10, 10}, (SDL_Texture*)0xdeadbeef);
  auto c = dList.visit([&](const DisplayListItem& el) {
    REQUIRE(el.action == DisplayListAction::TEXTURE_BOX);
    REQUIRE(el.rect == SDL_Rect{0, 0, 10, 10});
    REQUIRE(el.texture == (SDL_Texture*)0xdeadbeef);
  });
  REQUIRE(c == 1);
}
