#pragma once

#include <glib-object.h>
#include <glib.h>
#include <memory>

namespace hyprbar {

/**
 * RAII wrapper for GError
 * Automatically frees GError when it goes out of scope
 */
struct GErrorDeleter {
  void operator()(GError* error) const {
    if (error) {
      g_error_free(error);
    }
  }
};

using GErrorPtr = std::unique_ptr<GError, GErrorDeleter>;

/**
 * RAII wrapper for GObject-derived types
 * Automatically unrefs GObject when it goes out of scope
 */
struct GObjectUnref {
  void operator()(gpointer object) const {
    if (object) {
      g_object_unref(object);
    }
  }
};

/**
 * Smart pointer for GObject-derived types (GdkPixbuf, etc.)
 * Usage: GObjectPtr<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file(...));
 */
template <typename T> using GObjectPtr = std::unique_ptr<T, GObjectUnref>;

} // namespace hyprbar
