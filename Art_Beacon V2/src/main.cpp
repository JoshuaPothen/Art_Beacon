#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

// Access Point credentials
const char* ssid = "🎨 Art Beacon Portal";
const char* password = "";  // Open network for better accessibility

// Network configuration
IPAddress apIP(192, 168, 4, 1);
IPAddress netMask(255, 255, 255, 0);

// Create WebServer and DNS Server objects
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Variables to store uploaded text
String uploadedTexts[100]; // Store up to 100 text messages
int textCount = 0;

// Function declarations
void handleRoot();
void handleUpload();
void handleTextList();
void handleCSS();
void handleNotFound();
void handleCaptivePortal();
void handleAPI();
String getWebPage();
String getContentType(String filename);
bool handleFileRead(String path);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🎨 Starting ESP32 Art Beacon Portal...");
  
  // Initialize SPIFFS for file system
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Set WiFi mode and configure Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(ssid, password, 6, 0, 8); // Channel 6, no password, max 8 connections
  
  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Initialize mDNS
  if (!MDNS.begin("artbeacon")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }
  
  Serial.println("Art Beacon Portal Ready!");
  Serial.print("Access Point: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("📱 Connect to see messages from fellow travelers!");
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/texts", handleTextList);
  server.on("/api/texts", handleAPI);
  server.on("/style.css", handleCSS);
  server.on("/captive", handleCaptivePortal);
  
  // Captive portal - redirect all unknown requests
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      handleCaptivePortal();
    }
  });
  
  // Start the web server
  server.begin();
  Serial.println("✨ Web server and captive portal active!");
}

void loop() {
  // Process DNS requests for captive portal
  dnsServer.processNextRequest();
  
  // Handle web server requests
  server.handleClient();
  
  delay(10);
}

void handleRoot() {
  String html = getWebPage();
  server.send(200, "text/html", html);
}

void handleUpload() {
  if (server.hasArg("textInput")) {
    String newText = server.arg("textInput");
    
    if (newText.length() > 0 && textCount < 100) {
      uploadedTexts[textCount] = newText;
      textCount++;
      
      Serial.println("New text uploaded: " + newText);
      
      String successHTML = "<!DOCTYPE html><html><head>";
      successHTML += "<title>✨ Message Shared - Art Beacon</title>";
      successHTML += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      successHTML += "<meta http-equiv='refresh' content='3;url=/'>";
      successHTML += "<link rel='stylesheet' href='/style.css'></head>";
      successHTML += "<body><div class='container'>";
      successHTML += "<header><h1>✨ Message Shared!</h1>";
      successHTML += "<p class='subtitle'>Your words have joined the digital constellation</p></header>";
      successHTML += "<div style='padding:40px;text-align:center;'>";
      successHTML += "<h2 style='color:var(--success);margin-bottom:20px;'>🎉 Success!</h2>";
      successHTML += "<p>Your message has been added to the portal.</p>";
      successHTML += "<p><small>Redirecting in 3 seconds...</small></p>";
      successHTML += "</div>";
      successHTML += "<div class='action-buttons'>";
      successHTML += "<a href='/' class='button primary'>🏠 Back to Portal</a> ";
      successHTML += "<a href='/texts' class='button'>📜 View All Messages</a></div>";
      successHTML += "</div></body></html>";
      
      server.send(200, "text/html", successHTML);
    } else {
      server.send(400, "text/html", 
        "<html><head><title>Upload Error</title><link rel='stylesheet' href='/style.css'></head>"
        "<body><div class='container'><h2>Upload Error</h2>"
        "<p>Text is empty or storage is full.</p>"
        "<a href='/' class='button'>Try Again</a></div></body></html>");
    }
  } else {
    server.send(400, "text/html", 
      "<html><head><title>Upload Error</title><link rel='stylesheet' href='/style.css'></head>"
      "<body><div class='container'><h2>Upload Error</h2>"
      "<p>No text data received.</p>"
      "<a href='/' class='button'>Try Again</a></div></body></html>");
  }
}

void handleTextList() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>📜 Messages - Art Beacon Portal</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta charset='UTF-8'>";
  html += "<link rel='stylesheet' href='/style.css'>";
  html += "</head><body><div class='container'>";
  
  html += "<header>";
  html += "<h1>📜 Portal Messages</h1>";
  html += "<p class='subtitle'>Messages from fellow travelers • " + String(textCount) + " total</p>";
  html += "</header>";
  
  if (textCount == 0) {
    html += "<div style='padding:40px;text-align:center;color:#666;'>";
    html += "<h2>🌟 Be the First!</h2>";
    html += "<p>No messages yet. Share the first thought with the world!</p>";
    html += "</div>";
  } else {
    html += "<div style='padding:25px;'>";
    html += "<div class='text-list'>";
    
    // Display messages in reverse order (newest first)
    for (int i = textCount - 1; i >= 0; i--) {
      html += "<div class='text-item'>";
      html += "<h3>Message #" + String(i + 1);
      if (i == textCount - 1) html += " <span style='color:var(--accent);'>• Latest</span>";
      html += "</h3>";
      html += "<p>\"" + uploadedTexts[i] + "\"</p>";
      html += "</div>";
    }
    html += "</div></div>";
  }
  
  html += "<div class='action-buttons'>";
  html += "<a href='/' class='button primary'>✨ Add Your Message</a>";
  html += "<a href='#' onclick='location.reload();' class='button secondary'>🔄 Refresh</a>";
  html += "</div>";
  
  html += "<footer>";
  html += "<p><small>Art Beacon Portal • Connecting minds through words</small></p>";
  html += "</footer>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = ":root{--primary:#667eea;--secondary:#764ba2;--accent:#ff6b6b;--success:#51cf66;--text:#2c3e50;--bg:#f8f9fa;}";
  css += "*{box-sizing:border-box;margin:0;padding:0;}";
  css += "body{font-family:'SF Pro Display',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;";
  css += "background:linear-gradient(135deg,var(--primary) 0%,var(--secondary) 50%,var(--accent) 100%);";
  css += "min-height:100vh;padding:10px;color:var(--text);animation:gradientShift 10s ease infinite;}";
  css += "@keyframes gradientShift{0%,100%{background-position:0% 50%;}50%{background-position:100% 50%;}}";
  css += ".container{max-width:650px;margin:0 auto;background:rgba(255,255,255,0.95);";
  css += "backdrop-filter:blur(10px);border-radius:20px;overflow:hidden;";
  css += "box-shadow:0 20px 40px rgba(0,0,0,0.1);border:1px solid rgba(255,255,255,0.2);}";
  css += "header{background:linear-gradient(135deg,var(--primary),var(--secondary));";
  css += "color:white;padding:30px;text-align:center;}";
  css += "h1{font-size:2.5em;margin-bottom:10px;text-shadow:0 2px 4px rgba(0,0,0,0.3);}";
  css += ".subtitle{font-size:1.1em;opacity:0.9;font-weight:300;}";
  css += ".portal-info{padding:25px;background:var(--bg);}";
  css += ".stat-box{background:white;padding:20px;border-radius:15px;";
  css += "box-shadow:0 5px 15px rgba(0,0,0,0.08);border-left:4px solid var(--accent);}";
  css += ".stat-box h3{color:var(--primary);margin-bottom:15px;font-size:1.2em;}";
  css += ".stat-box p{margin:8px 0;font-size:0.95em;}";
  css += ".stat-box span{color:var(--accent);font-weight:bold;}";
  css += ".message-form{padding:25px;background:white;}";
  css += ".message-form h2{color:var(--primary);margin-bottom:20px;text-align:center;}";
  css += "textarea{width:100%;padding:15px;border:2px solid #e1e8ed;border-radius:12px;";
  css += "font-size:16px;font-family:inherit;resize:vertical;min-height:120px;";
  css += "transition:border-color 0.3s,box-shadow 0.3s;}";
  css += "textarea:focus{border-color:var(--primary);outline:none;";
  css += "box-shadow:0 0 0 3px rgba(102,126,234,0.1);}";
  css += ".form-footer{display:flex;justify-content:space-between;align-items:center;margin-top:15px;}";
  css += ".button{display:inline-block;padding:12px 24px;background:var(--primary);";
  css += "color:white;text-decoration:none;border:none;border-radius:8px;";
  css += "cursor:pointer;font-size:16px;font-weight:500;transition:all 0.3s;";
  css += "box-shadow:0 4px 12px rgba(102,126,234,0.3);}";
  css += ".button:hover{transform:translateY(-2px);box-shadow:0 8px 20px rgba(102,126,234,0.4);}";
  css += ".button.primary{background:var(--success);}";
  css += ".button.secondary{background:var(--accent);}";
  css += ".action-buttons{padding:25px;display:flex;gap:15px;justify-content:center;}";
  css += ".text-list{margin:20px 0;}";
  css += ".text-item{background:white;padding:20px;margin:15px 0;border-radius:12px;";
  css += "border-left:4px solid var(--primary);box-shadow:0 3px 10px rgba(0,0,0,0.05);}";
  css += ".text-item h3{margin-bottom:10px;color:var(--primary);font-size:1.1em;}";
  css += ".text-item p{line-height:1.6;color:var(--text);}";
  css += "footer{padding:20px;text-align:center;background:var(--bg);color:#666;}";
  css += "small{font-size:0.85em;color:#666;}";
  css += "@media (max-width:480px){.container{margin:5px;border-radius:15px;}";
  css += "header{padding:20px;}h1{font-size:2em;}.action-buttons{flex-direction:column;}}";
  
  server.send(200, "text/css", css);
}

void handleNotFound() {
  server.send(404, "text/html", 
    "<html><head><title>Page Not Found</title><link rel='stylesheet' href='/style.css'></head>"
    "<body><div class='container'><h2>404 - Page Not Found</h2>"
    "<p>The requested page was not found.</p>"
    "<a href='/' class='button'>Go Home</a></div></body></html>");
}

void handleCaptivePortal() {
  String redirectHTML = "<html><head>";
  redirectHTML += "<meta http-equiv='refresh' content='0; url=http://192.168.4.1/' />";
  redirectHTML += "<title>Art Beacon Portal</title></head>";
  redirectHTML += "<body style='font-family: Arial, sans-serif; text-align: center; margin-top: 50px;'>";
  redirectHTML += "<h2>🎨 Welcome to Art Beacon Portal!</h2>";
  redirectHTML += "<p>Redirecting you to the message portal...</p>";
  redirectHTML += "<p><a href='http://192.168.4.1/'>Click here if not redirected automatically</a></p>";
  redirectHTML += "</body></html>";
  
  server.send(200, "text/html", redirectHTML);
}

void handleAPI() {
  // API endpoint to get texts as JSON
  String json = "{\"texts\":[";
  for (int i = 0; i < textCount; i++) {
    if (i > 0) json += ",";
    json += "{\"id\":" + String(i + 1) + ",";
    json += "\"text\":\"" + uploadedTexts[i] + "\",";
    json += "\"timestamp\":\"" + String(millis()) + "\"}";
  }
  json += "],\"total\":" + String(textCount) + ",";
  json += "\"connected_devices\":" + String(WiFi.softAPgetStationNum()) + "}";
  
  server.send(200, "application/json", json);
}

String getWebPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>🎨 Art Beacon Portal</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta charset='UTF-8'>";
  html += "<link rel='stylesheet' href='/style.css'>";
  html += "<script>";
  html += "function refreshStats(){fetch('/api/texts').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('connected').textContent=d.connected_devices;";
  html += "document.getElementById('total').textContent=d.total;";
  html += "});}";
  html += "setInterval(refreshStats,5000);";
  html += "window.onload=refreshStats;";
  html += "</script>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  html += "<header>";
  html += "<h1>🎨 Art Beacon Portal</h1>";
  html += "<p class='subtitle'>A digital message board for wandering souls</p>";
  html += "</header>";
  
  html += "<div class='portal-info'>";
  html += "<div class='stat-box'>";
  html += "<h3>📡 Live Stats</h3>";
  html += "<p>Connected: <span id='connected'>" + String(WiFi.softAPgetStationNum()) + "</span> travelers</p>";
  html += "<p>Messages: <span id='total'>" + String(textCount) + "</span>/100</p>";
  html += "<p>Portal: <strong>" + String(ssid) + "</strong></p>";
  html += "</div></div>";
  
  html += "<div class='message-form'>";
  html += "<h2>✨ Leave Your Mark</h2>";
  html += "<form method='POST' action='/upload' onsubmit='this.querySelector(\"input[type=submit]\").disabled=true;'>";
  html += "<textarea name='textInput' rows='4' maxlength='500' ";
  html += "placeholder='Share your thoughts, art, poetry, or just say hello...' required></textarea>";
  html += "<div class='form-footer'>";
  html += "<small>Max 500 characters</small>";
  html += "<input type='submit' value='📤 Share Message' class='button primary'>";
  html += "</div></form></div>";
  
  html += "<div class='action-buttons'>";
  html += "<a href='/texts' class='button'>📜 View All Messages</a>";
  html += "<a href='#' onclick='refreshStats(); return false;' class='button secondary'>🔄 Refresh</a>";
  html += "</div>";
  
  html += "<footer>";
  html += "<p><small>Art Beacon Portal • A space for creative expression</small></p>";
  html += "</footer>";
  
  html += "</div></body></html>";
  
  return html;
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}