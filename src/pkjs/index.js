// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function (e) {
        console.log('PebbleKit JS ready!');
    }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
    function (e) {
        console.log('AppMessage received!');
    }                     
);