#ifndef FlaskCpp_RESPONSE_H
#define FlaskCpp_RESPONSE_H

#include <iostream>
#include <vector>
#include <unordered_map>

#include "FlaskCpp/utils/json.h"

namespace flaskcpp
{
enum ResponseType
{
    RESP_TYPE_UNDEFINED,
    RESP_TYPE_TEXT,
    RESP_TYPE_JSON,

    RESP_TYPE_CONTINUE = 100,
    RESP_TYPE_SWITCHING_PROTOCOLS = 101,

    // 2xx 成功状态码
    RESP_TYPE_OK = 200,
    RESP_TYPE_CREATED = 201,
    RESP_TYPE_ACCEPTED = 202,
    RESP_TYPE_NON_AUTHORITATIVE_INFORMATION = 203,
    RESP_TYPE_NO_CONTENT = 204,
    RESP_TYPE_RESET_CONTENT = 205,
    RESP_TYPE_PARTIAL_CONTENT = 206,

    // 3xx 重定向状态码
    RESP_TYPE_MULTIPLE_CHOICES = 300,
    RESP_TYPE_MOVED_PERMANENTLY = 301,
    RESP_TYPE_FOUND = 302,
    RESP_TYPE_SEE_OTHER = 303,
    RESP_TYPE_NOT_MODIFIED = 304,
    RESP_TYPE_USE_PROXY = 305,
    RESP_TYPE_TEMPORARY_REDIRECT = 307,

    // 4xx 客户端错误状态码
    RESP_TYPE_BAD_REQUEST = 400,
    RESP_TYPE_UNAUTHORIZED = 401,
    RESP_TYPE_FORBIDDEN = 403,
    RESP_TYPE_NOT_FOUND = 404,
    RESP_TYPE_METHOD_NOT_ALLOWED = 405,
    RESP_TYPE_NOT_ACCEPTABLE = 406,
    RESP_TYPE_PROXY_AUTHENTICATION_REQUIRED = 407,
    RESP_TYPE_REQUEST_TIMEOUT = 408,
    RESP_TYPE_CONFLICT = 409,
    RESP_TYPE_GONE = 410,
    RESP_TYPE_LENGTH_REQUIRED = 411,
    RESP_TYPE_PRECONDITION_FAILED = 412,
    RESP_TYPE_REQUEST_ENTITY_TOO_LARGE = 413,
    RESP_TYPE_REQUEST_URI_TOO_LONG = 414,
    RESP_TYPE_UNSUPPORTED_MEDIA_TYPE = 415,
    RESP_TYPE_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    RESP_TYPE_EXPECTATION_FAILED = 417,
    RESP_TYPE_LOCKED = 423,

    // 5xx 服务器错误状态码
    RESP_TYPE_INTERNAL_SERVER_ERROR = 500,
    RESP_TYPE_NOT_IMPLEMENTED = 501,
    RESP_TYPE_BAD_GATEWAY = 502,
    RESP_TYPE_SERVICE_UNAVAILABLE = 503,
    RESP_TYPE_GATEWAY_TIMEOUT = 504,
    RESP_TYPE_HTTP_VERSION_NOT_SUPPORTED = 505,


    RESP_TYPE_FILE = 9001,
    RESP_TYPE_FILE_BYTES
};

static const std::map<int, std::string> HttpStatusMap = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {423, "Locked"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"}
};

struct Response
{   
    int type = RESP_TYPE_UNDEFINED;
    std::string text;

    std::string file_path, file_name;
    std::vector<char> file_data;
    bool as_attachment=false;
    
    bool clear_session=false;
    std::map<std::string, std::string> session;
    std::vector<std::pair<std::string, std::string>> extra_headers;
};

Response send_error(JsonGenerator& json, int type,
                    std::vector<std::pair<std::string, std::string>> extra_headers={});

Response send_error(std::string text, int type,
                    std::vector<std::pair<std::string, std::string>> extra_headers={});

Response send_text(std::string text, 
                   std::vector<std::pair<std::string, std::string>> extra_headers={});

Response send_text(JsonGenerator& json, 
                   std::vector<std::pair<std::string, std::string>> extra_headers={});

Response send_file(std::string file_path, std::string file_name, bool as_attachment=false, 
                   std::vector<std::pair<std::string, std::string>> extra_headers={});

template <typename T>
Response send_file_data(std::vector<T> data, std::string file_name, bool as_attachment=false, 
                        std::vector<std::pair<std::string, std::string>> extra_headers={});

}

#endif