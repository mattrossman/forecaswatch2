module.exports = function (minified) {
    clayConfig = this;
    var $ = minified.$;

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        var clayFetch = clayConfig.getItemByMessageKey('fetch');
        clayFetch.set(false);

        // Save initial states to detect changes to provider
        var clayDarkSkyApiKey = clayConfig.getItemByMessageKey('darkSkyApiKey');
        var clayOwmApiKey = clayConfig.getItemByMessageKey('owmApiKey');
        var clayProvider = clayConfig.getItemByMessageKey('provider');
        var clayLocation = clayConfig.getItemByMessageKey('location');
        var initProvider = clayProvider.get();
        var initDarkSkyApiKey = clayDarkSkyApiKey.get();
        var initOwmApiKey = clayOwmApiKey.get();
        var initLocation = clayLocation.get();

        // Configure default provide section layout
        if (initProvider !== 'darksky') {
            clayDarkSkyApiKey.hide()
        }
        if (initProvider !== 'openweathermap') {
            clayOwmApiKey.hide()
        }

        // Configure logic for updating the provider section layout
        clayProvider.on('change', function() {
            if (this.get() === 'darksky') {
                clayDarkSkyApiKey.show();
            }
            else {
                clayDarkSkyApiKey.hide();
            }
            if (this.get() === 'openweathermap') {
                clayOwmApiKey.show();
            }
            else {
                clayOwmApiKey.hide();
            }
            console.log('Provider set to ' + this.get());
        })

        // Show last weather fetch status
        var lastFetchSuccessString = clayConfig.meta.userData.lastFetchSuccess;
        if (lastFetchSuccessString !== null) {
            var lastFetchSuccess = JSON.parse(lastFetchSuccessString);
            var date = new Date(lastFetchSuccess.time);
            $('#lastFetchSpan').ht(date.toLocaleDateString() + ' ' + date.toLocaleTimeString() + ' with ' + lastFetchSuccess.name);
        }

        // Override submit handler to force re-fetch if provider config changed
        $('#main-form').on('submit', function() {
            if (clayProvider.get() !== initProvider
                || clayDarkSkyApiKey.get() !== initDarkSkyApiKey
                || clayOwmApiKey.get() !== initOwmApiKey
                || clayLocation.get() !== initLocation) {
                clayFetch.set(true);
            }

            // Copied from original handler ($.off requires non-anonymous handler)
            var returnTo = window.returnTo || 'pebblejs://close#';
            location.href = returnTo +
                encodeURIComponent(JSON.stringify(clayConfig.serialize()));
        })
    });
};