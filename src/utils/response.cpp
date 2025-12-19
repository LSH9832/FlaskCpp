#ifndef FLASKCPP_UTILS_RESPONSE_CPP
#define FLASKCPP_UTILS_RESPONSE_CPP

#include "FlaskCpp/utils/response.h"
#include "FlaskCpp/FlaskTypes.h"
#include <cstring>
#include <cstdint>

namespace flaskcpp
{

Response send_error(std::string text, int type, 
                    std::vector<std::pair<std::string, std::string>> extra_headers)
{
    Response resp;
    type = HttpStatusMap.find(type)!=HttpStatusMap.end()?type:500;
    resp.type = type;
    std::string content_type = getFileTypeString(FLASK_FILE_TEXT_HTML, "");
    std::ostringstream oss;
    
    oss << "HTTP/1.1 " << type << " " 
        << HttpStatusMap.at(type) << "\r\n";
    
    oss << "Content-Type: " << content_type;
    oss << "; charset=utf-8\r\n";
    oss << "Content-Length: " << text.size() << "\r\n";
    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "Connection: close\r\n\r\n";

    oss << text;

    resp.text = oss.str();

    // std::cout << resp.text << std::endl;
    return resp;
}

Response send_error(JsonGenerator& json, int type,
                    std::vector<std::pair<std::string, std::string>> extra_headers)
{
    Response resp;
    type = HttpStatusMap.find(type)!=HttpStatusMap.end()?type:500;
    
    resp.type = type;
    std::string content_type = getFileTypeString(FLASK_FILE_APP_JSON, "");
    std::ostringstream oss;
    oss << "HTTP/1.1 " << type << " " 
        << HttpStatusMap.at(type) << "\r\n";

    oss << "Content-Type: " << content_type;
    oss << "; charset=utf-8\r\n";
    oss << "Content-Length: " << json.toString().size() << "\r\n";
    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "Connection: close\r\n\r\n";

    oss << json.toString();

    resp.text = oss.str();

    // std::cout << resp.text << std::endl;
    return resp;
}


Response send_text(std::string text, 
                   std::vector<std::pair<std::string, std::string>> extra_headers)
{
    Response resp;
    resp.type = RESP_TYPE_TEXT;
    std::string content_type = getFileTypeString(FLASK_FILE_TEXT_PLAIN, "");

    if (text.find("<!DOCTYPE html>") < 2 || (text.find("<head") != text.npos && 
                                              text.find("</head>")!= text.npos && 
                                              text.find("<body") != text.npos && 
                                              text.find("</body>") != text.npos &&
                                              text.find("<html") != text.npos && 
                                              text.find("</html>") != text.npos))
    {
        content_type = getFileTypeString(FLASK_FILE_TEXT_HTML, "");
    }
    else if (text[0] == '{' && text[text.size()-1] == '}')
    {
        content_type = getFileTypeString(FLASK_FILE_APP_JSON, "");
    }
    
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK" << "\r\n";
    oss << "Content-Type: " << content_type;
    if (content_type.find("text/") != std::string::npos || content_type.find("application/json") != std::string::npos) {
        oss << "; charset=utf-8";
    }
    oss << "\r\n";

    oss << "Content-Length: " << text.size() << "\r\n";

    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        oss << header.first << ": " << header.second << "\r\n";
    }

    oss << "Connection: close\r\n\r\n";

    oss << text;

    resp.text = oss.str();

    return resp;
}

Response send_text(JsonGenerator& json, 
                   std::vector<std::pair<std::string, std::string>> extra_headers)
{
    Response resp;
    resp.type = RESP_TYPE_JSON;
    std::string content_type = getFileTypeString(FLASK_FILE_APP_JSON, "");
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK" << "\r\n";
    oss << "Content-Type: " << content_type;
    if (content_type.find("text/") != std::string::npos || content_type.find("application/json") != std::string::npos) {
        oss << "; charset=utf-8";
    }
    oss << "\r\n";

    oss << "Content-Length: " << json.toString().size() << "\r\n";

    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        oss << header.first << ": " << header.second << "\r\n";
    }

    oss << "Connection: close\r\n\r\n";

    oss << json.toString();

    resp.text = oss.str();

    return resp;
}


Response send_file(std::string file_path, std::string file_name, bool as_attachment, 
                   std::vector<std::pair<std::string, std::string>> extra_headers)
{
    Response resp;
    resp.type = RESP_TYPE_FILE;

    resp.file_name = file_name;
    resp.file_path = file_path;
    resp.as_attachment = as_attachment;
    resp.extra_headers = extra_headers;

    return resp;
}

template <typename T>
Response send_file_data(std::vector<T>& data, std::string file_name, bool as_attachment, 
                        std::vector<std::pair<std::string, std::string>> extra_headers)
{
    Response resp;
    if (sizeof(T) != sizeof(char))
    {
        return resp;
    }

    resp.type = RESP_TYPE_FILE_BYTES;
    resp.file_name = file_name;
    resp.as_attachment = as_attachment;
    resp.extra_headers = extra_headers;

    resp.file_data.resize(data.size());
    memcpy(resp.file_data.data(), data.data(), data.size());

    return resp;
}

template Response send_file_data<char>(
    std::vector<char>&, std::string, bool, 
    std::vector<std::pair<std::string, std::string>>);

template Response send_file_data<uint8_t>(
    std::vector<uint8_t>&, std::string, bool, 
    std::vector<std::pair<std::string, std::string>>);

template Response send_file_data<int8_t>(
    std::vector<int8_t>&, std::string, bool, 
    std::vector<std::pair<std::string, std::string>>);


}


#endif