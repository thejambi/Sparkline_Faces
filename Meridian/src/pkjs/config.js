// Clay settings page — generated locally, no hosted page.
// Select values are string ints matching the enums in src/c/settings.h.
//
// Every description here answers "which one do I want", not "why is it built
// that way" — the reasoning lives in the README. On a phone, anything past two
// lines stops being read, so the long version does not belong here.
//
// Section order follows how often a setting gets touched. Custom colours is
// seven rows most people never open, so it sits at the foot of the page rather
// than between Theme and Layout.
module.exports = [
  {
    type: 'heading',
    defaultValue: 'Meridian'
  },
  {
    type: 'text',
    defaultValue: 'Sky over ground, with one line where they meet. The terrain along the bottom is the last hour of your movement, a column a minute — and last night’s sleep before you get up.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Colour' },
      {
        type: 'select',
        messageKey: 'Theme',
        defaultValue: '0',
        label: 'Theme',
        description: 'Most are a lit sky over land in shadow, with one bright colour spent on the horizon, the step count and the terrain. Phosphor is the odd one out: no sky at all, the time in orange and everything your body reports in green.',
        options: [
          { label: 'Dusk — navy and amber', value: '0' },
          { label: 'Phosphor — orange and green on black', value: '6' },
          { label: 'Noir — no hue at all', value: '1' },
          { label: 'Paper — light, and the best in sun', value: '2' },
          { label: 'Moss — deep green and chartreuse', value: '3' },
          { label: 'Tide — teal and cyan', value: '4' },
          { label: 'Custom — set at the foot of this page', value: '5' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Clock' },
      {
        type: 'select',
        messageKey: 'Layout',
        defaultValue: '0',
        label: 'Layout',
        description: 'Stacked gives the largest numerals the screen allows and a shorter terrain. One line is smaller and brings back the colon.',
        options: [
          { label: 'Stacked — hours over minutes', value: '0' },
          { label: 'One line — with the colon', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'ClockFont',
        defaultValue: '0',
        label: 'Clock face',
        description: 'Montserrat is geometric, Roboto a shade narrower and more neutral. LECO is the squared-off Pebble face and reaches only two thirds the size — quieter, and it leaves room for a much taller terrain.',
        options: [
          { label: 'Montserrat', value: '0' },
          { label: 'Roboto', value: '2' },
          { label: 'LECO', value: '1' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'BoldClock',
        defaultValue: true,
        label: 'Bold clock',
        description: 'Bold holds up in bright sun; light reads calmer indoors.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Display' },
      {
        type: 'select',
        messageKey: 'DateFormat',
        defaultValue: '0',
        label: 'Date',
        description: 'Stacked has room for the whole date and shows THU over 30 over JUL either way. This picks what the one-line layout shows, where only two parts fit.',
        options: [
          { label: 'Weekday + day', value: '0' },
          { label: 'Month + day', value: '1' },
          { label: 'Off', value: '2' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'LeadingZero',
        defaultValue: false,
        label: 'Leading zero',
        description: 'On a 12-hour clock, 09:41 rather than 9:41.'
      },
      {
        type: 'toggle',
        messageKey: 'ShowBpm',
        defaultValue: true,
        label: 'Heart rate',
        description: 'Turned off, or whenever there is no reading, that slot shows the day’s distance instead.'
      },
      {
        type: 'select',
        messageKey: 'DistUnit',
        defaultValue: '0',
        label: 'Distance units',
        description: 'Automatic follows the units set in the Pebble app.',
        options: [
          { label: 'Automatic', value: '0' },
          { label: 'Kilometres', value: '1' },
          { label: 'Miles', value: '2' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'ShowBattery',
        defaultValue: true,
        label: 'Battery bar',
        description: 'Two pixels along the very top edge, red below 20%.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Sleep' },
      {
        type: 'toggle',
        messageKey: 'ShowSleep',
        defaultValue: true,
        label: 'Show sleep on waking',
        description: 'Before you get moving, the step slot shows last night’s sleep as 6h 32m. Steps take it back once you pass the threshold below, and the colours warm at the same moment.'
      },
      {
        type: 'slider',
        messageKey: 'WakeThreshold',
        defaultValue: 500,
        label: 'Wake threshold',
        description: 'Show sleep until this many steps have been taken today.',
        min: 0,
        max: 2000,
        step: 50
      },
      {
        type: 'toggle',
        messageKey: 'SleepTerrain',
        defaultValue: true,
        label: 'Sleep terrain',
        description: 'While sleep is showing, the terrain draws the night instead — sixty columns from falling asleep to waking, each one how much you moved. First thing in the morning the past hour is mostly empty, which is what this avoids.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Weather' },
      {
        type: 'toggle',
        messageKey: 'WeatherOn',
        defaultValue: true,
        label: 'Show temperature',
        description: 'Top right, from Open-Meteo, refreshed every half hour.'
      },
      {
        type: 'input',
        messageKey: 'WeatherLoc',
        defaultValue: '',
        label: 'Location',
        description: 'A city or postal code. Left empty, weather follows your phone; filled in, it never touches your phone’s location at all.',
        attributes: { placeholder: 'e.g. Minneapolis or 55401' }
      },
      {
        type: 'select',
        messageKey: 'WeatherUnit',
        defaultValue: '0',
        label: 'Units',
        options: [
          { label: 'Fahrenheit', value: '0' },
          { label: 'Celsius', value: '1' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Alerts' },
      {
        type: 'toggle',
        messageKey: 'BTVibe',
        defaultValue: false,
        label: 'Vibrate on disconnect',
        description: 'Never during Quiet Time. Either way a red dot in the top-right corner marks a lost phone.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Custom colours' },
      {
        type: 'text',
        defaultValue: 'Used only when Theme is set to Custom. The watch shows 64 colours — every channel snaps to 00, 55, AA or FF — so what you pick within that grid is what you get.'
      },
      { type: 'color', messageKey: 'ColSky', label: 'Sky', defaultValue: 0x000055, sunlight: false },
      { type: 'color', messageKey: 'ColGround', label: 'Ground', defaultValue: 0x000000, sunlight: false },
      { type: 'color', messageKey: 'ColHorizon', label: 'Horizon line', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColInk', label: 'Clock', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColAccent', label: 'Steps and terrain', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColMuted', label: 'Date, weather, pulse', defaultValue: 0xAAAAFF, sunlight: false },
      { type: 'color', messageKey: 'ColScale', label: 'Chart scale and labels', defaultValue: 0x5555AA, sunlight: false }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
