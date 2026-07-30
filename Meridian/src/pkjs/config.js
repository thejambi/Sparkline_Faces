// Clay settings page — generated locally, no hosted page.
// Select values are string ints matching the enums in src/c/settings.h.
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
        defaultValue: '6',
        label: 'Theme',
        description: 'Most are built the same way: a lit sky over ground in shadow, and exactly one bright colour, spent only on the horizon, the step count and the terrain. Phosphor is the exception — no sky at all, the time in orange and everything your body reports in green.',
        options: [
          { label: 'Phosphor — orange and green on black', value: '6' },
          { label: 'Dusk — navy and amber', value: '0' },
          { label: 'Noir — no hue at all', value: '1' },
          { label: 'Paper — light', value: '2' },
          { label: 'Moss', value: '3' },
          { label: 'Tide', value: '4' },
          { label: 'Custom', value: '5' }
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
        description: 'Stacked puts the hours over the minutes and buys a much larger numeral \u2014 88px against 60 \u2014 paying for it with a shorter terrain. It runs the steps and pulse along the top and gives the right margin to a narrow day column, which is what leaves the clock room to be that size. One line buys back the colon, which sits dead centre, and swaps the two groups over: date and temperature on the left, steps and pulse on the right.',
        options: [
          { label: 'Stacked \u2014 biggest clock', value: '0' },
          { label: 'One line \u2014 with the colon', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'ClockFont',
        defaultValue: '0',
        label: 'Clock face',
        description: 'All three are drawn into fixed-width slots, so the digits never shuffle sideways as the minutes change. Montserrat is geometric and Roboto is a touch narrower and more neutral; stacked, both reach 88px. LECO is a system face and tops out at 60, so it sits noticeably smaller \u2014 quieter, and more instrument than poster, and it leaves a much taller terrain.',
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
        description: 'Light reads calmer at this size; bold survives brighter sun.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Custom colours' },
      {
        type: 'text',
        defaultValue: 'Used when the theme is set to Custom. The watch shows 64 colours, so each channel snaps to 00/55/AA/FF — picking within that grid is what you get.'
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
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Sleep' },
      {
        type: 'toggle',
        messageKey: 'ShowSleep',
        defaultValue: true,
        label: 'Show sleep on waking',
        description: 'Before you get moving, the step slot shows last night’s sleep as 6h 32m instead. Steps take it back once you pass the wake threshold, and the face warms up at the same moment.'
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
        description: 'While sleep is showing, the terrain draws last night rather than the past hour — sixty columns from the moment you fell asleep to the moment you woke, each one how much you moved. Off, it keeps showing the past hour, which first thing in the morning is mostly empty.'
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
        description: 'Only the one-line layout has to choose: two of the three parts crowd its top row. The stacked layout sets the date as a column and shows all three — THU over 30 over JUL — whichever is picked here. Off hides it in both.',
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
        label: 'Leading zero (12h clock)'
      },
      {
        type: 'toggle',
        messageKey: 'ShowBpm',
        defaultValue: true,
        label: 'Heart rate',
        description: 'Off, or when no reading is available, the slot shows the day’s distance instead.'
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
        description: 'A two-pixel bar along the very top edge. It turns red below 20%.'
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
        label: 'Current temperature',
        description: 'Top right, from Open-Meteo, refreshed every 30 minutes. Uses phone location unless a manual location is set below.'
      },
      {
        type: 'input',
        messageKey: 'WeatherLoc',
        defaultValue: '',
        label: 'Location (city or postal code)',
        description: 'When filled in, weather uses this place and never touches phone location.',
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
        description: 'Always silent during Quiet Time. A red dot in the top-right corner marks the disconnection either way.'
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
