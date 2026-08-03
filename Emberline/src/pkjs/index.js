// Clay is vendored (src/pkjs/clay.js) rather than pulled from npm: the
// pebble-clay package's manifest predates the flint/gabbro platforms and
// fails the build's platform check, but the bundle itself is plain JS.
var Clay = require('./clay');
var clayConfig = require('./config');

// Runs inside the config page, not here — Clay stringifies it — so it can
// close over nothing and has to look everything up through clayConfig.
//
// Simple mode hides the roles that were split out of others and keeps them
// following their parent. Hiding alone is not enough: a hidden Clay item still
// submits its value, so a parent changing while its child sits at a stale
// default would silently break the tie the mode is promising.
function colourMode() {
  var cfg = this;
  var TIED = {
    ColAccent: ['ColTerrain'],
    ColInk: ['ColNow'],
    ColMuted: ['ColSleep'],
    ColScale: ['ColLabel', 'ColSep'],
    ColSky: ['ColInfoBg', 'ColUnlit']
  };
  var ADVANCED = ['ColTerrain', 'ColNow', 'ColSleep', 'ColLabel', 'ColSep',
                  'ColInfoBg', 'ColUnlit'];

  cfg.on(cfg.EVENTS.AFTER_BUILD, function () {
    var mode = cfg.getItemByMessageKey('ColorMode');
    if (!mode) return;

    function simple() { return String(mode.get()) === '0'; }

    function apply() {
      ADVANCED.forEach(function (key) {
        var item = cfg.getItemByMessageKey(key);
        if (item) { simple() ? item.hide() : item.show(); }
      });
    }

    Object.keys(TIED).forEach(function (parent) {
      var p = cfg.getItemByMessageKey(parent);
      if (!p) return;
      p.on('change', function () {
        if (!simple()) return;
        TIED[parent].forEach(function (childKey) {
          var c = cfg.getItemByMessageKey(childKey);
          if (c) c.set(p.get());
        });
      });
    });

    mode.on('change', apply);
    apply();
  });
}

var clay = new Clay(clayConfig, colourMode);

// ---------------------------------------------------------------------------
// Weather — Open-Meteo, no API key. The standard courtesies: the enabled
// check runs again right before touching geolocation (double gate); a manual
// location skips GPS entirely via Open-Meteo's geocoder, resolved once and
// cached; refresh every 30 minutes.
// ---------------------------------------------------------------------------
function settings() {
  try {
    return JSON.parse(localStorage.getItem('clay-settings')) || {};
  } catch (e) {
    return {};
  }
}

function weatherOn(s) {
  if (s.WeatherOn === undefined) return true;   // on by default, like the watch
  return s.WeatherOn === true || s.WeatherOn === 1 || s.WeatherOn === '1';
}

function getJSON(url, cb) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    try { cb(JSON.parse(this.responseText)); } catch (e) {}
  };
  xhr.open('GET', url);
  xhr.send();
}

function fetchTemp(lat, lon, unit) {
  getJSON('https://api.open-meteo.com/v1/forecast?latitude=' + lat +
          '&longitude=' + lon + '&current=temperature_2m&temperature_unit=' + unit,
    function (d) {
      if (d && d.current && typeof d.current.temperature_2m === 'number') {
        Pebble.sendAppMessage({ WeatherTemp: Math.round(d.current.temperature_2m) });
      }
    });
}

function fetchWeather() {
  var s = settings();
  if (!weatherOn(s)) return;
  var unit = s.WeatherUnit === '1' ? 'celsius' : 'fahrenheit';
  var loc = (s.WeatherLoc || '').trim();
  if (loc) {
    // manual location: geocode once, cache, never touch phone location
    var cached = null;
    try { cached = JSON.parse(localStorage.getItem('md-geocode')); } catch (e) {}
    if (cached && cached.q === loc) return fetchTemp(cached.lat, cached.lon, unit);
    getJSON('https://geocoding-api.open-meteo.com/v1/search?count=1&name=' +
            encodeURIComponent(loc), function (d) {
      if (d && d.results && d.results.length) {
        var r = d.results[0];
        localStorage.setItem('md-geocode',
          JSON.stringify({ q: loc, lat: r.latitude, lon: r.longitude }));
        fetchTemp(r.latitude, r.longitude, unit);
      }
    });
    return;
  }
  if (!weatherOn(settings())) return;   // gate 2, right before the prompt-y path
  navigator.geolocation.getCurrentPosition(function (pos) {
    fetchTemp(pos.coords.latitude, pos.coords.longitude, unit);
  }, function () {}, { timeout: 15000, maximumAge: 30 * 60 * 1000 });
}

Pebble.addEventListener('ready', function () {
  fetchWeather();
  setInterval(fetchWeather, 30 * 60 * 1000);
});

Pebble.addEventListener('webviewclosed', function () {
  setTimeout(fetchWeather, 1500);   // Clay stores the new settings first
});
