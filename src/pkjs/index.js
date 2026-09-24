var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

const WMO_CODES = {
  0:  "CLEAR SKY",
  1:  "MAINLY CLEAR",
  2:  "PARTLY CLOUDY",
  3:  "OVERCAST",
  45: "FOG",
  48: "RIME FOG",
  51: "LIGHT DRIZZLE",
  53: "MODERATE DRIZZLE",
  55: "DENSE DRIZZLE",
  56: "LIGHT FREEZING DRIZ",
  57: "DENSE FREEZING DRIZ",
  61: "SLIGHT RAIN",
  63: "MODERATE RAIN",
  65: "HEAVY RAIN",
  66: "LIGHT FREEZING RAIN",
  67: "HEAVY FREEZING RAIN",
  71: "SLIGHT SNOW FALL",
  73: "MODERATE SNOW FALL",
  75: "HEAVY SNOW FALL",
  77: "SNOW GRAINS",
  80: "SLIGHT RAIN SHOWERS",
  81: "MOD RAIN SHOWERS",
  82: "VIOLENT RAIN SHWR",
  85: "SLIGHT SNOW SHOWERS",
  86: "HEAVY SNOW SHOWERS",
  95: "THUNDERSTORM",
  96: "THUNDER + LIGHT HAIL",
  99: "THUNDER + HEAVY HAIL"
};

function getCompassDirection(degrees) {
  const DIRECTIONS = [
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
  ];

  const idx = Math.round(degrees / 22.5) % 16

  return DIRECTIONS[idx];
}

function getWeatherLines(pos) {
  console.log('Getting weather lines');
  var lat = pos.coords.latitude;
  var lon = pos.coords.longitude;

  var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + lat + '&longitude=' + lon + '&daily=precipitation_probability_max&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,snowfall,showers,rain,precipitation,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_gusts_10m,wind_direction_10m,wind_speed_10m&timezone=auto';

  fetch(url).then(response => {
    if (response.ok) {
      return response.json();
    } else {
      throw new Error('API request failed');
    }
  }).then(data => {
    var dictionary = {
      'WEATHERLINE1':
        data.current.temperature_2m + data.current_units.temperature_2m[0] + " " +
        Math.round(data.current.wind_speed_10m) + getCompassDirection(data.current.wind_direction_10m) + " " +
        '(' + data.daily.precipitation_probability_max[0] + '%)',
      'WEATHERLINE2': WMO_CODES[data.current.weather_code],
      'WEATHERLASTUPDATED': Math.floor(Date.now() / 1000)
    };

    console.log('WEATHERLINE1:       ' + dictionary['WEATHERLINE1']);
    console.log('WEATHERLINE2:       ' + dictionary['WEATHERLINE2']);
    console.log('WEATHERLASTUPDATED: ' + dictionary['WEATHERLASTUPDATED']);

    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log('Sent data to Pebble successfully!');
      },
      function(e) {
        console.log('Error sending data to Pebble!');
      }
    );
  }).catch(error => {
    console.log('Error fetching weather data. Status: ' + error)
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    getWeatherLines,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    if (e.payload.GET_WEATHER !== undefined) {
      console.log('Getting location');
      getWeather();
    }
  }
);
