/* SPDX-FileCopyrightText: 2026 AmakeSasha
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/web_src/api.js
 *
 * Copyright 2026 AmakeSasha
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// esp_iot_framework - Shared Web API Core Component
// Copyright 2026 AmakeSasha. Apache-2.0 License.

const API = {
  loadJsonData: function(element, url) {
    const output = document.getElementById(element);

    return fetch(`${url}?t=${Date.now()}`, {
      method: "GET"
    }).then(function(response) {
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }

        return response.json(); 
      }).then(jsonBody => {
        output.innerHTML  = JSON.stringify(jsonBody, null, 2);
        return jsonBody;
      }).catch(function(err) {
        output.innerHTML  = `ERROR: ${err.message || err}`;
        throw err;
      });
  },
  
  sendWithButton: function(url, payloadType, payload, btnName, successText, needTimer = true, time = 3000) {
    const btn = document.getElementById(btnName);
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), time); 
    const options = { 
      method: "POST",
      signal: controller.signal
    };

    btn.disabled = true;
    btn.style.color = "green";
    btn.style.backgroundColor = "yellow";
    btn.innerHTML = "⏳ Processing...";

    if (payload) {
      if (payloadType === "json") {
        options.headers = { "Content-Type": "application/json" };
        options.body = JSON.stringify(payload);
      } else if (payloadType === "file") {
        options.headers = { "Content-Type": "application/octet-stream" };
        options.body = payload;
      }
    }

    return fetch(`${url}?t=${Date.now()}`, options).then(res => {
        clearTimeout(timeoutId); 

        if (!res.ok) throw new Error(`HTTP ${res.status}`);

        btn.style.color = "yellow";
        btn.innerHTML = "✅ Successfully!";
        btn.style.backgroundColor = "LimeGreen";
      }).catch(err => {
        clearTimeout(timeoutId); 

        btn.innerHTML = `❌ Error: ${err.message || err}`;
        btn.style.backgroundColor = "red";
        btn.style.color = "white";
        
        throw err; 
      }).finally(() => {
        btn.disabled = false;
        if (needTimer) {
          setTimeout(() => {
            btn.style.color = "";
            btn.innerHTML = successText;
            btn.style.backgroundColor = "";
          }, 5000);
        }
      });
  },

  reloadWithButton: function(btnName) {
    const btn = document.getElementById(btnName);

    btn.disabled = true;
    btn.style.backgroundColor = 'grey';
    btn.style.color = 'yellow';

    let secondsLeft = 5;
    const timer = setInterval(() => {
      if (secondsLeft > 0) {
        btn.innerHTML = `♻️ Refreshing (${secondsLeft.toFixed(1)}s)...`;
        secondsLeft -= 0.1;
      } else {
        clearInterval(timer);
        btn.innerHTML = '♻️ Refresh...';
        window.location.reload();
      }
    }, 100);
  },
};