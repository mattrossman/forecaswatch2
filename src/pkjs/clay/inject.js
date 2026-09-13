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

    /**
     * Show only the slots this watch has room for, and hide them all when the
     * calendar is selected.
     *
     * @param {Array} slotItems Clay items for statSlot1..6 (may contain nulls).
     * @param {Object|null} layoutItem Clay text item explaining the grid layout.
     * @param {string} mode Current topBand value.
     * @param {number} slotCount Slots the watch can display.
     * @returns {void}
     */
    function applyTopBandMode(slotItems, layoutItem, mode, slotCount) {
        if (layoutItem) {
            if (mode === 'stats') {
                layoutItem.show();
            }
            else {
                layoutItem.hide();
            }
        }

        slotItems.forEach(function(item, index) {
            if (!item) {
                return;
            }

            if (mode !== 'stats' || index >= slotCount) {
                item.hide();
            }
            else {
                item.show();
            }
        });
    }

    /**
     * Grey out the Calendar section while the stats grid replaces the calendar.
     * Disabled items keep their values, so switching back restores them.
     *
     * @param {Array} calendarItems Clay items only the calendar uses (may contain nulls).
     * @param {Object|null} headingItem The Calendar section heading.
     * @param {string} mode Current topBand value.
     * @returns {void}
     */
    function applyCalendarActive(calendarItems, headingItem, mode) {
        var active = mode !== 'stats';

        if (headingItem) {
            headingItem.set(active ? 'Calendar' : 'Calendar (not shown with health stats)');
        }

        calendarItems.forEach(function(item) {
            if (!item) {
                return;
            }

            if (active) {
                item.enable();
            }
            else {
                item.disable();
            }
        });
    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        var clayFetch;
        var clayTopBand;
        var calendarItems;
        var clayCalendarHeading;
        var slotItems;
        var statsCaps;
        var slotCount;
        var watchPlatform;
        var clayStatsLayout;
        var clayOwmApiKey;
        var clayProvider;
        var clayProviderDescription;
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

        // Every lookup is null-guarded: on aplite the whole Top band section is
        // capability-gated away, so these items are never created.
        statsCaps = parseStoredJson(clayConfig.meta.userData.statsCaps);
        watchPlatform = clayConfig.meta.activeWatchInfo
            ? clayConfig.meta.activeWatchInfo.platform
            : null;
        // Trust the watch's own report only when it came from this platform;
        // otherwise fall back to the grid size of the platform in hand.
        if (statsCaps && typeof statsCaps.slots === 'number'
                && !(watchPlatform && statsCaps.platform && statsCaps.platform !== watchPlatform)) {
            slotCount = statsCaps.slots;
        }
        else {
            slotCount = watchPlatform === 'emery' ? 6 : 4;
        }
        slotItems = [1, 2, 3, 4, 5, 6].map(function(index) {
            return clayConfig.getItemByMessageKey('statSlot' + index);
        });
        clayStatsLayout = clayConfig.getItemById('statsGridLayout');
        if (clayStatsLayout) {
            clayStatsLayout.set(slotCount === 6 ? '3x2 grid' : '2x2 grid');
        }
        calendarItems = ['weekStartDay', 'firstWeek', 'colorToday', 'colorSunday', 'colorSaturday', 'colorUSFederal']
            .map(function(messageKey) {
                return clayConfig.getItemByMessageKey(messageKey);
            });
        clayCalendarHeading = clayConfig.getItemById('calendarHeading');
        clayTopBand = clayConfig.getItemByMessageKey('topBand');
        if (clayTopBand) {
            applyTopBandMode(slotItems, clayStatsLayout, clayTopBand.get(), slotCount);
            applyCalendarActive(calendarItems, clayCalendarHeading, clayTopBand.get());
            clayTopBand.on('change', function() {
                applyTopBandMode(slotItems, clayStatsLayout, this.get(), slotCount);
                applyCalendarActive(calendarItems, clayCalendarHeading, this.get());
            });
        }

        // Save initial states to detect changes to provider
        clayOwmApiKey = clayConfig.getItemByMessageKey('owmApiKey');
        clayProvider = clayConfig.getItemByMessageKey('provider');
        clayProviderDescription = clayProvider.$element.select('.description');
        clayLocation = clayConfig.getItemByMessageKey('location');
        initProvider = clayProvider.get();
        initOwmApiKey = clayOwmApiKey.get();
        initLocation = clayLocation.get();

        // Configure default provide section layout
        if (initProvider !== 'openweathermap') {
            clayOwmApiKey.hide()
        }
        if (initProvider !== 'openmeteo') {
            clayProviderDescription.set('$display', 'none');
        }

        // Configure logic for updating the provider section layout
        clayProvider.on('change', function() {
            if (this.get() === 'openweathermap') {
                clayOwmApiKey.show();
            }
            else {
                clayOwmApiKey.hide();
            }
            if (this.get() === 'openmeteo') {
                clayProviderDescription.set('$display', '');
            }
            else {
                clayProviderDescription.set('$display', 'none');
            }
            console.log('Provider set to ' + this.get());
        })

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
