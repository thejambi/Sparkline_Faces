// Clay settings page — generated locally, no hosted page.
// Select values are string ints matching the enums in src/c/settings.h.
module.exports = [
  {
    type: 'heading',
    defaultValue: 'Sparkline'
  },
  {
    type: 'text',
    defaultValue: 'The clock, your steps, the date and the past hour of movement — nothing else, and all of it as large as the screen allows.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Time & display' },
      {
        type: 'toggle',
        messageKey: 'ShowHealth',
        defaultValue: true,
        label: 'Health data',
        description: 'Steps, distance and heart rate beside the date, plus the past-hour activity sparkline along the bottom. A shake shows hours slept. Turn off for just the clock and the date.'
      },
      {
        type: 'select',
        messageKey: 'DateFormat',
        defaultValue: '0',
        label: 'Date',
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
        messageKey: 'ShowBattery',
        defaultValue: true,
        label: 'Battery gauge'
      },
      {
        type: 'toggle',
        messageKey: 'ShowBT',
        defaultValue: true,
        label: 'Disconnected indicator'
      },
      {
        type: 'toggle',
        messageKey: 'TapInfo',
        defaultValue: true,
        label: 'Shake gesture',
        description: 'A shake swaps the heart rate for hours slept for a few seconds.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Style' },
      {
        type: 'select',
        messageKey: 'ClockFont',
        defaultValue: '0',
        label: 'Clock font',
        description: 'The size in brackets is the face\u2019s own \u2014 the system families top out well below LECO, so the smaller ones give a quieter clock rather than a scaled-down one. Whichever you pick also sets the step count.',
        options: [
          { label: 'LECO \u2014 tall numerals (60)', value: '0' },
          { label: 'Montserrat (58)', value: '1' },
          { label: 'Roboto (58)', value: '2' },
          { label: 'Bitham (42)', value: '3' },
          { label: 'Roboto, system (49)', value: '4' },
          { label: 'Droid Serif (28)', value: '5' },
          { label: 'Gothic (28)', value: '6' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'BoldFont',
        defaultValue: true,
        label: 'Bold clock',
        description: 'Montserrat and Roboto switch to their Light weight; LECO, Bitham and Gothic to their regular one. Roboto (system) and Droid Serif ship in bold only.'
      },
      {
        type: 'select',
        messageKey: 'Theme',
        defaultValue: '0',
        label: 'Colour theme',
        options: [
          { label: 'Classic \u2014 green on black', value: '0' },
          { label: 'Mono', value: '1' },
          { label: 'Amber', value: '2' },
          { label: 'Ice', value: '3' },
          { label: 'Paper \u2014 light', value: '4' },
          { label: 'Custom', value: '5' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Custom colours' },
      {
        type: 'text',
        defaultValue: 'Used when the colour theme is set to Custom. The watch shows 64 colours, so each channel snaps to 00/55/AA/FF \u2014 picking within that grid is what you see.'
      },
      { type: 'color', messageKey: 'ColBg', label: 'Background', defaultValue: 0x000000, sunlight: false },
      { type: 'color', messageKey: 'ColTime', label: 'Clock (and the newest bar)', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColHealth', label: 'Steps, distance, heart rate', defaultValue: 0x00FF00, sunlight: false },
      { type: 'color', messageKey: 'ColDate', label: 'Day of month', defaultValue: 0xFFFFFF, sunlight: false },
      { type: 'color', messageKey: 'ColMuted', label: 'Weekday and temperature', defaultValue: 0xAAAAAA, sunlight: false },
      { type: 'color', messageKey: 'ColLines', label: 'Rules and chart scale', defaultValue: 0x555555, sunlight: false },
      { type: 'color', messageKey: 'ColSpark', label: 'Sparkline bars', defaultValue: 0xFFAA00, sunlight: false }
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
        description: 'Under the date, from Open-Meteo, refreshed every 30 minutes. Uses phone location unless a manual location is set below.'
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
        description: 'Always silent during Quiet Time.'
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
