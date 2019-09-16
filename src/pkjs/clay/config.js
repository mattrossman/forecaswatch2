var meta = require('../../../package.json');
module.exports = [
    {
        "type": "heading",
        "defaultValue": "ForecasWatch2"
    },
    { 
      "type": "text", 
      "defaultValue": "Contribute on <a href=\"https://github.com/mattrossman/forecaswatch2\">GitHub!</a>" 
    },
    {
        "type": "section",
        "items": [
            {
              "type": "heading",
              "defaultValue": "Weather"
            },
            {
                "type": "radiogroup",
                "label": "Provider",
                "messageKey": "provider",
                "defaultValue": "wunderground",
                "options": [
                    {
                        "label": "Weather Underground",
                        "value": "wunderground"
                    },
                    {
                        "label": "Dark Sky",
                        "value": "darksky"
                    }
                ]
            },
            {
                "type": "input",
                "label": "Dark Sky API key",
                "messageKey": "darkSkyApiKey",
                "description": "<a href='https://darksky.net/dev/register'>Register a Dark Sky developer account</a> and paste your secret key here"
            },
            {
                "type": "toggle",
                "label": "Force weather fetch",
                "messageKey": "fetch",
                "description": "Last successful fetch:<br><span id='lastFetchSpan'>Never :(</span>"
            }
        ]
    },
    {
        "type": "submit",
        "defaultValue": "Save Settings"
    },
    { 
        "type": "text", 
        "defaultValue": "v" + meta.version 
    }
]