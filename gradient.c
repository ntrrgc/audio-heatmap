#include "gradient.h"
#include <math.h>

/*
 * Linear RGB color, from 0.0 to 1.0, non gamma-corrected.
 * Using a linear colorspace is needed to create good looking gradients.
 * sRGB, the colorspace used by output screens and usually associated with
 * RGB values is NOT linear, but gamma encoded.
 */
typedef struct
{
  float r;
  float g;
  float b;
} LRGBColor;

const static LRGBColor BLACK = { 0.0f, 0.0f, 0.0f };

typedef struct
{
  float position;
  LRGBColor color;
} ColorStop;

static gpointer
color_stop_array_copy (gpointer src)
{
  return g_array_ref (src);
}

static void
color_stop_array_free (gpointer object)
{
  g_array_unref (object);
}

struct _Gradient
{
  GObject parent_instance;

  GArray *color_stops;
};

G_DEFINE_TYPE (Gradient, gradient, G_TYPE_OBJECT)

enum
{
  PROP_COLOR_STOPS = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gradient_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  Gradient *self = (Gradient *)object;

  switch (property_id)
    {
    case PROP_COLOR_STOPS:
      {
      g_array_unref (self->color_stops);
      GArray *color_stops = g_value_get_boxed (value);
      self->color_stops = g_array_ref (color_stops);
      break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gradient_get_property (GObject     *object,
                       guint        property_id,
                       GValue      *value,
                       GParamSpec  *pspec)
{
  Gradient *self = (Gradient *)object;

  switch (property_id)
    {
    case PROP_COLOR_STOPS:
      g_value_set_boxed (value, self->color_stops);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
gradient_class_init (GradientClass *klass)
{
  GType color_stop_array_type =
      g_boxed_type_register_static ("ColorStopArray",
                                    color_stop_array_copy,
                                    color_stop_array_free);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gradient_set_property;
  object_class->get_property = gradient_get_property;

  obj_properties[PROP_COLOR_STOPS] =
      g_param_spec_boxed ("color_stops",
                          "Color stops",
                          "Array of color stops used to calculate the gradient.",
                          color_stop_array_type,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
gradient_init (Gradient *self)
{
  self->color_stops = g_array_new (FALSE, TRUE, sizeof (ColorStop));
}

#define GAMMA 2.2f

static LRGBColor
lrgb_color_from_srgb_values (guint8 r, guint8 g, guint8 b)
{
  LRGBColor ret;
  ret.r = powf (((float) r) / 255.0f, GAMMA);
  ret.g = powf (((float) g) / 255.0f, GAMMA);
  ret.b = powf (((float) b) / 255.0f, GAMMA);
  return ret;
}

static ClutterColor
lrgb_color_to_srgb (const LRGBColor *color)
{
  ClutterColor ret;
  ret.alpha = 255;
  ret.red   = (guint8) (255 * powf (color->r, 1 / GAMMA));
  ret.green = (guint8) (255 * powf (color->g, 1 / GAMMA));
  ret.blue  = (guint8) (255 * powf (color->b, 1 / GAMMA));
  return ret;
}

Gradient*
gradient_new (int count_stops, ...)
{
  /** (array) (element-type ColorStop) */
  GArray *color_stops = g_array_sized_new (FALSE, TRUE, sizeof (ColorStop),
                                           count_stops);

  int i;
  va_list args;
  va_start (args, count_stops);
  for (i = 0; i < count_stops; i++)
    {
      float position = (float) va_arg (args, double);
      guint8 r = (guint8) va_arg (args, int);
      guint8 g = (guint8) va_arg (args, int);
      guint8 b = (guint8) va_arg (args, int);

      ColorStop color_stop;
      color_stop.color = lrgb_color_from_srgb_values (r, g, b);
      color_stop.position = position;

      g_array_append_val (color_stops, color_stop);
    }
  va_end (args);

  Gradient *gradient = g_object_new (ANALYZER_TYPE_GRADIENT,
                                     "color_stops", color_stops,
                                     NULL);
}

static float
lerp (float a, float b, float position)
{
  g_assert (position >= 0.0f);
  g_assert (position <= 1.0f);
  return a + (b - a) * position;
}

static LRGBColor
lrgb_color_lerp (LRGBColor colorA,
                 LRGBColor colorB,
                 float     position)
{
  LRGBColor ret;
  ret.r = lerp(colorA.r, colorB.r, position);
  ret.g = lerp(colorA.g, colorB.g, position);
  ret.b = lerp(colorA.b, colorB.b, position);
  return ret;
}

static LRGBColor
gradient_evaluate_lrgb(const Gradient *self,
                       float           position)
{
  g_assert (position >= 0.0f && position <= 1.0f);

  int i;
  for (i = 0; i < self->color_stops->len; i++)
    {
      ColorStop *current_stop = &g_array_index (self->color_stops, ColorStop, i);
      if (current_stop->position == position)
        {
          return current_stop->color;
        }
      else if (current_stop->position > position)
        {
          g_assert (i > 0);
          ColorStop *previous_stop = &g_array_index
              (self->color_stops, ColorStop, i - 1);

          float distance_current_stop = current_stop->position - position;
          float distance_previous_stop = position - previous_stop->position;
          return lrgb_color_lerp (previous_stop->color,
                                  current_stop->color,
                                  distance_previous_stop /
                                  (distance_previous_stop + distance_current_stop));
        }
    }

  g_return_val_if_reached (BLACK);
}

ClutterColor gradient_evaluate (const Gradient *self,
                                float position)
{
  LRGBColor lrgb = gradient_evaluate_lrgb (self, position);
  return lrgb_color_to_srgb (&lrgb);
}
