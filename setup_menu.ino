// 2FA Sidecar
// Matt Perkins - Copyright (C) 2025
// Spawned out of the need to often type a lot of two factor authentication
// but still have some security while remaning mostly isolated from the host system.
// See github for 3D models and wiring diagram.
/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

// setup_menu.ino - replacement
// Modern config UI + show current values (mask/hash seeds, show Wi-Fi password, mask PIN)
// By Matt Perkins — Copyright (C) 2026 — GNU General Public Licence (GPLv3+)

#include <mbedtls/sha256.h>   // ESP32 core provides mbedTLS

// ---------- helpers ----------

static String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '"': out += F("&quot;"); break;
      case '\'': out += F("&#39;"); break;
      default: out += c; break;
    }
  }
  return out;
}

static String sha256Hex8(const String &in) {
  if (in.length() == 0) return "";

  uint8_t digest[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0 /*is224=*/);
  mbedtls_sha256_update_ret(&ctx, (const unsigned char*)in.c_str(), in.length());
  mbedtls_sha256_finish_ret(&ctx, digest);
  mbedtls_sha256_free(&ctx);

  const char *hex = "0123456789abcdef";
  char buf[9];
  for (int i = 0; i < 4; i++) {
    buf[i * 2 + 0] = hex[(digest[i] >> 4) & 0xF];
    buf[i * 2 + 1] = hex[digest[i] & 0xF];
  }
  buf[8] = '\0';
  return String(buf);
}

// Masked display for *seeds* with a short fingerprint so you can confirm what's stored
static String seedMaskedWithFingerprint(const String &secret) {
  if (secret.length() == 0) return String(F("<span class='unset'>(not set)</span>"));

  String fp = sha256Hex8(secret);

  String out;
  out.reserve(96);
  out += F("<span class='masked'>********</span> <span class='fp'>(sha256:</span><span class='fpv'>");
  out += fp;
  out += F("</span><span class='fp'>)</span>");
  return out;
}

// PIN display rules:
// - if set: show "***"
// - if blank: show blank (nothing)
static String pinDisplay(const String &pinCur) {
  if (pinCur.length() == 0) return String(""); // blank means blank
  return String(F("<span class='masked'>***</span>"));
}

// Wi-Fi password display: show in clear (per request)
static String wifiPasswordDisplay(const String &passCur) {
  if (passCur.length() == 0) return String(F("<span class='unset'>(not set)</span>"));
  String out;
  out.reserve(passCur.length() + 20);
  out += F("<code>");
  out += htmlEscape(passCur);
  out += F("</code>");
  return out;
}

static bool validPinValue(const String &p) {
  if (p.length() == 0) return true;     // blank => none
  if (p.length() != 4) return false;
  for (int i = 0; i < 4; i++) {
    if (!isDigit(p[i])) return false;
  }
  return true;
}

static String pageHtml(const String &ssidCur,
                       const String &wifiPassCur,
                       const String &pinCur,
                       const String &n1, const String &s1,
                       const String &n2, const String &s2,
                       const String &n3, const String &s3,
                       const String &n4, const String &s4,
                       const String &n5, const String &s5,
                       const String &flashMsg) {
  String h;
  h.reserve(9500);

  h += F(
    "<!doctype html><html lang='en-AU'><head>"
    "<meta charset='utf-8'/>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'/>"
    "<title>2FA-Sidecar Config</title>"
    "<style>"
    ":root{--bg:#0b1020;--panel:#121a33;--panel2:#0f1730;--text:#e8ecff;--muted:#a8b3d6;--border:#25305c;"
    "--accent:#6ee7ff;--accent2:#a78bfa;--good:#34d399;--bad:#fb7185;--warn:#fbbf24;}"
    "*{box-sizing:border-box}"
    "body{margin:0;font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Helvetica,Arial; background:radial-gradient(1200px 800px at 10% 10%, #16204a 0%, var(--bg) 55%); color:var(--text)}"
    ".wrap{max-width:980px;margin:0 auto;padding:24px}"
    ".top{display:flex;align-items:flex-start;justify-content:space-between;gap:16px;flex-wrap:wrap}"
    ".brand{padding:16px 18px;border:1px solid var(--border);background:linear-gradient(180deg,var(--panel),var(--panel2));border-radius:16px;box-shadow:0 10px 30px rgba(0,0,0,.25)}"
    ".brand h1{margin:0 0 6px 0;font-size:20px;letter-spacing:.2px}"
    ".brand p{margin:0;color:var(--muted);font-size:13px;line-height:1.35}"
    ".grid{display:grid;grid-template-columns:1.1fr .9fr;gap:16px;margin-top:16px}"
    "@media (max-width:900px){.grid{grid-template-columns:1fr}}"
    ".card{border:1px solid var(--border);background:linear-gradient(180deg,var(--panel),var(--panel2));border-radius:16px;box-shadow:0 10px 30px rgba(0,0,0,.25)}"
    ".card .hd{padding:14px 16px;border-bottom:1px solid var(--border)}"
    ".card .hd h2{margin:0;font-size:15px}"
    ".card .bd{padding:16px}"
    ".msg{margin:0 0 12px 0;padding:10px 12px;border-radius:12px;border:1px solid var(--border);background:rgba(255,255,255,.04);color:var(--text)}"
    ".msg.ok{border-color:rgba(52,211,153,.35)}"
    ".msg.bad{border-color:rgba(251,113,133,.35)}"
    ".kv{width:100%;border-collapse:separate;border-spacing:0 10px}"
    ".kv td{vertical-align:top}"
    ".k{color:var(--muted);width:36%;padding-right:10px;font-size:13px}"
    ".v{font-size:13px}"
    ".unset{color:var(--warn)}"
    ".masked{font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,'Liberation Mono','Courier New',monospace;letter-spacing:1px;color:#fff}"
    ".fp{color:var(--muted);font-size:12px}"
    ".fpv{font-family:ui-monospace,monospace;color:#dbe4ff}"
    "form{display:block;margin-top:10px}"
    ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
    "@media (max-width:700px){.row{grid-template-columns:1fr}}"
    "label{display:block;color:var(--muted);font-size:12px;margin:10px 0 6px}"
    "input{width:100%;padding:11px 12px;border-radius:12px;border:1px solid var(--border);background:rgba(0,0,0,.22);color:var(--text);outline:none}"
    "input:focus{border-color:rgba(110,231,255,.55);box-shadow:0 0 0 3px rgba(110,231,255,.12)}"
    ".hint{color:var(--muted);font-size:12px;margin-top:6px}"
    ".btns{display:flex;gap:10px;flex-wrap:wrap;margin-top:14px}"
    "button{border:1px solid var(--border);background:linear-gradient(180deg, rgba(110,231,255,.18), rgba(167,139,250,.14));color:var(--text);padding:10px 14px;border-radius:12px;cursor:pointer;font-weight:600}"
    "button:hover{border-color:rgba(110,231,255,.55)}"
    ".ghost{background:rgba(255,255,255,.03)}"
    ".danger{background:linear-gradient(180deg, rgba(251,113,133,.20), rgba(251,113,133,.08))}"
    ".foot{margin-top:14px;color:var(--muted);font-size:12px}"
    "code{font-family:ui-monospace,Menlo,Consolas,monospace;color:#dbe4ff}"
    "</style></head><body><div class='wrap'>"
  );

  // Header / branding with version + licence note
  h += F("<div class='top'><div class='brand'>"
         "<h1>2FA-Sidecar v");
  h += htmlEscape(String(mainver));
  h += F("</h1>"
         "<p>By Matt Perkins<br>"
         "Copyright &copy; 2026<br>"
         "GNU General Public Licence (GPLv3+)"
         "</p>"
         "</div></div>");

  if (flashMsg.length()) {
    h += F("<div class='msg ok'>");
    h += flashMsg;
    h += F("</div>");
  }

  h += F("<div class='grid'>");

  // ---- left: current config ----
  h += F("<div class='card'><div class='hd'><h2>Current settings</h2></div><div class='bd'>");
  h += F("<table class='kv'>");

  h += F("<tr><td class='k'>Wi-Fi SSID</td><td class='v'><code>");
  h += htmlEscape(ssidCur);
  h += F("</code></td></tr>");

  h += F("<tr><td class='k'>Wi-Fi password</td><td class='v'>");
  h += wifiPasswordDisplay(wifiPassCur);
  h += F("</td></tr>");

  h += F("<tr><td class='k'>Access PIN</td><td class='v'>");
  h += pinDisplay(pinCur);
  h += F("</td></tr>");

  // NOTE: blank labels should remain blank (no "(unnamed)")
  auto addKeyRow = [&](int idx, const String &name, const String &seed) {
    h += F("<tr><td class='k'>2FA key ");
    h += String(idx);
    h += F("</td><td class='v'><div><code>");

    if (name.length()) {
      h += htmlEscape(name);
    } else {
      h += F("&nbsp;"); // intentionally blank label
    }

    h += F("</code></div><div class='hint'>Seed: ");
    h += seedMaskedWithFingerprint(seed);
    h += F("</div></td></tr>");
  };

  addKeyRow(1, n1, s1);
  addKeyRow(2, n2, s2);
  addKeyRow(3, n3, s3);
  addKeyRow(4, n4, s4);
  addKeyRow(5, n5, s5);

  h += F("</table>");
  h += F("<div class='foot'>Tip: leaving a seed field blank keeps the existing value.</div>");
  h += F("</div></div>");

  // ---- right: edit form ----
  h += F("<div class='card'><div class='hd'><h2>Update settings</h2></div><div class='bd'>");
  h += F("<form method='post' action='/save'>");

  h += F("<label for='ssid'>Wi-Fi SSID</label>");
  h += F("<input id='ssid' name='ssid' type='text' value='");
  h += htmlEscape(ssidCur);
  h += F("'/>");

  h += F("<label for='password'>Wi-Fi password <span class='hint'>(leave blank to keep existing)</span></label>");
  h += F("<input id='password' name='password' type='password' value='' autocomplete='off'/>");

  h += F("<label for='pin'>Access PIN (4 digits, blank = none) <span class='hint'>(leave blank to keep existing)</span></label>");
  h += F("<input id='pin' name='pin' type='password' inputmode='numeric' pattern='[0-9]*' value='' autocomplete='off'/>");

  h += F("<div class='row'>");

  auto addNameSeedInputs = [&](int idx, const char *nameField, const char *seedField, const String &curName) {
    h += F("<div>");
    h += F("<label for='");
    h += nameField;
    h += F("'>2FA key ");
    h += String(idx);
    h += F(" name</label><input id='");
    h += nameField;
    h += F("' name='");
    h += nameField;
    h += F("' type='text' value='");
    h += htmlEscape(curName);
    h += F("'/></div>");

    h += F("<div>");
    h += F("<label for='");
    h += seedField;
    h += F("'>2FA key ");
    h += String(idx);
    h += F(" seed <span class='hint'>(leave blank to keep existing)</span></label>"
           "<input id='");
    h += seedField;
    h += F("' name='");
    h += seedField;
    h += F("' type='password' value='' autocomplete='off'/>"
           "<div class='hint'>Seeds are never shown in clear; use the fingerprint under Current settings.</div>"
           "</div>");
  };

  addNameSeedInputs(1, "tfa_name_1", "tfa_seed_1", n1);
  addNameSeedInputs(2, "tfa_name_2", "tfa_seed_2", n2);
  addNameSeedInputs(3, "tfa_name_3", "tfa_seed_3", n3);
  addNameSeedInputs(4, "tfa_name_4", "tfa_seed_4", n4);
  addNameSeedInputs(5, "tfa_name_5", "tfa_seed_5", n5);

  h += F("</div>"); // row

  h += F("<div class='btns'>"
         "<button type='submit'>Save settings</button>"
         "<button class='ghost' type='button' onclick=\"window.location.href='/'\">Refresh</button>"
         "<button class='danger' type='button' onclick=\"if(confirm('Reboot the Sidecar now?')) window.location.href='/reboot'\">Reboot</button>"
         "</div>");

  h += F("</form>");

  h += F("<div class='foot'>"
         "Seeds are masked and fingerprinted. "
         "Wi-Fi password is displayed for convenience. "
         "PIN is shown as *** when set."
         "</div>");

  h += F("</div></div>"); // card

  h += F("</div>"); // grid
  h += F("</div></body></html>");

  return h;
}

// ---------- entry points (test + setup AP) ----------

void setup_test() {
  // Simple screen test
  tft.fillScreen(ST77XX_RED);   delay(300);
  tft.fillScreen(ST77XX_GREEN); delay(300);
  tft.fillScreen(ST77XX_BLUE);  delay(300);
  tft.fillScreen(ST77XX_BLACK);

  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(3, 18);  tft.print("Key 1");
  tft.setCursor(3, 38);  tft.print("Key 2");
  tft.setCursor(3, 58);  tft.print("Key 3");
  tft.setCursor(3, 78);  tft.print("Key 4");
  tft.setCursor(3, 98);  tft.print("Key 5");

  uint8_t seen = 0;

  while (1) {
    key1.update(); key2.update(); key3.update(); key4.update(); key5.update();

    if (key1.isClick() && !(seen & (1 << 0))) { seen |= (1 << 0); tft.setCursor(65, 18); tft.print("OK"); }
    if (key2.isClick() && !(seen & (1 << 1))) { seen |= (1 << 1); tft.setCursor(65, 38); tft.print("OK"); }
    if (key3.isClick() && !(seen & (1 << 2))) { seen |= (1 << 2); tft.setCursor(65, 58); tft.print("OK"); }
    if (key4.isClick() && !(seen & (1 << 3))) { seen |= (1 << 3); tft.setCursor(65, 78); tft.print("OK"); }
    if (key5.isClick() && !(seen & (1 << 4))) { seen |= (1 << 4); tft.setCursor(65, 98); tft.print("OK"); }

    uint8_t required_mask = (maxkeys == 15) ? 0x1F : 0x03; // 5-key vs 2-key default
    if ((seen & required_mask) == required_mask) {
      tft.setCursor(77, 58);
      tft.print("Pass");
      delay(1200);
      break;
    }

    delay(10);
  }

  wifi_setup();
}

void wifi_setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str()); // keep behaviour (open AP) per your current setup

  IPAddress apip = WiFi.softAPIP();

  // Reduce newlines so it fits the 240x135 display (rotation 3).
  // Use a smaller font and start at the top-left.
  tft.fillScreen(ST77XX_RED);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(0, 14);

  // Keep it short: version, SSID, URL
  tft.printf("2FA-Sidecar v%s\n", mainver);
  tft.printf("By Matt Perkins\n");
  tft.printf("SSID: %s\n", ssid.c_str());
  tft.printf("Browse: http://%d.%d.%d.%d\n", apip[0], apip[1], apip[2], apip[3]);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.begin("2FA_Sidecar", true);

    String ssidCur = preferences.getString("ssid", "");
    String passCur = preferences.getString("password", "");
    String pinCur  = preferences.getString("pin", "");

    String n1 = preferences.getString("tfa_name_1", "");
    String s1 = preferences.getString("tfa_seed_1", "");
    String n2 = preferences.getString("tfa_name_2", "");
    String s2 = preferences.getString("tfa_seed_2", "");
    String n3 = preferences.getString("tfa_name_3", "");
    String s3 = preferences.getString("tfa_seed_3", "");
    String n4 = preferences.getString("tfa_name_4", "");
    String s4 = preferences.getString("tfa_seed_4", "");
    String n5 = preferences.getString("tfa_name_5", "");
    String s5 = preferences.getString("tfa_seed_5", "");

    preferences.end();

    String flash = "";
    if (request->hasParam("msg")) {
      flash = htmlEscape(request->getParam("msg")->value());
    }

    String html = pageHtml(ssidCur, passCur, pinCur, n1, s1, n2, s2, n3, s3, n4, s4, n5, s5, flash);
    request->send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    preferences.begin("2FA_Sidecar", false);

    auto getPost = [&](const char *name) -> String {
      if (request->hasParam(name, true)) {
        String v = request->getParam(name, true)->value();
        v.trim();
        return v;
      }
      return "";
    };

    String newSsid = getPost("ssid");
    if (newSsid.length()) preferences.putString("ssid", newSsid);

    // Secrets: blank keeps existing
    String newPass = getPost("password");
    if (newPass.length()) preferences.putString("password", newPass);

    String newPin = getPost("pin");
    if (newPin.length()) {
      if (!validPinValue(newPin)) {
        preferences.end();
        request->send(400, "text/html",
                      "Invalid PIN. Must be exactly 4 digits, or blank.<br><a href='/'>Back</a>");
        return;
      }
      preferences.putString("pin", newPin);
    }

    // 2FA names: overwrite if provided (allow blank to intentionally clear)
    // To allow clearing, we check hasParam() rather than length().
    auto maybePutStringAllowBlank = [&](const char *key) {
      if (request->hasParam(key, true)) {
        String v2 = request->getParam(key, true)->value();
        v2.trim();
        preferences.putString(key, v2); // can be blank on purpose
      }
    };

    maybePutStringAllowBlank("tfa_name_1");
    maybePutStringAllowBlank("tfa_name_2");
    maybePutStringAllowBlank("tfa_name_3");
    maybePutStringAllowBlank("tfa_name_4");
    maybePutStringAllowBlank("tfa_name_5");

    // 2FA seeds: blank keeps existing (do NOT allow clearing by blank)
    String v;
    v = getPost("tfa_seed_1"); if (v.length()) preferences.putString("tfa_seed_1", v);
    v = getPost("tfa_seed_2"); if (v.length()) preferences.putString("tfa_seed_2", v);
    v = getPost("tfa_seed_3"); if (v.length()) preferences.putString("tfa_seed_3", v);
    v = getPost("tfa_seed_4"); if (v.length()) preferences.putString("tfa_seed_4", v);
    v = getPost("tfa_seed_5"); if (v.length()) preferences.putString("tfa_seed_5", v);

    preferences.end();

    request->redirect("/?msg=Saved%20settings.");
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "Rebooting...<br>");
    delay(250);
    ESP.restart();
  });

  server.onNotFound(notFound);
  server.begin();

  // 10 minute setup window then reboot
  unsigned long start = millis();
  while (millis() - start < 600000UL) {
    key5.update();
    if (key5.isLongClick()) ESP.restart();
    delay(10);
  }
  ESP.restart();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
