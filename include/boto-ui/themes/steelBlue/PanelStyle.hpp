#ifndef BOTO_PANELSTYLE_HPP_
#define BOTO_PANELSTYLE_HPP_

#include "EdgeSize.hpp"
#include "GroupStyle.hpp"
#include "core/Theme.hpp"

namespace boto {

// Style for panels
struct PanelStyle
{
  ElementStyle decoration;
  EdgeSize padding;
  GroupStyle client;

  constexpr PanelStyle withDecoration(const ElementStyle& decoration) const
  {
    return {decoration, padding, client};
  }

  constexpr PanelStyle withPadding(const EdgeSize& padding) const
  {
    return {decoration, padding, client};
  }
  constexpr PanelStyle withBorderSize(const EdgeSize& border) const
  {
    return withDecoration(decoration.withBorderSize(border));
  }
  constexpr PanelStyle withBackground(SDL_Color background) const
  {
    return withDecoration(decoration.withBackground(background));
  }
  constexpr PanelStyle withBorder(SDL_Color border) const
  {
    return withDecoration(decoration.withBorder(border));
  }

  constexpr PanelStyle withClient(const GroupStyle& client) const
  {
    return {decoration, padding, client};
  }

  constexpr PanelStyle withElementSpacing(int elementSpacing) const
  {
    return withClient(client.withElementSpacing(elementSpacing));
  }

  constexpr PanelStyle withLayout(Layout layout) const
  {
    return withClient(client.withLayout(layout));
  }

  constexpr operator ElementStyle() const { return decoration; }

  constexpr operator GroupStyle() const { return client; }
};

struct Panel;

/// Default panel style
template<>
struct StyleFor<SteelBlue, Panel>
{
  static PanelStyle get(Theme& theme)
  {
    return {
      theme.of<Element>(),
      EdgeSize::all(2),
      theme.of<Group>(),
    };
  }
};

} // namespace boto

#endif // BOTO_PANELSTYLE_HPP_
