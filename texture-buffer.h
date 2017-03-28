#ifndef TEXTUREBUFFER_H
#define TEXTUREBUFFER_H

#include <glib.h>

typedef struct _TextureBuffer
{
  guint8 *data;
  guint width;
  guint height;
} TextureBuffer;

TextureBuffer*
texture_buffer_new (guint width,
                    guint height);

void
texture_buffer_set_pixel (TextureBuffer *self,
                          guint x,
                          guint y,
                          guint8 r,
                          guint8 g,
                          guint8 b);

void
texture_buffer_fill (TextureBuffer *self,
                     guint8 r,
                     guint8 g,
                     guint8 b);

void
texture_buffer_free (TextureBuffer *self);

#endif // TEXTUREBUFFER_H
