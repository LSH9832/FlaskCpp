// headers/FlaskCpp.h
#ifndef FLASKCPP_H
#define FLASKCPP_H

#include "TemplateEngine.h"
#include "ThreadPool.h" // Добавляем пул потоков
#include "FlaskTypes.h"
#include "./utils/file.h"
#include "./utils/response.h"
#include "./utils/log.h"
#include "./utils/urlSafeSerializer.h"

#define FLASKDEBUG(app, msg) app->log({0, (msg), __LINE__, __FILE__, __func__})
#define FLASKINFO(app, msg) app->log({1, (msg), __LINE__, __FILE__, __func__})
#define FLASKSUCCESS(app, msg) app->log({2, (msg), __LINE__, __FILE__, __func__})
#define FLASKWARN(app, msg) app->log({3, (msg), __LINE__, __FILE__, __func__})
#define FLASKERROR(app, msg) app->log({4, (msg), __LINE__, __FILE__, __func__})
#define FLASKCRITICAL(app, msg) app->log({5, (msg), __LINE__, __FILE__, __func__})


#define FLASK_EMPTY_EXTRA_HEADER {"", ""}
#define FLASK_SET_COOKIES(cookies, time) {"Set-Cookie", parse_cookies( ( cookies ) , ( time ) )}
#define FLASK_NO_CACHE {"Cache-Control", "no-cache"}
#define FLASK_CACHE_TIME(t) {"Cache-Control", "public, max-age=" + std::to_string(t) + "\r\n"}
#define FLASK_DELETE_SESSION {"Set-Cookie", "session=deleted;Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly"}



// Типы хендлеров маршрутов

using UniteHandler = std::function<flaskcpp::Response(const RequestData&)>;

// deprecated
using SimpleHandler = std::function<std::string(const RequestData&)>;
using ComplexHandler = std::function<std::string(const RequestData&)>;
using FlaskFileHandler = std::function<flaskcpp::FileHandler(const RequestData&)>;

std::vector<char> readFileBytesData(const std::string& file_path, bool verbose=false);

void flaskSetDefaultVerbose(bool flag);

std::string parse_cookies(std::pair<std::string, std::string> pair, size_t seconds);

class FlaskCpp {
public:

    FlaskCpp(std::string server_name, size_t minThreads=2, size_t maxThreads=128);

    FlaskCpp(int port, bool verbose = false, bool enableHotReload = true, size_t minThreads = 2, size_t maxThreads = 8);

    void setTemplate(const std::string& name, const std::string& content);

    void setTemplateChangedCallback(TemplateEngine::FileChangedCallback callback);

    std::pair<std::string, std::string> generateSession(std::map<std::string, std::string> session, int max_age=604800);

    void setConfigUpdateListener(const std::string& file_path, TemplateEngine::FileChangedCallback callback);

    void setCheckTaskDuration(size_t ms);

    void addCheckTask(TemplateEngine::CheckTask task);

    void log(const flaskcpp::LogMsg& msg, bool simple=false);

    void route2(const std::string& path, UniteHandler handler);

    void route(const std::string& path, SimpleHandler handler) [[deprecated]];

    void setLog(loggerWrite w);

    void routeFile(const std::string& path, FlaskFileHandler handler) [[deprecated]];

    void routeParam(const std::string& pattern, ComplexHandler handler) [[deprecated]];

    void loadTemplatesFromDirectory(const std::string& directoryPath);

    void runAsync();

    void runAsync(int port, bool verbose=false, bool enableHotReload=true);

    void run();

    void run(int port, bool verbose=false, bool enableHotReload=true);

    void stop();

    std::string renderTemplate(const std::string& templateName, const TemplateEngine::Context& context);

    std::string renderTemplateString(const std::string& template_content, const TemplateEngine::Context& context);

    template <typename T>
    std::string send_file(std::vector<T> file_data, std::string file_name="", bool as_attachment=false,
                          const std::vector<std::pair<std::string, std::string>>& extra_headers = {});

    std::string send_file(std::string file_path, std::string file_name="", bool as_attachment=false,
                          const std::vector<std::pair<std::string, std::string>>& extra_headers = {});

    std::string buildResponse(const std::string& status_code,
                              const std::string& content_type,
                              const std::string& body,
                              const std::vector<std::pair<std::string, std::string>>& extra_headers = {});
    
    template <typename T>
    std::string buildResponse(const std::string& status_code,
                              int file_type, std::vector<T> file_data,
                              const std::vector<std::pair<std::string, std::string>>& extra_headers = {});
    

    std::string buildResponse(const std::string& status_code,
                              int file_type, std::string file_path,
                              const std::vector<std::pair<std::string, std::string>>& extra_headers = {});

    std::string setCookie(const std::string& name, const std::string& value,
                          const std::string& path = "/", const std::string& expires = "",
                          bool httpOnly = true, bool secure = false, const std::string& sameSite = "Lax");


    std::string deleteCookie(const std::string& name,
                             const std::string& path = "/");

    std::string generate404Error(const std::string& msg="", bool gen_header=true);
    std::string generate500Error(const std::string& msg, bool gen_header=true);

    std::vector<std::string> getAllTemplateNames();

    std::string jump_to(const std::string& route_, const std::string& message="", size_t delay=0);
    std::string jump2(const std::string& route_);

#ifdef ENABLE_PHP
    std::string executePHP(const RequestData& reqData, const std::filesystem::path& scriptPath);
#endif

    void setSecretKey(const std::string& key);

    bool isRunning();
    
private:
    size_t check_duration=500;
    URLSafeSerializer serialzer;
    std::string server_name;
    int port;
    bool verbose=false;
    bool enableHotReload;
    TemplateEngine templateEngine; 
    std::string templatesDirectory;
    std::map<std::string, std::filesystem::file_time_type> templatesTimestamps;
    std::atomic<bool> running; // flag

    loggerWrite logger=nullptr;

    std::atomic<bool> bind_success = true;

    TemplateEngine::FileChangedCallback template_changed_callback=nullptr;

    // thread pool
    ThreadPool threadPool;

    // hot reload
    std::thread hotReloadThread;

    void monitorTemplates();

    
    struct UnitedParamRoute {
        std::string pattern;
        UniteHandler handler;
    };
    std::unordered_map<std::string, UniteHandler> ROUTES;
    std::vector<UnitedParamRoute> paramROUTES;

    // deprecated
    struct ParamRoute {
        std::string pattern;
        ComplexHandler handler;
    };
    std::unordered_map<std::string, ComplexHandler> routes;
    std::unordered_map<std::string, FlaskFileHandler> file_routes;
    std::vector<ParamRoute> paramRoutes;


    std::mutex routeMutex;
    void handleClient2(int clientSocket, const std::string& clientIP);
    void handleClient(int clientSocket, const std::string& clientIP);
    std::string readRequest(int clientSocket);
    void parseRequest(const std::string& request, RequestData& reqData);
    void parseRequest2(ClientData& client_data, RequestData& reqData);
    void parseQueryString(const std::string& queryString, std::map<std::string, std::string>& queryParams);
    bool matchParamRoute(const std::string& path, const std::string& pattern, std::map<std::string,std::string>& routeParams);
    bool serveStaticFile(const RequestData& reqData, std::string& response);
    void sendResponse(int clientSocket, const std::string& content);
    

    void parseCookies(const std::string& cookieHeader, std::map<std::string, std::string>& cookies);
    std::string urlDecode(const std::string &value);
};

#endif // FLASKCPP_H
