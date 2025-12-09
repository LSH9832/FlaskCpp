#ifndef FLASKCPP_TYPES_H
#define FLASKCPP_TYPES_H

#include <iostream>
#include <unordered_map>
#include <functional>
#include <string>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <atomic> 
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <cstdlib>      // Для system
#include <cstdio>       // Для popen, pclose
#include <memory>
#include <array>
// #include <pair>
#include "./utils/parse/json.hpp"

// Структура для хранения данных запроса
struct RequestData {
    std::string method;
    std::string path, full_path;
    int content_type=0;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> formData; // Для POST-запросов
    std::map<std::string, std::string> routeParams; // Параметры из пути: /user/<id>
    std::map<std::string, std::string> headers;
    std::string body;
    struct File
    {
        std::string name;
        std::string file_name;
        std::string type;
        std::vector<char> data;
    };
    nlohmann::json json;
    std::map<std::string, File> files;
    std::map<std::string, std::string> cookies; // Хранит парсенные cookies
    std::map<std::string, std::string> session;
};

enum FlaskFileType
{
    FLASK_FILE_AUTO=-1,
    FLASK_FILE_DEFAULT,
    FLASK_FILE_JPEG,
    FLASK_FILE_PNG,
    FLASK_FILE_GIF,
    FLASK_FILE_WEBP,
    FLASK_FILE_PDF,
    FLASK_FILE_MSWORD,
    FLASK_FILE_ZIP,

    FLASK_FILE_BMP,
    FLASK_FILE_MP4,
    FLASK_FILE_MP3,
    FLASK_FILE_WAV,
    FLASK_FILE_VIDEO_OGG,
    FLASK_FILE_AUDIO_OGG,
    FLASK_FILE_VIDEO_WEBM,
    FLASK_FILE_AUDIO_WEBM,
    FLASK_FILE_AVI,
    FLASK_FILE_TEXT_PLAIN,
    FLASK_FILE_TEXT_HTML,
    FLASK_FILE_TEXT_CSS,
    FLASK_FILE_TEXT_JS,
    FLASK_FILE_TEXT_CSV,
    FLASK_FILE_TEXT_XML,
    FLASK_FILE_APP_JSON,
    FLASK_FILE_APP_XML,
    FLASK_FILE_APP_FORM
};


static const std::vector<std::string> __flaskFileTypeMap__ = {
    "application/octet-stream", "image/jpeg", "image/png", "image/gif", "image/webp",
    "application/pdf", "application/msword", "application/zip", "image/bmp", "video/mp4",
    "audio/mpeg", "audio/wav", "video/ogg", "audio/ogg", "video/webm", "audio/webm", "video/avi",
    "text/plain", "text/html", "text/css", "text/javascript", "text/csv", "text/xml", "application/json",
    "application/xml", "application/x-www-form-urlencoded"
};


std::pair<std::string, std::string> genFileExtraSettings(std::string fileName="", bool as_attachment=false);

std::string getFileTypeString(int type=-1, std::string filename="");

int getContentTypeByString(const std::string& content_type);


#endif