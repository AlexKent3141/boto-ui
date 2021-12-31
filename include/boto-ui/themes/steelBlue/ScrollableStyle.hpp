#pragma once
#include "PanelStyle.hpp"
#include "SliderBoxStyle.hpp"
#include "core/Theme.hpp"

namespace boto {

struct ScrollableStyle
{
  bool fixHorizontal;
  bool fixVertical;
  SliderBoxStyle slider;
  GroupStyle client;

  constexpr ScrollableStyle withFixHorizontal(bool fixHorizontal) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollableStyle withFixVertical(bool fixVertical) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollableStyle withSlider(SliderBoxStyle slider) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollableStyle withClient(const GroupStyle& client) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollableStyle withElementSpacing(int elementSpacing) const
  {
    return withClient(client.withElementSpacing(elementSpacing));
  }

  constexpr ScrollableStyle withLayout(Layout layout) const
  {
    return withClient(client.withLayout(layout));
  }

  constexpr operator GroupStyle() const { return client; }
};

struct ScrollablePanelStyle
{
  bool fixHorizontal;
  bool fixVertical;
  SliderBoxStyle slider;
  PanelStyle client;
  constexpr operator PanelStyle() const { return client; }

  constexpr ScrollablePanelStyle withFixHorizontal(bool fixHorizontal) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollablePanelStyle withFixVertical(bool fixVertical) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollablePanelStyle withSlider(SliderBoxStyle slider) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollablePanelStyle withClient(const PanelStyle& client) const
  {
    return {fixHorizontal, fixVertical, slider, client};
  }

  constexpr ScrollablePanelStyle withElementSpacing(int elementSpacing) const
  {
    return withClient(client.withElementSpacing(elementSpacing));
  }

  constexpr ScrollablePanelStyle withLayout(Layout layout) const
  {
    return withClient(client.withLayout(layout));
  }

  constexpr ScrollablePanelStyle withPadding(const EdgeSize& padding) const
  {
    return withClient(client.withPadding(padding));
  }
  constexpr ScrollablePanelStyle withBorderSize(const EdgeSize& border) const
  {
    return withClient(client.withBorderSize(border));
  }
  constexpr ScrollablePanelStyle withDecoration(
    const ElementStyle& decoration) const
  {
    return withClient(client.withDecoration(decoration));
  }
  constexpr ScrollablePanelStyle withBackground(SDL_Color background) const
  {
    return withClient(client.withBackground(background));
  }
  constexpr ScrollablePanelStyle withBorder(SDL_Color border) const
  {
    return withClient(client.withBorder(border));
  }
};

struct Scrollable;
struct ScrollablePanel;

template<>
struct StyleFor<SteelBlue, Scrollable>
{
  static ScrollableStyle get(Theme& theme)
  {
    return {
      false,                 // Fix horizontal
      false,                 // Fix vertical
      theme.of<SliderBox>(), // scrollable
      theme.of<Group>(),     // group
    };
  }
};

template<>
struct StyleFor<SteelBlue, ScrollablePanel>
{
  static ScrollablePanelStyle get(Theme& theme)
  {
    return {
      false,                 // Fix horizontal
      false,                 // Fix vertical
      theme.of<SliderBox>(), // scrollable
      theme.of<Panel>().withPadding({0, 0, 255, 255}),
    };
  }
};

constexpr SDL_Point
evalScrollbarSpace(const SliderBoxStyle& style,
                   bool fixedHorizontal,
                   bool fixedVertical)
{
  SDL_Point p{0};
  auto buttonStyle = style.buttons.normal;
  if (!fixedHorizontal) {
    p.x = buttonStyle.padding.left + buttonStyle.padding.right +
          buttonStyle.decoration.border.left +
          buttonStyle.decoration.border.right + 8;
  }
  if (!fixedVertical) {
    p.y = buttonStyle.padding.top + buttonStyle.padding.bottom +
          buttonStyle.decoration.border.top +
          buttonStyle.decoration.border.bottom + 8;
  }
  return p;
}

} // namespace boto
