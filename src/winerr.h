// Copied from:
//   https://github.com/nfc-tools/libnfc/blob/b978c45a11a71a502f26ca416381492124d62162/contrib/win32/err.h
// Which is under LGPL:
//   https://github.com/nfc-tools/libnfc/blob/b978c45a11a71a502f26ca416381492124d62162/COPYING

#ifndef _WINERR_H_
#define _WINERR_H_

#include <stdlib.h>

#define warnx(...) do { \
    fprintf (stderr, __VA_ARGS__); \
    fprintf (stderr, "\n"); \
  } while (0)

#define errx(code, ...) do { \
    fprintf (stderr, __VA_ARGS__); \
    fprintf (stderr, "\n"); \
    exit (code); \
  } while (0)

#define err errx

#endif /* !_WINERR_H_ */