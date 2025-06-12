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
                    callback('');
                }
            } catch (e) {
                console.log("❌ JSON parse error:", e.message);
                callback('');
            }
        } else {
            console.log("❌ Login failed: " + xhr.status);
            callback('');
        }
    }, payload, headers);
}

function findStatus(input) {
  input = input.toLowerCase();

  if (input.includes("need more training")) {
    return 1;
  } else if (input.includes("fresh condition")) {
    return 2;
  } else if (input.includes("keeping it steady")) {
    return 3;
  } else if (input.includes("gradually improving")) {
    return 4;
  } else if (input.includes("best training status")) {
    return 5;
  } else if (input.includes("slightly overtrained")) {
    return 6;
  } else if (input.includes("overtraining warning")) {
    return 7;
  } else {
    return 0; // Not found
  }
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
                adviceNumber = findStatus(advice);
                callback(adviceNumber);
            } catch (e) {
                console.log("❌ JSON parse error:", e.message);
                callback(0);
            }
        } else {
            console.log("❌ Dashboard fetch failed: " + xhr.status);
            callback(0);
        }
    }, null, headers);
}

module.exports = RiDuckProvider;
