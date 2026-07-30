#include "ui.h"

void fmt1(char *buf, size_t cap, double v) {
  bool neg = v < 0;
  if (neg) v = -v;
  int whole = (int)v;
  int tenths = (int)((v - whole) * 10 + 0.5);
  if (tenths >= 10) { whole++; tenths = 0; }
  snprintf(buf, cap, "%s%d.%d", neg ? "-" : "", whole, tenths);
}
