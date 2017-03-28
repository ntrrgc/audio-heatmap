#include "visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <clutter/clutter.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gst/gst.h>
#include <math.h>
#include "texture-buffer.h"
#include "gradient.h"

static int            *argc             = NULL;
static char         ***argv             = NULL;
static gint            bands            = -1;
static ClutterActor   *stage            = NULL;
static TextureBuffer  *buf              = NULL;
static ClutterContent *image            = NULL;
static gint            current_x_pos    = 0;
static QuitCallback    quit_callback    = NULL;
static Gradient       *gradient         = NULL;

const guint WIDTH = 1000;
const guint HEIGHT = 500;

static GError *error = NULL;

gboolean
process_pixel_cols (gpointer data)
{
  TextureBuffer *pixel_col = (TextureBuffer*) data;

  cairo_rectangle_int_t rect = {
    .x = current_x_pos,
    .y = 0,
    .width = pixel_col->width,
    .height = pixel_col->height,
  };

  gboolean ret = clutter_image_set_area (CLUTTER_IMAGE (image),
                          pixel_col->data,
                          COGL_PIXEL_FORMAT_RGBA_8888,
                          &rect, 0, &error);
  g_assert (ret);

  current_x_pos += 1;
  if (current_x_pos >= WIDTH)
    current_x_pos = 0;

  return G_SOURCE_REMOVE;
}

static gboolean
window_closed (ClutterStage *stage,
               ClutterEvent *event,
               gpointer      user_data)
{
  clutter_main_quit ();
  quit_callback ();
}

static gpointer
visualization_thread_fun (gpointer data)
{
  GAsyncQueue* clutter_init_queue = (GAsyncQueue*) g_async_queue_ref (data);

  if (CLUTTER_INIT_SUCCESS != clutter_init (argc, argv))
    {
      g_printerr ("Failed to initialize Clutter.\n");
      return NULL;
    }

  ClutterColor stage_color = { 0, 0, 0, 255 };

  stage = clutter_stage_new ();
  clutter_actor_set_size (stage, WIDTH, HEIGHT);
  clutter_actor_set_background_color (stage, &stage_color);
  g_signal_connect (stage, "delete-event", (GCallback) window_closed, NULL);
  clutter_actor_show (stage);

  image = clutter_image_new ();
  buf = texture_buffer_new (WIDTH, bands);
  texture_buffer_fill (buf, 0, 0, 0);
  clutter_image_set_data (CLUTTER_IMAGE (image),
                          buf->data,
                          COGL_PIXEL_FORMAT_RGBA_8888,
                          WIDTH, HEIGHT, 0, &error);

  /* renderer is destroyed with Stage */
  ClutterActor *renderer = clutter_actor_new ();
  clutter_actor_set_content (renderer, image);
  clutter_actor_set_position (renderer, 0, 0);
  clutter_actor_set_size (renderer, WIDTH, HEIGHT);

  clutter_actor_add_child (stage, renderer);

  /* This gradient is used to generate heatmap colors. */
  gradient = gradient_new (5,
                           0.0, 0, 0, 0,
                           0.2, 0, 0, 255,
                           0.4, 0, 255, 255,
                           0.6, 255, 255, 0,
                           1.0, 255, 255, 255);

  /* Signal the main thread that Clutter is ready. */
  g_async_queue_push (clutter_init_queue, "clutter ready");
  g_async_queue_unref (clutter_init_queue);

  clutter_main();

  return NULL;
}

void visualization_launch (int         *_argc,
                           char      ***_argv,
                           gint         _bands,
                           QuitCallback _quit_callback)
{
  argc = _argc;
  argv = _argv;
  bands = _bands;
  quit_callback = _quit_callback;

  /* Used to wait for Clutter initialization. */
  GAsyncQueue* clutter_init_queue = g_async_queue_new();

  g_thread_new ("visualization", visualization_thread_fun, clutter_init_queue);

  /* Wait for Clutter initialization. */
  g_async_queue_pop (clutter_init_queue);
  g_async_queue_unref (clutter_init_queue);
}

void visualization_feed_spectrum(const double *mags,
                                 double minValue,
                                 double maxValue)
{
  TextureBuffer* new_pixel_col = texture_buffer_new (1, bands);
  texture_buffer_fill(new_pixel_col, 0, 0, 255);

  for (int i = 0; i < bands; ++i)
    {
      double amp = mags[i];

      float gain = 0.4;
      float intensity = fmin(1.0, gain * fmax(0.0, amp - minValue) /
                             (maxValue - minValue));
      ClutterColor color = gradient_evaluate (gradient, intensity);

      texture_buffer_set_pixel(new_pixel_col, 0, (bands - i) - 1,
                               color.red, color.green, color.blue);
    }

  /* Command the UI thread to read the new pixel column, if the main loop
   * has not exited yet. */
  clutter_threads_add_idle (process_pixel_cols, new_pixel_col);
}

