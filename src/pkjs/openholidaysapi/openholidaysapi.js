function request(url, type, callback, body = null, headers = {}) {
    var xhr = new XMLHttpRequest();
    xhr.open(type, url, true);

    for (var key in headers) {
        xhr.setRequestHeader(key, headers[key]);
    }

    xhr.onload = function () {
        callback(this);
    };

    xhr.onerror = function () {
        console.log("❌ Request error");
    };

    xhr.send(body);
}

var OpenHolidaysProvider = function() {
};

OpenHolidaysProvider.prototype.getHolidayBitmask = function (countryIsoCode, regionIsoCode, callback) {
    const today = new Date();
    const validFrom = today.toISOString().split('T')[0];

    const futureDate = new Date();
    futureDate.setDate(today.getDate() + 6);
    const validTo = futureDate.toISOString().split('T')[0];

    const url = `https://openholidaysapi.org/PublicHolidays?countryIsoCode=${countryIsoCode}&regionIsoCode=${regionIsoCode}&validFrom=${validFrom}&validTo=${validTo}`;

    const headers = {
        "Accept": "application/json"
    };

    request(url, "GET", function (xhr) {
        if (xhr.status === 200) {
            try {
                const holidays = JSON.parse(xhr.responseText);
                let bitmask = 0;

                for (let i = 0; i < 7; i++) {
                    const checkDate = new Date(today);
                    checkDate.setDate(today.getDate() + i);
                    const checkDateStr = checkDate.toISOString().split('T')[0];

                    const isHoliday = holidays.some(h => h.date === checkDateStr);
                    if (isHoliday) {
                        bitmask |= (1 << i);
                    }
                }

                console.log("✅ Holiday bitmask:", bitmask);
                callback(bitmask);
            } catch (e) {
                console.log("❌ JSON parse error:", e.message);
                callback(0);
            }
        } else {
            console.log("❌ Holiday fetch failed: " + xhr.status);
            callback(0);
        }
    }, null, headers);
};

module.exports = OpenHolidaysProvider;
