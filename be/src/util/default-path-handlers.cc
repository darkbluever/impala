// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#include <sstream>
#include <fstream>
#include <sys/stat.h>

#include "common/logging.h"
#include "util/default-path-handlers.h"
#include "util/webserver.h"
#include "util/logging.h"

using namespace std;
using namespace google;
using namespace impala;

DEFINE_int64(web_log_bytes, 1024 * 1024, 
    "The maximum number of bytes to display on the debug webserver's log page");

// Writes the last FLAGS_web_log_bytes of the INFO logfile to a webpage
// Note to get best performance, set GLOG_logbuflevel=-1 to prevent log buffering
void LogsHandler(stringstream* output) {
  string logfile;
  impala::GetFullLogFilename(google::INFO, &logfile);
  (*output) << "<h2>INFO logs</h2>" << endl;
  (*output) << "Log path is: " << logfile << endl;

  struct stat file_stat;
  if (stat(logfile.c_str(), &file_stat) == 0) {
    long size = file_stat.st_size;
    long seekpos = size < FLAGS_web_log_bytes ? 0L : size - FLAGS_web_log_bytes;
    ifstream log(logfile.c_str(), ios::in);
    // Note if the file rolls between stat and seek, this could fail
    // (and we could wind up reading the whole file). But because the
    // file is likely to be small, this is unlikely to be an issue in
    // practice.
    log.seekg(seekpos);
    (*output) << "<br/>Showing last " << FLAGS_web_log_bytes << " bytes of log" << endl;
    (*output) << "<br/><pre>" << log.rdbuf() << "</pre>";

  } else {
    (*output) << "<br/>Couldn't open INFO log file: " << logfile;
  }

}

// Registered to handle "/flags", and prints out all command-line flags and their values
void FlagsHandler(stringstream* output) {
  (*output) << "<h2>Command-line Flags</h2>";
  (*output) << "<pre>" << CommandlineFlagsIntoString() << "</pre>";
}

void impala::AddDefaultPathHandlers(Webserver* webserver) {
  webserver->RegisterPathHandler("/logs", LogsHandler);
  webserver->RegisterPathHandler("/varz", FlagsHandler);
}
