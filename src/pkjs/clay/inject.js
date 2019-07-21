module.exports = function (minified) {
    clayConfig = this;
    var $ = minified.$;

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        clayConfig.getItemByMessageKey('fetch').set(false);

        // Show last weather fetch status
        var lastFetchTime = clayConfig.meta.userData.lastFetchTime;
        if (lastFetchTime !== null) {
            var date = new Date(lastFetchTime);
            $('#lastFetchSpan').ht(date.toLocaleDateString() + ' ' + date.toLocaleTimeString());
        }

        var clayDarkSkyApiKey = clayConfig.getItemByMessageKey('darkSkyApiKey');
        var clayProvider = clayConfig.getItemByMessageKey('provider');
        var oldProviderValue = clayProvider.get();

        // Configure default provide section layout
        if (clayProvider.get() !== 'darksky') {
            clayDarkSkyApiKey.hide()
        }

        // Configure logic for updating the provider section layout
        clayProvider.on('change', function() {
            if (this.get() === 'darksky') {
                clayDarkSkyApiKey.show();
            }
            if (this.get() !== 'darksky' && oldProviderValue === 'darksky') {
                clayDarkSkyApiKey.hide();
            }
            console.log('Provider set to ' + this.get());
            oldProviderValue = this.get();
        })
    });
};