#ifndef GRADIENT_H
#define GRADIENT_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ANALYZER_TYPE_GRADIENT gradient_get_type()
G_DECLARE_FINAL_TYPE (Gradient, gradient, ANALYZER, GRADIENT, GObject)

Gradient *gradient_new (int count_stops, ...);

ClutterColor gradient_evaluate (const Gradient *self,
                                float position);

G_END_DECLS


#endif // GRADIENT_H
