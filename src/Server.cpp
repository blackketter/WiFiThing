#include "WiFiThing.h"
#include "PString.h"

#ifdef ESP8266
ESP8266WebServer server(80);
#endif

#ifdef ESP32
WebServer server(80);
#endif

// default request handlers
void handleRoot() {
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  PString t;
  t.printf(
"<html>\
  <head>\
    <meta http-equiv='refresh' content='60'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from %s!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",
    WiFiThing::getHostname().c_str(),
    hr, min % 60, sec % 60
  );
  server.send ( 200, "text/html", t );
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void drawGraph() {
  PString out;
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x+= 10) {
    int y2 = rand() % 130;
    out.printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send ( 200, "image/svg+xml", out);
}

void debugLog() {
  PString log;
  console.printLog(&log);
  server.send(200, "text/plain", log);
}

String getCommandLine() {
  String p;
  String cmd;
  // build a command line from http://host/command.txt?cmd=COMMAND&p1=PARAM1&p2=PARAM2&p3=PARAM3
  if (server.args() >= 1) {
    cmd = server.arg("cmd");
    if (cmd) {
        p = server.arg("p1");
        if (p) {
          cmd += " " + p;
        }
        p = server.arg("p2");
        if (p) {
          cmd += " " + p;
        }
        p = server.arg("p3");
        if (p) {
          cmd += " " + p;
        }
    }
  }
  return cmd;
}

void runCommandHTML() {
  String commandLine = getCommandLine();
  PString output;

  // add header with command line field
  output +=
"<html>\
 <head>\
 <style>\
  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
 </style>\
 <title>Command</title></head>\
 <body>\
";

  output += "<pre>";

  // add results
  console.executeCommandLine(&output,commandLine.c_str());

  // add footer
  output +=
"</pre>\
<form action=\"/command.html\" method=\"GET\"><input type=\"text\" autofocus name=\"cmd\" size=\"100\"><input type=\"submit\" value=\"run\" name=\"Run\"></form>\
</body></html>\
";
  server.send(200, "text/html", output);
}

void runCommand() {
  String commandLine = getCommandLine();
  if (commandLine && commandLine.length()) {
    PString output;
    console.executeCommandLine(&output,commandLine.c_str());
    server.send(200, "text/plain", output);
  }
}

// server setup
void WiFiThing::beginServer() {
  console.debugln("Starting web server...");
  // examples: todo -replace
  server.on( "/", handleRoot );
  server.on( "/test.svg", drawGraph );
  server.on( "/command.txt", runCommand );
  server.on( "/command.html", runCommandHTML );
  server.on( "/inline", []() {
  server.send ( 200, "text/plain", "this works as well" );
  } );

  server.on(  "/log.txt", debugLog);

  server.onNotFound ( handleNotFound );
  server.begin();
}
