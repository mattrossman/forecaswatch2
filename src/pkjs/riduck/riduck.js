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
    return 1000;
  } else if (input.includes("fresh condition")) {
    return 2000;
  } else if (input.includes("keeping it steady")) {
    return 3000;
  } else if (input.includes("gradually improving")) {
    return 4000;
  } else if (input.includes("best training status")) {
    return 5000;
  } else if (input.includes("slightly overtrained")) {
    return 6000;
  } else if (input.includes("overtraining warning")) {
    return 7000;
  } else {
    return 0; // Not found
  }
}

function getIntervalPercentage(arr, value) {
    for (let i = 0; i < arr.length - 1; i++) {
      const start = arr[i] + 1;
      const end = arr[i + 1];
  
      if (value >= start && value <= end) {
        const range = end - start;
        const position = value - start;
        const percentage = (position / range) * 100;
        return Math.ceil(percentage); // Round up to the next integer
      }
    }
    return 0; // Value doesn't fall into any interval
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
                var advice_range = data &&
                             data.summary_data &&
                             data.summary_data.tss_table &&
                             data.summary_data.tss_table.range_tss;
                var current_tss = data &&
                             data.summary_data &&
                             data.summary_data.tss_table &&
                             data.summary_data.tss_table.last_week_tss;
                console.log("✅ Dashboard received.");
                console.log("Advice ranges:", advice_range);
                var advice_percentage = getIntervalPercentage(advice_range, current_tss);
                console.log("Advice percentage:", advice_percentage);
                console.log("Current tss:", current_tss);
                console.log("🧠 Advice: " + advice);
                adviceNumber = findStatus(advice);
                callback(adviceNumber+advice_percentage);
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
