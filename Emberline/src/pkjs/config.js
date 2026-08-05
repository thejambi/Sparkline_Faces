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
    defaultValue: 'Sky over ground, with the glow of the horizon as the emberline where they meet. The terrain along the bottom is the last hour of your movement — and last night’s sleep before you get up.'
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
        description: 'Stacked gives the largest digits the screen allows, with everything else in two panels.',
        options: [
          { label: 'Stacked — hours over minutes', value: '0' },
          { label: 'One line — with the colon', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'ClockFont',
        defaultValue: '12',
        label: 'Clock face',
        description: 'Customize your clock digit font. DSEG7 is a real seven-segment face and supports unlit segments behind the lit ones. Blocky Digits is drawn for this watchface rather than set in a typeface.',
        options: [
          { label: 'Montserrat — geometric', value: '0' },
          { label: 'Inter — tall and clean', value: '4' },
          { label: 'DSEG7 — seven-segment', value: '7' },
          { label: 'Blocky Digits — drawn for this face', value: '12' }
        ]
      },
      {
        type: 'select',
        messageKey: 'TextFont',
        defaultValue: '0',
        label: 'Everything else',
        description: 'The face used for the step count, the date column, the pulse and the labels.',
        options: [
          { label: 'Montserrat', value: '0' },
          { label: 'DSEG14 — fourteen-segment', value: '5' },
          { label: 'Gothic — the system face', value: '3' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'ShowSep',
        defaultValue: true,
        label: 'Separator rule',
        description: 'A rule line separating the clock from the rest of the face info.'
      },
      {
        type: 'toggle',
        messageKey: 'AutoHide',
        defaultValue: true,
        label: 'Hide the panels',
        description: 'Stacked layout only. The panels stay off screen; shake your wrist to bring them back for seven seconds.'
      },
      {
        type: 'toggle',
        messageKey: 'GrowClock',
        defaultValue: true,
        label: 'Grow the clock',
        description: 'While the panels are hidden, Blocky Digits takes the freed width as size. Off, and the clock keeps its size and simply glides to the middle. Only Blocky Digits can grow.'
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
        description: 'When off, or whenever there is no reading, that slot shows your walking distance instead.'
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
        description: 'Before you get moving, the step slot shows last night’s sleep as 7h 32m. Step count displays once it passes the wake threshold below, and the colors warm at the same moment.'
      },
      {
        type: 'slider',
        messageKey: 'WakeThreshold',
        defaultValue: 350,
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
        description: 'A city or postal code. When empty, weather follows your phone; when filled in, it never reads for your phone’s location.',
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
        defaultValue: 'Used only when Theme is set to Custom. Simple keeps the roles that mean the same thing tied together — the terrain follows the step count, the newest bar follows the clock, the rule follows the chart. Advanced gives full control.'
      },
      {
        type: 'select',
        messageKey: 'ColorMode',
        defaultValue: '0',
        label: 'Detail',
        options: [
          { label: 'Simple — seven colors', value: '0' },
          { label: 'Advanced — all fourteen', value: '1' }
        ]
      },

      { type: 'heading', defaultValue: 'Ground and sky' },
      { type: 'color', messageKey: 'ColSky', label: 'Sky', defaultValue: 0x000055, sunlight: false },
      { type: 'color', messageKey: 'ColGround', label: 'Ground', defaultValue: 0x000000, sunlight: false },
      { type: 'color', messageKey: 'ColHorizon', label: 'Horizon line', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColInfoBg', label: 'Behind the info', defaultValue: 0x000055, sunlight: false },
      { type: 'color', messageKey: 'ColSep', label: 'Separator rule', defaultValue: 0x5555AA, sunlight: false },

      { type: 'heading', defaultValue: 'Time' },
      { type: 'color', messageKey: 'ColInk', label: 'Clock', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColNow', label: 'The newest bar', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColUnlit', label: 'Unlit segments (DSEG7)', defaultValue: 0x000055, sunlight: false },

      { type: 'heading', defaultValue: 'Movement' },
      { type: 'color', messageKey: 'ColAccent', label: 'Step count', defaultValue: 0xFFAA00, sunlight: false },
      { type: 'color', messageKey: 'ColTerrain', label: 'Terrain', defaultValue: 0xFFAA00, sunlight: false },

      { type: 'heading', defaultValue: 'Context' },
      { type: 'color', messageKey: 'ColMuted', label: 'Date, weather, pulse', defaultValue: 0xAAAAFF, sunlight: false },
      { type: 'color', messageKey: 'ColSleep', label: 'Sleep', defaultValue: 0xAAAAFF, sunlight: false },
      { type: 'color', messageKey: 'ColLabel', label: 'Labels and battery', defaultValue: 0x5555AA, sunlight: false },
      { type: 'color', messageKey: 'ColScale', label: 'Chart scale', defaultValue: 0x5555AA, sunlight: false }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
