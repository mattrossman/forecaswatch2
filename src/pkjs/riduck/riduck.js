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

var RiDuckProvider = function() {
}


RiDuckProvider.prototype.login = function (username, password, callback) {
    var url = "https://rest.riduck.com/api/aam/v2/authenticate";
    var payload = JSON.stringify({
        rememberMe: true,
        username: username,
        password: password,
        issueJWT: true
    });

    var headers = {
        "Content-Type": "application/json",
        "User-Agent": "PebbleKitJS/1.0"
    };

    request(url, "POST", function (xhr) {
        if (xhr.status === 200) {
            try {
                var response = JSON.parse(xhr.responseText);
                var token = response && response.jwt && response.jwt.token;
                if (token) {
                    console.log("✅ Login successful.");
                    callback(token);
                } else {
                    console.log("⚠️ Token not found.");
                }
            } catch (e) {
                console.log("❌ JSON parse error:", e.message);
            }
        } else {
            console.log("❌ Login failed: " + xhr.status);
        }
    }, payload, headers);
}

RiDuckProvider.prototype.fetchAdvice = function (jwtToken, callback) {
    var url = "https://rest.riduck.com/json-api/dashboard.php?order=get_dashboard&days=84";

    var headers = {
        "Authorization": "Bearer " + jwtToken,
        "User-Agent": "PebbleKitJS/1.0"
    };

    request(url, "GET", function (xhr) {
        if (xhr.status === 200) {
            try {
                var data = JSON.parse(xhr.responseText);
                var advice = data &&
                             data.summary_data &&
                             data.summary_data.tss_table &&
                             data.summary_data.tss_table.advice;
                console.log("✅ Dashboard received.");
                console.log("🧠 Advice: " + advice);
                callback(advice);
            } catch (e) {
                console.log("❌ JSON parse error:", e.message);
            }
        } else {
            console.log("❌ Dashboard fetch failed: " + xhr.status);
        }
    }, null, headers);
}

module.exports = RiDuckProvider;
