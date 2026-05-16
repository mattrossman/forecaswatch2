module.exports = function (minified) {
    clayConfig = this;
    var $ = minified.$;

    /**
     * Parse stored JSON safely.
     *
     * @param {string|null} value Raw JSON string.
     * @returns {Object|null} Parsed object or null.
     */
    function parseStoredJson(value) {
        if (value === null) {
            return null;
        }

        try {
            return JSON.parse(value);
        }
        catch (ex) {
            return null;
        }
    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        var clayFetch;
        var clayOwmApiKey;
        var clayProvider;
        var clayLocation;
        var initProvider;
        var initOwmApiKey;
        var initLocation;
        var lastFetchSuccessString;
        var lastFetchSuccess;
        var date;
        var lastFetchSuccessTime;
        var lastFetchAttemptString;
        var lastFetchAttempt;
        var attemptDate;
        var attemptTime;
        var attemptText;
        var shouldShowLastAttempt;

        clayFetch = clayConfig.getItemByMessageKey('fetch');
        clayFetch.set(false);

        // Save initial states to detect changes to provider
        clayOwmApiKey = clayConfig.getItemByMessageKey('owmApiKey');
        clayProvider = clayConfig.getItemByMessageKey('provider');
        clayLocation = clayConfig.getItemByMessageKey('location');
        initProvider = clayProvider.get();
        initOwmApiKey = clayOwmApiKey.get();
        initLocation = clayLocation.get();

        // Configure default provide section layout
        if (initProvider !== 'openweathermap') {
            clayOwmApiKey.hide()
        }

        // Configure logic for updating the provider section layout
        clayProvider.on('change', function() {
            if (this.get() === 'openweathermap') {
                clayOwmApiKey.show();
            }
            else {
                clayOwmApiKey.hide();
            }
            console.log('Provider set to ' + this.get());
        })

        // Auto-update wind max default when wind unit changes
        var clayWindUnit = clayConfig.getItemByMessageKey('windUnit');
        var clayWindMax = clayConfig.getItemByMessageKey('windMax');
        clayWindUnit.on('change', function() {
            clayWindMax.set(this.get() === 'kph' ? '30' : '20');
        });

        // Show last weather fetch status
        lastFetchSuccessString = clayConfig.meta.userData.lastFetchSuccess;
        lastFetchSuccessTime = null;
        lastFetchSuccess = parseStoredJson(lastFetchSuccessString);
        if (lastFetchSuccess !== null) {
            date = new Date(lastFetchSuccess.time);
            lastFetchSuccessTime = date.getTime();
            $('#lastFetchSpan').ht(date.toLocaleDateString() + ' ' + date.toLocaleTimeString() + ' with ' + lastFetchSuccess.name);
        }

        lastFetchAttemptString = clayConfig.meta.userData.lastFetchAttempt;
        lastFetchAttempt = parseStoredJson(lastFetchAttemptString);
        if (lastFetchAttempt !== null) {
            if (lastFetchAttempt.error) {
                attemptDate = new Date(lastFetchAttempt.time);
                attemptTime = attemptDate.getTime();
                shouldShowLastAttempt = !Boolean(lastFetchSuccessTime) || attemptTime > lastFetchSuccessTime;

                if (shouldShowLastAttempt) {
                    attemptText = '<br>Last failed attempt:<br>';
                    attemptText += attemptDate.toLocaleDateString() + ' ' + attemptDate.toLocaleTimeString() + ' with ' + lastFetchAttempt.name;
                    attemptText += '<br>Error: ' + lastFetchAttempt.error.stage + ': ' + lastFetchAttempt.error.code;
                    $('#lastAttemptBlock').ht(attemptText);
                }
            }
        }

        // Override submit handler to force re-fetch if provider config changed
        $('#main-form').on('submit', function() {
            var returnTo;
            if (clayProvider.get() !== initProvider
                || clayOwmApiKey.get() !== initOwmApiKey
                || clayLocation.get() !== initLocation) {
                clayFetch.set(true);
            }

            // Copied from original handler ($.off requires non-anonymous handler)
            returnTo = window.returnTo || 'pebblejs://close#';
            location.href = returnTo +
                encodeURIComponent(JSON.stringify(clayConfig.serialize()));
        })
    });
};
