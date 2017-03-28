#include "texture-buffer.h"
#include <glib.h>

void
texture_buffer_set_pixel (TextureBuffer *self,
                          guint x,
                          guint y,
                          guint8 r,
                          guint8 g,
                          guint8 b)
{
  self->data[((y * self->width + x) << 2) | 0] = r;
  self->data[((y * self->width + x) << 2) | 1] = g;
  self->data[((y * self->width + x) << 2) | 2] = b;
  self->data[((y * self->width + x) << 2) | 3] = 255;
}

TextureBuffer*
texture_buffer_new (guint width,
                    guint height)
{
  TextureBuffer *self = g_slice_new (TextureBuffer);
  self->data = g_slice_alloc (4 * width * height);
  self->width = width;
  self->height = height;
  return self;
}

void
texture_buffer_free (TextureBuffer *self)
{
  g_slice_free1 (4 * self->width * self->height, self->data);
  g_slice_free (TextureBuffer, self);
}

void
texture_buffer_fill(TextureBuffer *self, guint8 r, guint8 g, guint8 b)
{
  for (int y = 0; y < self->height; y++)
    {
      for (int x = 0; x < self->width; x++)
        {
          texture_buffer_set_pixel(self, x, y, r, g, b);
        }
    }
}
