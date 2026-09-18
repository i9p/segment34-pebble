module.exports = [
  {
    "type": "heading",
    "defaultValue": "segment34-pebble Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "this is a section"
      },
      {
        "type": "slider",
        "messageKey": "SecondsTimeout",
        "defaultValue": 15,
        "label": "Show Seconds Timeout",
        "min": 0,
        "max": 120,
        "step": 5
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color"
      },
      {
        "type": "color",
        "messageKey": "InactiveColor",
        "defaultValue": "0x000055",
        "label": "Inactive Segment/Matrix Color"
      },
      {
        "type": "color",
        "messageKey": "GradientTopColor",
        "defaultValue": "0xFFAA55",
        "label": "Segment Gradient Top Color"
      },
      {
        "type": "color",
        "messageKey": "GradientBottomColor",
        "defaultValue": "0xFF5500",
        "label": "Segment Gradient Bottom Color"
      },
      {
        "type": "color",
        "messageKey": "DateColor",
        "defaultValue": "0xFF5500",
        "label": "Dateline Color"
      },
      {
        "type": "color",
        "messageKey": "SecondaryDateColor",
        "defaultValue": "0x0000FF",
        "label": "Secondary Dateline Color"
      },
      {
        "type": "color",
        "messageKey": "WeatherColor",
        "defaultValue": "0xFFFFFF",
        "label": "Weatherline Color"
      },
      {
        "type": "color",
        "messageKey": "HealthLabelColor",
        "defaultValue": "0xFFFFFF",
        "label": "Health Label Color"
      },
      {
        "type": "color",
        "messageKey": "HealthActiveColor",
        "defaultValue": "0xFF5500",
        "label": "Health Matrix Active Color"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
