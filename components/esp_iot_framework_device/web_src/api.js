/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_device/web_src
 * File: api.js
 * Library: esp_iot_framework_device
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

!function(t) {
  "use strict";
  var n = {
    // API
    _addAnticash: function(t) {
      var n = (new Date).getTime();
      return t + (t.indexOf("?") >= 0 ? "&" : "?") + "t=" + n
    },
    _sendRaw: function(n, e, a, o, i) {
      var s;
      try {
        s = t.XMLHttpRequest ? new XMLHttpRequest : new ActiveXObject("Microsoft.XMLHTTP")
      } catch (errInit) {
        return void(i && i("XHR_INIT_ERR"))
      }
      s.open(n, this._addAnticash(e), !0), o && s.setRequestHeader("Content-Type", o), s.onreadystatechange = function() {
        if (4 === s.readyState) {
          var t = 1223 === s.status ? 204 : s.status,
            n = t >= 200 && t < 300,
            e = s.responseText;
          s.onreadystatechange = null, i && i(n ? null : t || "NET_ERR", e)
        }
      };
      try {
        s.send(a || null)
      } catch (errInit) {
        i && i("SEND_ERR")
      }
    },
    _sendJson: function(t, n, e, a) {
      var o = e ? JSON.stringify(e) : null;
      this._sendRaw(t, n, o, "application/json", function(t, n) {
        var e = null;
        if (!t && n) try {
          e = JSON.parse(n)
        } catch (n) {
          t = "JSON_PARSE_ERR"
        }
        a && a(t, e)
      })
    },

    // Web
    sendWithBtn: function(btn, apiFunc, addData, ok_func) {
      if (!btn) return;

      btn.style.backgroundColor = 'yellow';
      btn.style.color = 'red';
      btn.innerHTML = '⏳ Sending...';
      if (btn.disabled) btn.disabled = true;

      apiFunc(function(err, data) {
        if (!err) {
          btn.innerHTML = '⏳ Wait...';
          ok_func(data);
        } else {
          btn.style.backgroundColor = 'red';
          btn.style.color = 'yellow';
          btn.innerHTML = '❌ FAIL (' + err + ')';
          if (btn.disabled) btn.disabled = false;
        }
      }, addData);
    },
    reloadWindow: function(btn) {
      if (btn) {
        btn.disabled = true;
        btn.innerHTML = '♻️ Refreshing...';
        btn.style.backgroundColor = 'grey';
        btn.style.color = 'yellow';
      }

      var baseHost = top.location.protocol + "//" + top.location.host;
      var fileName = window.location.pathname.split("/").pop();
      var baseUri = top.location.pathname + "?p=" + fileName;
      var finalUrl = baseHost + n._addAnticash(baseUri);

      setTimeout(function() { top.location.href = finalUrl }, 5e3)
    },

    // JSON
    formatJson: function(json) {
      var res = JSON.stringify(json, null, 2);

      res = res.replace(/("(\\.|[^"\\])*")/g, function(match) {
          return '<span class="json-string">' + match + '</span>';
      });
      res = res.replace(/\b\d+\b/g, function(match, offset, fullString) {
        var part = fullString.substring(0, offset);
        if (part.lastIndexOf('<span class="json-string">') > part.lastIndexOf('</span>')) return match;
        return '<span class="json-number">' + match + '</span>';
      });
      res = res.replace(/\b(true|false|null)\b/g, function(match, word, offset, fullString) {
        var part = fullString.substring(0, offset);
        if (part.lastIndexOf('<span class="json-string">') > part.lastIndexOf('</span>')) return match;
        return '<span class="json-boolean">' + match + '</span>';
      });
      res = res.replace(/<span class="json-string">("[^"]+")<\/span>\s*:/g, '<span class="json-key">$1</span>:');
     
      return res;
    }
  };
  t.API = n
}(window);