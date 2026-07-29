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
