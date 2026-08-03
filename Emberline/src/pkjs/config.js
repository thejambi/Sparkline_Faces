// Clay settings page — generated locally, no hosted page.
// Select values are string ints matching the enums in src/c/settings.h.
//
// Every description here answers "which one do I want", not "why is it built
// that way" — the reasoning lives in the README. On a phone, anything past two
// lines stops being read, so the long version does not belong here.
//
// Section order follows how often a setting gets touched. Custom colors is
// seven rows most people never open, so it sits at the foot of the page rather
// than between Theme and Layout.
module.exports = [
  {
    type: 'heading',
    defaultValue: 'Emberline'
  },
  {
    type: 'text',
    defaultValue: 'Sky over ground, with one line where they meet. The terrain along the bottom is the last hour of your movement, a column a minute — and last night’s sleep before you get up.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Color' },
      {
        type: 'select',
        messageKey: 'Theme',
        defaultValue: '0',
        label: 'Theme',
        description: 'Paint the sky over land in shadow, let the step count be the terrain.',
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
        description: 'Stacked gives the largest digits the screen allows and a shorter terrain.',
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
        description: 'All of them are drawn one digit at a time into fixed-width slots, so the numerals never shuffle sideways as the minutes change. DSEG7 is a real seven-segment face — it looks its best on Phosphor, against a black sky. LECO is the system face and cannot grow, so it sits much smaller and leaves a taller terrain.',
        options: [
          { label: 'Montserrat — geometric', value: '0' },
          { label: 'Roboto — neutral', value: '2' },
          { label: 'Inter — tall and clean', value: '4' },
          { label: 'Space Grotesk — quirky, and smaller', value: '3' },
          { label: 'Source Sans 3 — humanist', value: '5' },
          { label: 'IBM Plex Mono — engineered', value: '6' },
          { label: 'DSEG7 — seven-segment', value: '7' },
          { label: 'LECO — small, system', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'TextFont',
        defaultValue: '0',
        label: 'Everything else',
        description: 'The face used for the step count, the date column, the pulse and the labels. Inter has the tallest x-height of the three, so it reads largest at the same size; Source Sans 3 is the narrowest.',
        options: [
          { label: 'Montserrat', value: '0' },
          { label: 'Inter', value: '1' },
          { label: 'Source Sans 3', value: '2' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'BoldClock',
        defaultValue: true,
        label: 'Bold clock',
        description: 'Whether to bold the clock face font.'
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
        description: 'For one-line layout. Stacked layout shows weekday and month.',
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
          { label: 'Kilometers', value: '1' },
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
        description: 'Before you get moving, the step slot shows last night’s sleep as 6h 32m. Step count displays once it passes the wake threshold below, and the colors warm at the same moment.'
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
        description: 'While sleep is showing, the terrain draws the night instead of step count.'
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
        description: 'A city or postal code. Left empty, weather follows your phone; filled in, it never reads for your phone’s location at all.',
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
      { type: 'heading', defaultValue: 'Custom colors' },
      {
        type: 'text',
        defaultValue: 'Used only when Theme is set to Custom. The presets tie three of these together on purpose — the newest bar is the clock’s color because it is the time, and the terrain is the step color because both are movement. Here you can cut them loose.'
      },
      { type: 'color', messageKey: 'ColSky', label: 'Sky', defaultValue: 0x000055, sunlight: false },
      { type: 'color', messageKey: 'ColGround', label: 'Ground', defaultValue: 0x000000, sunlight: false },
      { type: 'color', messageKey: 'ColHorizon', label: 'Horizon line', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColInk', label: 'Clock', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColAccent', label: 'Step count', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColTerrain', label: 'Terrain', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColNow', label: 'The newest bar', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColMuted', label: 'Date, weather, pulse', defaultValue: 0xAAAAFF, sunlight: false },
      { type: 'color', messageKey: 'ColSleep', label: 'Sleep', defaultValue: 0xAAAAFF, sunlight: false },
      { type: 'color', messageKey: 'ColScale', label: 'Chart scale and labels', defaultValue: 0x5555AA, sunlight: false },
      { type: 'color', messageKey: 'ColUnlit', label: 'Unlit segments (DSEG7 only)', defaultValue: 0x000055, sunlight: false }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
