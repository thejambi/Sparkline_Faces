#include "health.h"
#include "settings.h"

//#define DEV_FAKE_HEALTH

static Terrain s_terrain;
static bool s_terrain_valid;
static bool s_was_sleeping;

// ---------------------------------------------------------------------------
#if defined(PBL_HEALTH)

int hl_steps(void) {
#ifdef DEV_FAKE_HEALTH
  return 8842;
#endif
  return (int)health_service_sum_today(HealthMetricStepCount);
}

int hl_walked_m(void) {
#ifdef DEV_FAKE_HEALTH
  return 6300;
#endif
  return (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
}

int hl_sleep_secs(void) {
#ifdef DEV_FAKE_HEALTH
  return 6 * 3600 + 32 * 60;
#endif
  time_t start = time_start_of_today(), end = time(NULL);
  HealthServiceAccessibilityMask m =
      health_service_metric_accessible(HealthMetricSleepSeconds, start, end);
  if (!(m & HealthServiceAccessibilityMaskAvailable)) return 0;
  return (int)health_service_sum_today(HealthMetricSleepSeconds);
}

int hl_bpm(void) {
#ifdef DEV_FAKE_HEALTH
  return 68;
#endif
#if PBL_API_EXISTS(health_service_peek_current_value)
  return (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
#else
  return 0;
#endif
}

// --- the past hour ---------------------------------------------------------
// Solfarer's discipline, unchanged: the minute history is read once at boot
// and then exactly once more at the first quarter-hour mark, because that is
// when the watch has filled in the hour it was not running for. After that
// the newest column is kept live from the step delta and never refetches.
static uint8_t s_minsteps[60];
static int s_step_snap = -1;
static bool s_refetch_pending;

static void fetch_hour(void) {
#ifdef DEV_FAKE_HEALTH
  {
    time_t fnow = time(NULL);
    struct tm *ft = localtime(&fnow);
    for (int i = 0; i < 60; i++) {
      int ago = 59 - i, st = 0;
      if (ago >= 40 && ago < 48) st = 55 + (i * 7) % 40;
      else if (ago >= 20 && ago < 26) st = 12 + (i * 5) % 24;
      else if (ago < 6) st = 40 + (i * 11) % 50;
      s_minsteps[((ft->tm_min - ago) % 60 + 60) % 60] = st;
    }
    return;
  }
#endif
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int cur = t->tm_min;
  HealthMinuteData *md = malloc(60 * sizeof *md);
  if (!md) return;
  time_t end = now, start = now - SECONDS_PER_HOUR;
  uint32_t n = health_service_get_minute_history(md, 60, &start, &end);
  for (uint32_t i = 0; i < n; i++) {
    int idx = ((int)i + 1 + cur) % 60;
    if (!md[i].is_invalid) s_minsteps[idx] = md[i].steps;
  }
  free(md);
}

static int live_minute_steps(void) {
  if (s_step_snap < 0) return 0;
  int d = hl_steps() - s_step_snap;
  return d > 0 ? d : 0;
}

// --- last night ------------------------------------------------------------
// The face shows a sleep duration for the first stretch of every morning. If
// the terrain kept showing the past hour it would be sixty empty columns —
// the signature element blank exactly when the alternate state is up. So in
// sleep state the window becomes the night itself, and the columns carry
// movement instead of steps.
typedef struct { time_t start, end; } SleepWindow;

static bool sleep_cb(HealthActivity activity, time_t start, time_t end,
                     void *ctx) {
  SleepWindow *w = (SleepWindow *)ctx;
  if (w->start == 0 || start < w->start) w->start = start;
  if (end > w->end) w->end = end;
  return true;
}

static bool last_night(SleepWindow *w) {
  w->start = w->end = 0;
  time_t now = time(NULL);
  health_service_activities_iterate(
      HealthActivitySleep | HealthActivityRestfulSleep,
      now - 20 * SECONDS_PER_HOUR, now, HealthIterationDirectionPast,
      sleep_cb, w);
  return w->end > w->start && (w->end - w->start) > 30 * SECONDS_PER_MINUTE;
}

// Movement per column. vmc is the watch's own restlessness measure, which is
// what makes a night legible: steps during sleep are all zero.
static bool build_night(Terrain *t) {
#ifdef DEV_FAKE_HEALTH
  {
    static const uint8_t N[COLS] = {
      0,0,30,8,0,0,0,52,14,0,0,0,0,0,80,34,0,0,0,0,
      0,18,0,0,0,0,66,110,44,0,0,0,0,0,0,26,0,0,0,0,
      40,10,0,0,0,0,0,96,150,55,0,0,0,22,0,0,70,160,230,140 };
    memcpy(t->col, N, sizeof N);
    t->cap = 255; t->live = false;
    return true;
  }
#endif
  SleepWindow w;
  if (!last_night(&w)) return false;
  uint32_t span = (uint32_t)(w.end - w.start);
  uint32_t acc[COLS];
  memset(acc, 0, sizeof acc);

  HealthMinuteData *md = malloc(60 * sizeof *md);
  if (!md) return false;
  time_t cur = w.start;
  int guard = 0;
  while (cur < w.end && guard++ < 20) {
    time_t s = cur, e = w.end;
    uint32_t n = health_service_get_minute_history(md, 60, &s, &e);
    if (n == 0) break;
    for (uint32_t i = 0; i < n; i++) {
      if (md[i].is_invalid) continue;
      time_t mt = s + (time_t)i * SECONDS_PER_MINUTE;
      if (mt < w.start || mt >= w.end) continue;
      uint32_t b = (uint32_t)(mt - w.start) * COLS / span;
      if (b < COLS) acc[b] += md[i].vmc;
    }
    cur = e > cur ? e : cur + 60 * SECONDS_PER_MINUTE;
  }
  free(md);

  uint32_t peak = 0;
  for (int i = 0; i < COLS; i++) if (acc[i] > peak) peak = acc[i];
  if (peak == 0) return false;
  for (int i = 0; i < COLS; i++)
    t->col[i] = (uint8_t)(acc[i] * 255u / peak);
  t->cap = 255;
  t->live = false;
  return true;
}

static void health_evt(HealthEventType e, void *ctx) {
  if (e == HealthEventMovementUpdate) {
    s_terrain_valid = false;
    face_poke();
  }
}

void health_init(void) {
  fetch_hour();
  s_refetch_pending = true;
  s_step_snap = hl_steps();
  health_service_events_subscribe(health_evt, NULL);
}

void health_deinit(void) { health_service_events_unsubscribe(); }

void health_minute(struct tm *t) {
  s_minsteps[t->tm_min] = 0;
  s_step_snap = hl_steps();
  if (s_refetch_pending && t->tm_min % 15 == 1) {
    fetch_hour();
    s_refetch_pending = false;
  }
  s_terrain_valid = false;
}

#else   // no health on this platform
int hl_steps(void) { return 0; }
int hl_walked_m(void) { return 0; }
int hl_sleep_secs(void) { return 0; }
int hl_bpm(void) { return 0; }
static uint8_t s_minsteps[60];
static int live_minute_steps(void) { return 0; }
static bool build_night(Terrain *t) { (void)t; return false; }
void health_init(void) {}
void health_deinit(void) {}
void health_minute(struct tm *t) { (void)t; s_terrain_valid = false; }
#endif

// ---------------------------------------------------------------------------
bool hl_sleeping(void) {
  return g_cfg.show_sleep && hl_steps() <= (int)g_cfg.wake_threshold &&
         hl_sleep_secs() > 0;
}

const Terrain *hl_terrain(void) {
  bool sleeping = hl_sleeping();
  if (s_terrain_valid && sleeping == s_was_sleeping) return &s_terrain;
  s_was_sleeping = sleeping;
  s_terrain_valid = true;

  if (sleeping && g_cfg.sleep_terrain && build_night(&s_terrain))
    return &s_terrain;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int live = live_minute_steps();
  s_minsteps[t->tm_min] = live > 255 ? 255 : (uint8_t)live;
  for (int i = 0; i < COLS; i++)
    s_terrain.col[i] = s_minsteps[(t->tm_min + 1 + i) % 60];
  s_terrain.cap = STEP_CAP;
  s_terrain.live = true;
  return &s_terrain;
}
