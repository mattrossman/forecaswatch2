module.exports = function (minified) {
    var clayConfig = this;
    var $ = minified.$;

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        var btnUpdateWeather = clayConfig.getItemById('btnUpdateWeather');
        btnUpdateWeather.on('click', function() {
            $('#lastFetchSpan').ht('Now!');
        })
    });
};