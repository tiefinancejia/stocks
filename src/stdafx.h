#pragma once

#include <iostream>
#include <string.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>
using namespace std;

#include "secitpdk/secitpdk.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "scopeguard.hpp"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"

//×Ö·û´®·Ö¸î
void split_string_by_char(const char* str, char c, vector<string> &arVec);