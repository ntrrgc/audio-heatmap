#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef void (*QuitCallback) (void);

void visualization_launch (int         *argc,
                           char      ***argv,
                           gint         bands,
                           QuitCallback quit_callback);

void visualization_feed_spectrum(const double *mags,
                                 double minValue,
                                 double maxValue);

G_END_DECLS


#endif // VISUALIZATION_H
