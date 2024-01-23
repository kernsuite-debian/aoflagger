#include "title.h"

void Title::Draw(const Cairo::RefPtr<Cairo::Context>& cairo) {
  initializeMetrics(cairo);

  cairo->set_source_rgb(0.0, 0.0, 0.0);
  const Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_fontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  layout->set_text(_text);
  const Pango::Rectangle extents = layout->get_pixel_ink_extents();

  cairo->move_to(_plotWidth / 2.0 - extents.get_width() / 2.0,
                 _topMargin + 6 - extents.get_y());
  layout->show_in_cairo_context(cairo);
}

void Title::initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo) {
  if (!_metricsAreInitialized) {
    const Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
    Pango::FontDescription fontDescription;
    fontDescription.set_size(_fontSize * PANGO_SCALE);
    layout->set_font_description(fontDescription);
    layout->set_text(_text);
    const Pango::Rectangle extents = layout->get_pixel_ink_extents();
    _height = extents.get_height() + 12;

    _metricsAreInitialized = true;
  }
}
