#include "FlaskCpp/FlaskCpp.h"
#include <cstdlib>
#include <csignal>
#include <thread>
#include <atomic>
#include <sys/select.h>
#include <fcntl.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <atomic>


const std::string LOGO =  R"( _____ _           _       ____             
|  ___| | __ _ ___| | __  / ___|_ __  _ __  
| |_  | |/ _` / __| |/ / | |   | '_ \| '_ \ 
|  _| | | (_| \__ \   <  | |___| |_) | |_) |
|_|   |_|\__,_|___/_|\_\  \____| .__/| .__/ 
                               |_|   |_|)";
const std::string FLASK_AUTHORS = R"(LSH9832 & Andrew-Gomonov)";

const std::string FLASK_COMPILE_TIME = __DATE__ + std::string(" ") + __TIME__;
std::atomic<bool> flask_first = true;
std::atomic<bool> verbose_default = true;

void flaskSetDefaultVerbose(bool flag)
{
    verbose_default = flag;
}

std::string parse_cookies(std::pair<std::string, std::string> pair, size_t seconds)
{
    std::ostringstream oss;
    oss << " " << pair.first << "=" << pair.second << ";";
    oss << " max-age=" << seconds << ";";
    oss << " HttpOnly;";
    // std::cout << oss.str() << std::endl;
    return oss.str();
}

static inline std::string url_decode(const std::string& encoded_str) {
    std::string decoded_str;
    for (size_t i = 0; i < encoded_str.size(); ++i) {
        if (encoded_str[i] == '%') {
            if (i + 2 < encoded_str.size()) {
                std::string hex = encoded_str.substr(i + 1, 2);
                char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                decoded_str += decoded_char;
                i += 2;
            }
        } else {
            decoded_str += encoded_str[i];
        }
    }
    return decoded_str;
}

static inline bool stringStartsWith(std::string str_, const std::string prefix)
{
    size_t str_len = str_.length();
    size_t prefix_len = prefix.length();
    if (prefix_len > str_len) return false;
    return str_.find(prefix) == 0;
}

static inline bool stringEndsWith(std::string str_, const std::string suffix)
{
    size_t str_len = str_.length();
    size_t suffix_len = suffix.length();
    if (suffix_len > str_len) return false;
    return (str_.find(suffix, str_len - suffix_len) == (str_len - suffix_len));
}

std::string strfnowtime(std::string format="%Y-%m-%d %H:%M:%S")
{
    std::ostringstream oss;
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&current_time);
    oss << std::put_time(local_time, format.c_str());
    return oss.str();
}


std::vector<char> readFileBytesData(const std::string& file_path, bool verbose) {
    std::vector<char> file_data;
    
    // 以二进制模式打开文件
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件 " << file_path << std::endl;
        return file_data;
    }
    
    // 获取文件大小
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 检查文件大小是否合理
    if (size <= 0) {
        std::cerr << "错误：文件大小无效或文件为空" << std::endl;
        return file_data;
    }
    
    // 预留空间以提高性能
    file_data.resize(size);
    
    // 读取整个文件到vector
    if (!file.read(file_data.data(), size)) {
        std::cerr << "错误：读取文件失败" << std::endl;
        file_data.clear();
        return file_data;
    }
    
    if(verbose) std::cout << "成功读取文件，大小: " << size << " 字节" << std::endl;
    return file_data;
}

// 构造函数
FlaskCpp::FlaskCpp(std::string server_name, size_t minThreads, size_t maxThreads)
:running(false), threadPool(minThreads, maxThreads), server_name(server_name)
{
    verbose = verbose_default;
    if (flask_first && verbose)
    {
        flask_first = false;
        std::cout << "\033[33m\033[1m" << LOGO << "\033[0m" << std::endl;
        std::cout << "FlaskCpp Compile Time: \033[35m\033[1m" 
                  << FLASK_COMPILE_TIME << "\033[0m" << std::endl;
        std::cout << "FlaskCpp Authors:\033[36m\033[1m " << FLASK_AUTHORS << "\033[0m" << std::endl;
    }
}

// 构造函数
FlaskCpp::FlaskCpp(int port, bool verbose, bool enableHotReload, size_t minThreads, size_t maxThreads) 
    : port(port), verbose(verbose), enableHotReload(enableHotReload), running(false), threadPool(minThreads, maxThreads) {
    if (verbose) {
        if (flask_first)
        {
            flask_first = false;
            std::cout << LOGO << std::endl;
            std::cout << "FlaskCpp Compile Time: \033[35m\033[1m" 
                    << FLASK_COMPILE_TIME << "\033[0m" << std::endl;
            std::cout << "FlaskCpp Authors:\033[36m\033[1m " << FLASK_AUTHORS << "\033[0m" << std::endl;
        }
    }
}

void FlaskCpp::setSecretKey(const std::string& key)
{
    serialzer.setKey(key);
}

void FlaskCpp::setLog(loggerWrite w)
{
    logger = w;
}

void FlaskCpp::setTemplate(const std::string& name, const std::string& content) {
    templateEngine.setTemplate(name, content);
}

void FlaskCpp::setConfigUpdateListener(const std::string& file_path, TemplateEngine::FileChangedCallback callback)
{
    templateEngine.addConfigUpdateListener(file_path, callback);
}


void FlaskCpp::log(const flaskcpp::LogMsg& msg)
{
    if (logger)
    {
        logger(msg);
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " 
                  << msg.content << std::endl;
    }
}

void FlaskCpp::setTemplateChangedCallback(TemplateEngine::FileChangedCallback callback)
{
    template_changed_callback = [callback](const std::string& path)
    {
        callback(path);
    };
}

void FlaskCpp::route2(const std::string& path, UniteHandler handler)
{
    if (path.find("<") != path.npos && path.find(">") != path.npos)
    {
        paramROUTES.push_back({path, handler});
        if (logger)
        {
            std::ostringstream oss;
            oss << "Param route added: " << path; 
            logger({2, oss.str(), __LINE__, __FILE__, __func__});
        }
        else if (verbose) {
            std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " 
                      << "\033[32m\033[1mParam route added\033[0m: " 
                      << path << std::endl;
        }
    }
    else
    {
        ROUTES[path] = [handler](const RequestData& req) {
            return handler(req);
        };
        if (logger)
        {
            std::ostringstream oss;
            oss << "Route added: " << path;
            logger({2, oss.str(), __LINE__, __FILE__, __func__});
        }
        else if (verbose) {
            std::cout << "[\033[32m" << strfnowtime() << "\033[0m] \033[32m\033[1m" << "Route added\033[0m: " << path << std::endl;
        }
    }
}


void FlaskCpp::route(const std::string& path, SimpleHandler handler) [[deprecated]] 
{
    routes[path] = [handler](const RequestData& req) {
        return handler(req);
    };
    if (logger)
    {
        std::ostringstream oss;
        oss << "Route added: " << path;
        logger({2, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] \033[32m\033[1m" << "Route added\033[0m: " << path << std::endl;
    }
}

void FlaskCpp::routeFile(const std::string& path, FlaskFileHandler handler) [[deprecated]] 
{
    file_routes[path] = [handler](const RequestData& req) {
        return handler(req);
    };
    if (logger)
    {
        std::ostringstream oss;
        oss << "File Route added: " << path;
        logger({2, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] \033[32m\033[1m" << "File Route added\033[0m: " << path << std::endl;
    }
}


std::vector<std::string> FlaskCpp::getAllTemplateNames()
{
    return templateEngine.getAllTemplateNames();
}


void FlaskCpp::routeParam(const std::string& pattern, ComplexHandler handler) [[deprecated]]
{
    paramRoutes.push_back({pattern, handler});
    if (logger)
    {
        std::ostringstream oss;
        oss << "Param route added: " << pattern;
        logger({2, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "\033[32m\033[1mParam route added\033[0m: " << pattern << std::endl;
    }
}

std::pair<std::string, std::string> FlaskCpp::generateSession(std::map<std::string, std::string> session, int max_age)
{
    std::string map_str = serialzer.map2str(session);

    // std::cout << map_str << std::endl;
    std::string dump_str = serialzer.dumps(map_str);
    // std::cout << dump_str << std::endl;
    std::string load_str = serialzer.loads(dump_str);
    // std::cout << load_str << std::endl;
    std::map<std::string, std::string> map;
    serialzer.str2map(load_str, map);
    // for (auto& it: map)
    // {
    //     std::cout << it.first << ": " << it.second << std::endl;
    // }


    return {"Set-Cookie", parse_cookies({"session", dump_str}, max_age)};
}


void FlaskCpp::loadTemplatesFromDirectory(const std::string& directoryPath) 
{
    namespace fs = std::filesystem;
    templatesDirectory = directoryPath;

    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        if (logger)
        {
            std::ostringstream oss;
            oss << "Templates directory does not exist: " << directoryPath;
            logger({3, oss.str(), __LINE__, __FILE__, __func__});
        }
        else 
            std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "\033[31m\033[1mTemplates directory does not exist\033[0m: " << directoryPath << std::endl;
        return;
    }

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".html") {
            std::ifstream file(entry.path());
            if (file) {
                std::ostringstream ss;
                ss << file.rdbuf();
                std::string content = ss.str();
                std::string filename = entry.path().filename().string();
                setTemplate(filename, content);

                // 保存文件的时间戳
                templatesTimestamps[entry.path().string()] = fs::last_write_time(entry);

                if (logger)
                {
                    std::ostringstream oss;
                    oss << "Loaded template: " << filename;
                    logger({2, oss.str(), __LINE__, __FILE__, __func__});
                }
                else if (verbose) {
                    std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "\033[32m\033[1mLoaded template\033[0m: " 
                              << filename << std::endl;
                }
            } else {
                if (logger)
                {
                    std::ostringstream oss;
                    oss << "Failed to open template file: " << entry.path();
                    logger({3, oss.str(), __LINE__, __FILE__, __func__});
                }
                else
                    std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "\033[31m\033[1mFailed to open template file\033[0m: " << entry.path() << std::endl;
            }
        }
    }
}

void FlaskCpp::setCheckTaskDuration(size_t ms)
{
    check_duration = ms?ms:1;
}

void FlaskCpp::addCheckTask(TemplateEngine::CheckTask task)
{
    templateEngine.addCheckTask(task);
}

void FlaskCpp::monitorTemplates() {
    namespace fs = std::filesystem;

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(check_duration)); // 适中的检查间隔

        templateEngine.checkConfigOnce();
        templateEngine.execAllCheckTask();

        if (templatesDirectory.empty()) continue;

        for (const auto& entry : fs::directory_iterator(templatesDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".html") {
                std::string filePath = entry.path().string();
                auto currentTimestamp = fs::last_write_time(entry);

                // 如果文件已更改则重新启动
                if (templatesTimestamps.find(filePath) != templatesTimestamps.end()) {
                    if (templatesTimestamps[filePath] != currentTimestamp) {
                        std::ifstream file(entry.path());
                        if (file) {
                            std::ostringstream ss;
                            ss << file.rdbuf();
                            std::string content = ss.str();
                            std::string filename = entry.path().filename().string();
                            setTemplate(filename, content);

                            templatesTimestamps[filePath] = currentTimestamp;

                            if (template_changed_callback)
                            {
                                template_changed_callback(filename);
                            }
                            if (logger)
                            {
                                std::ostringstream oss;
                                oss << "Template reloaded: " << filename;
                                logger({1, oss.str(), __LINE__, __FILE__, __func__});
                            }
                            else if (verbose) {
                                std::cout << "[\033[32m" << strfnowtime() << "\033[0m] \033[33mTemplate reloaded: " 
                                          << filename << "\033[0m" << std::endl;
                            }
                        }
                    }
                } else {
                    // 更新文件时间
                    templatesTimestamps[filePath] = currentTimestamp;
                }
            }
        }
    }
}

void FlaskCpp::runAsync() {
    if (running.load()) {
        if (logger)
        {
            std::ostringstream oss;
            oss << "Server is already running.";
            logger({1, oss.str(), __LINE__, __FILE__, __func__});
        }
        else
            std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "Server is already running." << std::endl;
        return;
    }

    running.store(true);

    // 仅在启用热重新加载时运行监视流
    if (enableHotReload) {
        hotReloadThread = std::thread(&FlaskCpp::monitorTemplates, this);
        if (logger)
        {
            std::ostringstream oss;
            oss << "Hot reload is enabled. Monitoring templates for changes.";
            logger({2, oss.str(), __LINE__, __FILE__, __func__});
        }
        else if (verbose) {
            std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " 
                      << "\033[33mHot reload is enabled. Monitoring templates for changes.\033[0m" << std::endl;
        }
    } else {
        if (verbose) {
            if (logger)
            {
                std::ostringstream oss;
                oss << "Hot reload is disabled.";
                logger({1, oss.str(), __LINE__, __FILE__, __func__});
            }
            else
                std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "\033[33mHot reload is disabled.\033[0m" << std::endl;
        }
    }

    // 将服务器启动任务添加到高优先级线程池
    // 假设0是最高优先级
    threadPool.enqueue(0, [this](){
        this->run();
    });

    
}

void FlaskCpp::run() {
    int serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        if (logger)
        {
            std::ostringstream oss;
            oss << "Failed to create socket.";
            logger({4, oss.str(), __LINE__, __FILE__, __func__});
        }
        else
            std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "Failed to create socket." << std::endl;
        running.store(false);
        return;
    }

    int opt = 0;
    setsockopt(serverSocket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
    opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in6 serverAddr = {};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_addr = in6addr_any;
    serverAddr.sin6_port = htons(port);

    bind_success = true;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        if (logger)
        {
            std::ostringstream oss;
            oss << "Bind failed.";
            logger({4, oss.str(), __LINE__, __FILE__, __func__});
        }
        else
            std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "Bind failed." << std::endl;
        close(serverSocket);
        bind_success = false;
        // running.store(false);
        // stop();
        return;
    }

    if (listen(serverSocket, 100) == -1) { // 增加Backlog以获得更大的负载
        if (logger)
        {
            std::ostringstream oss;
            oss << "Listen failed.";
            logger({4, oss.str(), __LINE__, __FILE__, __func__});
        }
        else
            std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "Listen failed." << std::endl;
        close(serverSocket);
        bind_success = false;
        // running.store(false);
        // stop();
        return;
    }

    if (logger)
    {
        std::ostringstream oss;
        oss << "ThreadPool initialized with range [" << threadPool.getMinThreads() << ", " << threadPool.getMaxThreads() << "]";
        logger({1, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose)
    {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "\033[32m\033[1mThreadPool initialized with range [" 
                  << threadPool.getMinThreads()
                  << ", " << threadPool.getMaxThreads() << "]\033[0m" << std::endl;
    }

    if (logger)
    {
        std::ostringstream oss;
        oss << "Server is running on http://0.0.0.0:" << port;
        logger({1, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "Server is running on \033[32mhttp://0.0.0.0:" << port << "\033[0m" << std::endl;
    }

    while (running.load()) {  // 支持服务器停止的循环
        sockaddr_in6 clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == -1) {
            if (running.load()) { // 检查服务器是否已停止
                if (logger)
                {
                    std::ostringstream oss;
                    oss << "Failed to accept connection.";
                    logger({4, oss.str(), __LINE__, __FILE__, __func__});
                }
                else 
                    std::cerr << "[\033[32m" << strfnowtime() << "\033[0m] " << "Failed to accept connection." << std::endl;
            }
            continue;
        }

        // 设置读取第一个数据块的超时（5秒）
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        // 读取查询方法的前4096字节
        char buffer[4096];
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), MSG_PEEK);
        std::string requestSample;
        if (bytesRead > 0) {
            requestSample = std::string(buffer, bytesRead);
        }

        // 从第一行提取查询方法
        std::string method = "GET"; // 默认GET
        size_t firstLineEnd = requestSample.find("\r\n");
        if (firstLineEnd != std::string::npos) {
            std::string firstLine = requestSample.substr(0, firstLineEnd);
            std::istringstream iss(firstLine);
            iss >> method;
        }

        // 根据查询方法设置优先级
        int priority = 5; // 默认平均优先级
        if (method == "GET") {
            priority = 1; // Get高优先级
        } else if (method == "POST") {
            priority = 2; // Post平均优先级
        } else if (method == "PUT" || method == "DELETE") {
            priority = 3; // 低优先级的put和delete
        } else {
            priority = 4; // 其他方法的优先级非常低
        }

        // if (verbose) {
        //     std::cout << "Request Method: " << method << " - Assigned Priority: " << priority << std::endl;
        // }

        char clientIP[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &clientAddr.sin6_addr, clientIP, INET6_ADDRSTRLEN);

        bool fromIPV4 = stringStartsWith(clientIP, "::ffff:");

        // 将客户端处理添加到具有特定优先级的线程池
        threadPool.enqueue(priority, [this, clientSocket, clientIP, fromIPV4]() {
            this->handleClient(clientSocket, std::string(fromIPV4?clientIP+7:clientIP));
        });
    }

    close(serverSocket);
}

void FlaskCpp::run(int port, bool verbose, bool enableHotReload)
{
    this->port = port;
    this->verbose= verbose;
    this->enableHotReload = enableHotReload;
    this->run();
}

void FlaskCpp::runAsync(int port, bool verbose, bool enableHotReload)
{
    this->port = port;
    this->verbose= verbose;
    this->enableHotReload = enableHotReload;
    this->runAsync();
}

bool FlaskCpp::isRunning()
{
    return running && bind_success;
}

void FlaskCpp::stop() {
    if (!running.load() && bind_success) return;

    if (logger)
    {
        std::ostringstream oss;
        oss << "please wait for http server stop.";
        logger({1, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "(\033[36m" << server_name << "\033[0m) " 
    << "please wait for http server stop" << std::endl;
    
    // std::cout << __LINE__ << std::endl;
    running.store(false);

    // std::cout << __LINE__ << std::endl;

    // 创建与服务器套接字的连接以终止Accept锁
    int dummySocket = socket(AF_INET, SOCK_STREAM, 0);
    if(dummySocket != -1){
        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddr.sin_port = htons(port);
        connect(dummySocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        close(dummySocket);
    }

    // std::cout << __LINE__ << std::endl;
    // 停止线程池
    threadPool.shutdown();
    // std::cout << __LINE__ << std::endl;
    
    // 等待监控线程完成
    if (enableHotReload && hotReloadThread.joinable()) {
        hotReloadThread.join();
    }
    // std::cout << __LINE__ << std::endl;
    

    if (logger)
    {
        std::ostringstream oss;
        oss << "Server has been stopped.";
        logger({2, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " << "Server has been stopped." << std::endl;
    }
}

std::string FlaskCpp::renderTemplate(const std::string& templateName, const TemplateEngine::Context& context) {
    return templateEngine.render(templateName, context);
}

std::string FlaskCpp::jump_to(const std::string& route_, const std::string& msg, size_t delay)
{
    std::string html_string = R"(<!DOCTYPE html>
<html>
    <head>
        <title>redirect</title>
        <meta http-equiv="refresh" content=")" + std::to_string(delay) + ";url=" + route_ + R"(">
    </head>
    <body>
        <p>)" + msg + R"(</p>
        wait for )" + std::to_string(delay) + R"(seconds.
    </body>
</html>)";
    return html_string;
}

std::string FlaskCpp::buildResponse(const std::string& status_code,
                                    const std::string& content_type,
                                    const std::string& body,
                                    const std::vector<std::pair<std::string, std::string>>& extra_headers) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << "\r\n";
    response << "Content-Type: " << content_type;

    // 为文本内容类型添加charset=utf-8
    if (content_type.find("text/") != std::string::npos || content_type.find("application/json") != std::string::npos) {
        response << "; charset=utf-8";
    }
    response << "\r\n";

    response << "Content-Length: " << body.size() << "\r\n";

    // 添加其他标题，包括多个设置cookie
    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        response << header.first << ": " << header.second << "\r\n";
    }

    response << "Connection: close\r\n\r\n";
    response << body;
    return response.str();
}

template <typename T>
std::string FlaskCpp::buildResponse(const std::string& status_code,
                                    int file_type, std::vector<T> file_data,
                                    const std::vector<std::pair<std::string, std::string>>& extra_headers)
{
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << "\r\n";
    if (file_type >=0 && file_type < __flaskFileTypeMap__.size())
    {
        response << "Content-Type: " << __flaskFileTypeMap__[file_type] << "\r\n";
    }
    else
    {
        response << "Content-Type: application/octet-stream\r\n";
    }
        
    response << "Content-Length: " << file_data.size() << "\r\n";
    
    
    bool has_cache_control = false;
    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        if (header.first == "Cache-Control") has_cache_control = true;
        response << header.first << ": " << header.second << "\r\n";
    }
    if (!has_cache_control)
    {
        response << "Cache-Control: public, max-age=3600\r\n";
    }
    
    response << "Connection: close\r\n\r\n";
    
    // 将二进制数据写入响应体
    response.write((char*)file_data.data(), file_data.size() * sizeof(T));
    
    return response.str();
}

template std::string FlaskCpp::buildResponse<char>(
    const std::string&, int, std::vector<char>,
    const std::vector<std::pair<std::string, std::string>>&);

template std::string FlaskCpp::buildResponse<int8_t>(
    const std::string&, int, std::vector<int8_t>,
    const std::vector<std::pair<std::string, std::string>>&);

template std::string FlaskCpp::buildResponse<uint8_t>(
    const std::string&, int, std::vector<uint8_t>,
    const std::vector<std::pair<std::string, std::string>>&);


template <typename T>
std::string FlaskCpp::send_file(std::vector<T> file_data, std::string file_name, bool as_attachment,
                                const std::vector<std::pair<std::string, std::string>>& extra_headers)
{
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    if (file_data.size())
    {
        response << "Content-Type: " << getFileTypeString(-2, file_name) << "\r\n";
    }
    else
    {
        return generate404Error();
    }
        
    response << "Content-Length: " << file_data.size() << "\r\n";
    

    auto fileExtraHeader = genFileExtraSettings(file_name, as_attachment);
    response << fileExtraHeader.first << ": " << fileExtraHeader.second;
    
    bool has_cache_control = false;
    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        if (header.first == "Cache-Control") has_cache_control = true;
        response << header.first << ": " << header.second << "\r\n";
    }
    if(!has_cache_control)
    {
        response << "Cache-Control: public, max-age=3600\r\n";
    }
    
    response << "Connection: close\r\n\r\n";
    
    // 将二进制数据写入响应体
    response.write((char*)file_data.data(), file_data.size() * sizeof(T));
    
    return response.str();
}


template std::string FlaskCpp::send_file<char>(std::vector<char>, std::string, bool,
                                const std::vector<std::pair<std::string, std::string>>&);

template std::string FlaskCpp::send_file<int8_t>(std::vector<int8_t>, std::string, bool,
                                const std::vector<std::pair<std::string, std::string>>&);

template std::string FlaskCpp::send_file<uint8_t>(std::vector<uint8_t>, std::string, bool,
                                const std::vector<std::pair<std::string, std::string>>&);

std::string FlaskCpp::send_file(std::string file_path, std::string file_name, bool as_attachment,
                                const std::vector<std::pair<std::string, std::string>>& extra_headers)
{
    auto data = readFileBytesData(file_path);
    if (data.size())
    {
        if (!file_name.size())
        {
            int k = -1;
            for (int i=0;i<file_path.size();i++)
            {
                if (file_path[i] == '/')
                {
                    k = i;
                }
            }
            file_name = file_path.substr(k+1);
        }
        return this->send_file(data, file_name, as_attachment, extra_headers);
    }
    else
    {
        return generate404Error();
    }
}

std::string FlaskCpp::buildResponse(const std::string& status_code,
                                    int file_type, std::string file_path,
                                    const std::vector<std::pair<std::string, std::string>>& extra_headers)
{
    auto data = readFileBytesData(file_path);
    if (data.size())
    {
        return this->buildResponse(status_code, file_type, data, extra_headers);
    }
    else
    {
        return generate404Error();
    }
}

std::string FlaskCpp::setCookie(const std::string& name, const std::string& value,
                                const std::string& path, const std::string& expires,
                                bool httpOnly, bool secure, const std::string& sameSite) {
    std::ostringstream cookie;
    cookie << name << "=" << value;
    cookie << "; Path=" << path;
    if (!expires.empty()) {
        cookie << "; Expires=" << expires;
    }
    if (httpOnly) {
        cookie << "; HttpOnly";
    }
    if (secure) {
        cookie << "; Secure";
    }
    if (!sameSite.empty()) {
        cookie << "; SameSite=" << sameSite;
    }
    return cookie.str();
}

std::string FlaskCpp::deleteCookie(const std::string& name,
                                   const std::string& path) {
    std::ostringstream cookie;
    cookie << name << "=deleted";
    cookie << "; Path=" << path;
    cookie << "; Expires=Thu, 01 Jan 1970 00:00:00 GMT";
    cookie << "; HttpOnly";
    return cookie.str();
}

void FlaskCpp::handleClient(int clientSocket, const std::string& clientIP) {
    int status = 200;
    std::string method="Unknown", path="Unknown";
    try {
        // std::cout << 1 << std::endl;
        std::string requestStr = readRequest(clientSocket);
        // std::cout << 2 << std::endl;
        RequestData reqData;
        parseRequest(requestStr, reqData);
        reqData.path = url_decode(reqData.path);
        // std::cout << 3 << ". " << reqData.method << "," << reqData.path << std::endl;
        
        if (reqData.path[0] == '/')
        {
            method = reqData.method;
            path = reqData.full_path;
        }
        else
        {
            return;
        }
        

        std::string response;
        bool is_file=false;
        flaskcpp::FileHandler fh;
        flaskcpp::Response resp;

        {
            std::lock_guard<std::mutex> lock(routeMutex);

            UniteHandler unite_handler = nullptr;
            ComplexHandler handler = nullptr;
            
            // 试图找到准确的路线。
            auto uit = ROUTES.find(reqData.path);
            if (uit != ROUTES.end())
            {
                unite_handler = uit->second;
            }
            else
            {
                for (auto &pr : paramROUTES) {
                    if (matchParamRoute(reqData.path, pr.pattern, reqData.routeParams)) {
                        unite_handler = pr.handler;
                        break;
                    }
                }
            }
            
            
            if (unite_handler)
            {
                resp = unite_handler(reqData);
                if (!resp.type)
                {
                    sendResponse(clientSocket, generate404Error("404 NOT FOUND"));
                    status = 404;
                }
            }
            // 如果新接口没有匹配，尝试匹配老接口，不建议使用
            else
            {
                auto it = routes.find(reqData.path);
                if (it != routes.end()) {
                    handler = it->second;
                } else {
                    // 检查带有参数的路由
                    for (auto &pr : paramRoutes) {
                        if (matchParamRoute(reqData.path, pr.pattern, reqData.routeParams)) {
                            handler = pr.handler;
                            break;
                        }
                    }
                }

                if (!handler) {
                    // 寻找是否是本地文件
                    FlaskFileHandler fhandler = nullptr;
                    auto it = file_routes.find(reqData.path);
                    if (it != file_routes.end()) {
                        fhandler = it->second;
                        is_file = true;
                        fhandler(reqData).copyTo(fh);
                    }
                    // 检查静态文件
                    else if (!serveStaticFile(reqData, response)) {
                        response = generate404Error();
                        status = 404;
                    }
                } else {
                    response = handler(reqData);
                }
            }
        }
        
        if (resp.type)
        {
            if (resp.type == flaskcpp::RESP_TYPE_NOT_FOUND)
            {
                status = 404;
            }
            if (resp.type == flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR)
            {
                status = 500;
            }
            switch (resp.type)
            {
            case flaskcpp::RESP_TYPE_TEXT:
            case flaskcpp::RESP_TYPE_JSON:
                sendResponse(clientSocket, resp.text);
                break;
            case flaskcpp::RESP_TYPE_FILE:
                {
                    fh.init(resp.file_path, resp.file_name, resp.as_attachment);
                }
                break;
            case flaskcpp::RESP_TYPE_FILE_BYTES:
                {
                    
                    fh.init("", resp.file_name, resp.as_attachment);
                    
                    fh.setFileData(resp.file_data);
                    
                }
                break;
            default:
                if (resp.type >= 100 && resp.type < 600)
                {
                    sendResponse(clientSocket, resp.text);
                }
                else
                {
                    sendResponse(clientSocket, generate404Error("404 NOT FOUND"));
                }
                
                break;
            }
            if (resp.type >= flaskcpp::RESP_TYPE_FILE)
            {
                
                auto splitStr = [](const std::string&s, const char sig){
                    std::vector<std::string> parts;
                    std::istringstream iss(s);
                    std::string p;
                    while(std::getline(iss, p, sig)) {
                        if (!p.empty()) parts.push_back(p);
                    }
                    return parts;
                };
                
                auto it = reqData.headers.find("Range");
                
                if (it != reqData.headers.end())
                {
                    auto range = splitStr(it->second, '-');
                    if (range.size() >= 1)
                    {
                        fh.setStart(std::atoll(splitStr(range[0], '=')[1].c_str()));
                        if (range.size() == 2)
                        {
                            if (range[1].size())
                            {
                                fh.setEnd(std::atoll(range[1].c_str()));
                            }
                        }
                        status = 206;
                    }
                }
                
                std::string header_str = fh.generateHeader(resp.extra_headers);

                // std::cout << header_str << std::endl;
                
                if (!header_str.size() || !fh.isFileExist())
                {
                    sendResponse(clientSocket, generate500Error("file not found"));
                }
                else
                {
                    
                    sendResponse(clientSocket, header_str);
                    std::vector<char> file_data(8192);
                    size_t read_size=0;
                    
                    // std::cout << header_str << std::endl;
                    while (read_size = fh.read(file_data))
                    {
                        if (send(clientSocket, file_data.data(), read_size, MSG_NOSIGNAL) <= 0)
                        {
                            break;
                        }
                    }
                }

            }
        }
        else
        {
            if (is_file)
            {
                auto splitStr = [](const std::string&s, const char sig){
                    std::vector<std::string> parts;
                    std::istringstream iss(s);
                    std::string p;
                    while(std::getline(iss, p, sig)) {
                        if (!p.empty()) parts.push_back(p);
                    }
                    return parts;
                };
                auto it = reqData.headers.find("Range");
                if (it != reqData.headers.end())
                {
                    // std::cout << it->second << std::endl;
                    auto range = splitStr(it->second, '-');
                    // std::cout << range.size() << std::endl;
                    if (range.size() >= 1)
                    {
                        // std::cout << splitStr(range[0], '=')[1].c_str() << std::endl;
                        // std::cout << "start: " << std::atoll(splitStr(range[0], '=')[1].c_str()) << " bytes" << std::endl;
                        fh.setStart(std::atoll(splitStr(range[0], '=')[1].c_str()));
                        // std::cout << "set start done" << std::endl;
                        if (range.size() == 2)
                        {
                            if (range[1].size())
                            {
                                fh.setEnd(std::atoll(range[1].c_str()));
                            }
                        }
                    }
                }
                // std::cout << "gen header" << std::endl;
                std::string header_str = fh.generateHeader();
                if (!header_str.size() || !fh.isFileExist())
                {
                    sendResponse(clientSocket, generate404Error("file not found"));
                    status = 404;
                }
                else
                {
                    sendResponse(clientSocket, header_str);
                    std::vector<char> file_data(8192);
                    size_t read_size=0;
                    while (read_size = fh.read(file_data))
                    {
                        if (send(clientSocket, file_data.data(), read_size, MSG_NOSIGNAL) <= 0)
                        {
                            break;
                        }
                    }
                }
                // sendResponse(clientSocket, )
            }
            else
            {
                sendResponse(clientSocket, response);
            }
        }
        
        
        close(clientSocket);
    } catch (std::exception& e) {
        std::string response = generate500Error(e.what());
        sendResponse(clientSocket, response);
        status = 500;
        close(clientSocket);
    } catch (...) {
        std::string response = generate500Error("Unknown error");
        sendResponse(clientSocket, response);
        status = 500;
        close(clientSocket);
    }
    if (logger)
    {
        std::ostringstream oss;
        oss << clientIP << " | " << method << " " << path << " -> " << status;
        logger({1, oss.str(), __LINE__, __FILE__, __func__});
    }
    else if (verbose) {
        std::cout << "[\033[32m" << strfnowtime() << "\033[0m] " 
                << "\033[36m" << clientIP << "\033[0m | "
                << "\033[34m\033[1m" << method << " \033[0m\033[35m" << path 
                << "\033[0m -> \033[36m" << status << "\033[0m" << std::endl;
    }
}

std::string FlaskCpp::readRequest(int clientSocket) {
    std::string request;
    char buffer[4096];
    ssize_t bytesRead;
    // 阅读标题
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), MSG_PEEK)) > 0) {
        std::string temp(buffer, bytesRead);
        size_t pos = temp.find("\r\n\r\n");
        if (pos != std::string::npos) {
            recv(clientSocket, buffer, pos+4, 0);
            request.append(buffer, pos+4);
            break;
        } else {
            recv(clientSocket, buffer, bytesRead, 0);
            request.append(buffer, bytesRead);
        }
    }

    // 检查读取主体的内容长度
    std::string headers = request;
    size_t bodyPos = headers.find("\r\n\r\n");
    int contentLength = 0;
    if (bodyPos != std::string::npos) {
        std::string headerPart = headers.substr(0, bodyPos);
        std::istringstream iss(headerPart);
        std::string line;
        while (std::getline(iss, line)) {
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            if (line.find("Content-Length:") != std::string::npos) {
                std::string lenStr = line.substr(line.find(":")+1);
                lenStr.erase(0,lenStr.find_first_not_of(' '));
                contentLength = std::stoi(lenStr);
            }
        }

        if (contentLength > 0) {
            std::string body;
            body.resize(contentLength);
            int totalRead = 0;
            while (totalRead < contentLength) {
                ssize_t r = recv(clientSocket, &body[totalRead], contentLength - totalRead, 0);
                if (r <= 0) break;
                totalRead += r;
            }
            request.append(body);
        }
    }

    return request;
}


std::vector<std::string> stringSplit(std::string str_, std::string delimiter) {
    if (str_.empty()) return {""};
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string::size_type prev_pos = 0;
    while ((pos = str_.find(delimiter, prev_pos)) != std::string::npos) {
        tokens.push_back(str_.substr(prev_pos, pos - prev_pos));
        prev_pos = pos + delimiter.length();
    }
    if (prev_pos < str_.length()) tokens.push_back(str_.substr(prev_pos));
    if (stringEndsWith(str_, delimiter)) tokens.push_back("");
    return tokens;
}


static inline std::string parse_file(std::string body, RequestData& req)
{
    // std::cout << body << std::endl;
    bool recursive = false;
    std::string head;
    if (req.headers.find("Content-Type") != req.headers.end())
    {
        for(auto&line: stringSplit(req.headers.at("Content-Type"), "; "))
        {
            if (stringStartsWith(line, "boundary="))
            {
                head = line.substr(9);
            }
        }
    }
    if (head.empty())
    {
        head = "------";
        recursive = true;
    }

    size_t loc = body.find(head);
    if (loc == body.npos)
    {
        return body;
    }

    // std::cout << body.substr(loc, 500) << std::endl;

    size_t line_loc = body.substr(loc).find("\n");
    head = body.substr(loc, line_loc-1);

    std::string end = head + "--";

    size_t end_loc = body.substr(loc+line_loc+1).find(end);

    if (end_loc == body.npos) return body;

    std::string content = body.substr(loc+line_loc+1, end_loc-4);
    std::string rest = body.substr(0, loc) + body.substr(loc+line_loc+1+end_loc+head.size()+2);

    size_t disp_loc = content.find("Content-Disposition: ");
    if (disp_loc == content.npos) return body;

    size_t disp_loc_end = content.find("\r\n");
    if (disp_loc_end == content.npos) return body;

    RequestData::File f;
    for(auto& p: stringSplit(content.substr(disp_loc + 21, disp_loc_end - disp_loc - 21), "; "))
    {
        // std::cout << p << std::endl;
        if (stringStartsWith(p, "name=\""))
        {
            f.name = p.substr(6, p.size()-7);
        }
        else if (stringStartsWith(p, "filename=\""))
        {
            f.file_name = p.substr(10, p.size()-11);
            // std::cout << f.file_name << std::endl;
        }
    }

    if (strcmp(f.name.c_str(), "file") != 0)   // 如果不是文件，递归后续字符串
    {
        return recursive?(head + "\r\n" + content + "\r\n\r\n" + end + "\r\n" + parse_file(rest, req)):body;
    }
    
    size_t type_loc = content.find("Content-Type: ", disp_loc_end);
    if (type_loc != content.npos) 
    {
        size_t type_loc_end = content.find("\r\n", type_loc);
        f.type = content.substr(type_loc + 14, type_loc_end - type_loc - 14);
    }
    size_t f_data_start = content.find("\r\n\r\n", type_loc);

    std::string fdata = content.substr(f_data_start+4);

    f.data.resize(fdata.size());
    memcpy(f.data.data(), fdata.data(), f.data.size());
    // size_t type_loc_end = content.find
    req.files[f.file_name] = std::move(f);

    
    // std::cout << fdata << std::endl;
    // std::cout << "content:-----------\n" << content  << "\nrest--------------\n" << rest << "\nend:-------------" << std::endl;

    return recursive?parse_file(rest, req):rest;
}

void FlaskCpp::parseRequest(const std::string& request, RequestData& reqData) {
    // std::cout << request.substr(0, 2000) << std::endl;
    std::istringstream stream(request);
    std::string firstLine;
    std::getline(stream, firstLine);
    firstLine.erase(std::remove(firstLine.begin(), firstLine.end(), '\r'), firstLine.end());
    {
        std::istringstream fls(firstLine);
        fls >> reqData.method;
        std::string fullPath;
        fls >> fullPath;
        // HTTP版本不能详细处理
    }

    // Headers
    std::string line;
    while (std::getline(stream, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        if (line.empty()) break;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string val = line.substr(colonPos+1);
            while(!val.empty() && isspace((unsigned char)val.front())) val.erase(val.begin());
            reqData.headers[key] = val;
        }
    }

    

    // 余数 - 主体
    {
        std::string all = request;
        size_t bodyPos = all.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string body = all.substr(bodyPos+4);
            
            
            if (reqData.headers.find("Content-Type") != reqData.headers.end())
            {
                // json
                std::string content_type = reqData.headers.at("Content-Type");
                reqData.content_type = getContentTypeByString(content_type);
                if (reqData.content_type == FLASK_FILE_APP_JSON)
                {
                    reqData.json.clear();
                    try {
                        reqData.json = nlohmann::json::parse(body);
                    } catch (const nlohmann::json::parse_error& e) {
                        std::cerr << "failed to parse JSON: " << e.what() << std::endl;
                    }
                    // std::cout << body << std::endl;

                    // std::cout << reqData.json << std::endl;
                    // for (auto& [key, value]: reqData.json.items())
                    // {
                    //     std::cout << key << ": " << value.type_name() << std::endl;
                    // }
                }
            }

            // parse_file
            reqData.body = parse_file(body, reqData);
        }
    }

    // 解析查询字符串
    {
        std::istringstream fls(firstLine);
        std::string tmp, fullPath;
        fls >> tmp >> fullPath;
        size_t questionMarkPos = fullPath.find('?');
        reqData.full_path = fullPath;
        if (questionMarkPos != std::string::npos) {
            reqData.path = fullPath.substr(0, questionMarkPos);
            std::string queryString = fullPath.substr(questionMarkPos + 1);
            parseQueryString(queryString, reqData.queryParams);
        } else {
            reqData.path = fullPath;
        }
    }

    // 如果POST请求且Content-Type为application/x-www-form-urlencoded，则解析formData
    if (reqData.method == "POST") {
        auto ctypeIt = reqData.headers.find("Content-Type");
        if (ctypeIt != reqData.headers.end() && ctypeIt->second.find("application/x-www-form-urlencoded") != std::string::npos) {
            parseQueryString(reqData.body, reqData.formData);
        }
    }

    // 解析Cookies
    auto cookieIt = reqData.headers.find("Cookie");
    if (cookieIt != reqData.headers.end()) {
        parseCookies(cookieIt->second, reqData.cookies);

        // for (auto& it: reqData.cookies)
        // {
        //     std::cout << it.first << ": " << it.second << std::endl;
        // }

        if (reqData.cookies.find("session") != reqData.cookies.end())
        {
            // std::cout << "session: " << reqData.cookies["session"] << std::endl;
            serialzer.str2map(serialzer.loads(reqData.cookies["session"]), reqData.session);
            // for (auto& it: reqData.session)
            // {
            //     std::cout << it.first << ": " << it.second << std::endl;
            // }
        }
    }
}

void FlaskCpp::parseQueryString(const std::string& queryString, std::map<std::string, std::string>& queryParams) {
    std::istringstream stream(queryString);
    std::string pair;
    while (std::getline(stream, pair, '&')) {
        size_t equalSignPos = pair.find('=');
        std::string key, value;
        if (equalSignPos != std::string::npos) {
            key = pair.substr(0, equalSignPos);
            value = pair.substr(equalSignPos + 1);
        } else {
            key = pair;
        }
        queryParams[key] = urlDecode(value);
    }
}

bool FlaskCpp::matchParamRoute(const std::string& path, const std::string& pattern, std::map<std::string,std::string>& routeParams) {
    // 将path和pattern按'/'拆分
    auto splitPath = [](const std::string&s){
        std::vector<std::string> parts;
        std::istringstream iss(s);
        std::string p;
        while(std::getline(iss, p, '/')) {
            if (!p.empty()) parts.push_back(p);
        }
        return parts;
    };

    auto pathParts = splitPath(path);
    auto patternParts = splitPath(pattern);

    if (stringStartsWith(patternParts[patternParts.size()-1], "<path:") && 
        stringEndsWith(patternParts[patternParts.size()-1], ">"))
    {
        std::string paramName = patternParts[patternParts.size()-1].substr(6, patternParts[patternParts.size()-1].size()-7);
        std::ostringstream oss;
        for (int i=patternParts.size()-1;i<pathParts.size();++i)
        {
            if (i>patternParts.size()-1)
            {
                oss << "/";
            }
            oss << pathParts[i];
        }
        
        routeParams[paramName] = oss.str();

        size_t sz = patternParts.size()-1;
        patternParts.resize(sz);
        pathParts.resize(sz);
    }
    if (pathParts.size() != patternParts.size()) return false;

    for (size_t i=0; i<pathParts.size(); i++) {
        if (patternParts[i].size()>2 && patternParts[i].front()=='<' && patternParts[i].back()=='>') {
            std::string paramName = patternParts[i].substr(1, patternParts[i].size()-2);
            routeParams[paramName] = pathParts[i];
        } else {
            if (patternParts[i] != pathParts[i]) return false;
        }
    }

    return true;
}

bool FlaskCpp::serveStaticFile(const RequestData& reqData, std::string& response) {
    if (reqData.path.rfind("/static/", 0) == 0) {
        std::string filename = reqData.path.substr(8); // Убираем /static/
        std::filesystem::path filePath = std::filesystem::current_path() / "static" / filename;
        if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
            std::string ext = filePath.extension().string();
#ifdef ENABLE_PHP
            if(ext == ".php"){
                // 通过php-cgi支持PHP
                std::string phpOutput = executePHP(reqData, filePath);
                response = phpOutput;
                return true;
            }
#endif

            std::string ct = "text/plain";
            if (ext == ".html") ct = "text/html";
            else if (ext == ".css") ct = "text/css";
            else if (ext == ".js") ct = "application/javascript";
            else if (ext == ".json") ct = "application/json";
            else if (ext == ".png") ct = "image/png";
            else if (ext == ".jpg" || ext == ".jpeg") ct = "image/jpeg";
            else if (ext == ".gif") ct = "image/gif";

            std::ifstream f(filePath, std::ios::binary);
            if (!f) return false;
            std::ostringstream oss;
            oss << f.rdbuf();
            std::string body = oss.str();

            std::ostringstream resp;
            resp << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: " << ct << "\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;

            response = resp.str();
            return true;
        }
    }
    return false;
}

void FlaskCpp::sendResponse(int clientSocket, const std::string& content) {
    send(clientSocket, content.c_str(), content.size(), MSG_NOSIGNAL);
}

std::string FlaskCpp::generate404Error(const std::string& msg, bool gen_header) {
    std::ostringstream response;
    std::ostringstream body;
    body << R"(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>404 Not Found</title>
    <style>
        body {
            background-color: #f0f2f5;
            color: #333;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            text-align: center;
        }
        .container {
            background-color: #fff;
            padding: 40px 60px;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 {
            font-size: 80px;
            margin-bottom: 20px;
            color: #e74c3c;
        }
        p {
            font-size: 24px;
            margin-bottom: 30px;
        }
        a {
            display: inline-block;
            padding: 12px 25px;
            background-color: #3498db;
            color: #fff;
            text-decoration: none;
            border-radius: 4px;
            font-size: 18px;
            transition: background-color 0.3s ease;
        }
        a:hover {
            background-color: #2980b9;
        }
        .illustration {
            margin-bottom: 30px;
        }
        @media (max-width: 600px) {
            .container {
                padding: 20px 30px;
            }
            h1 {
                font-size: 60px;
            }
            p {
                font-size: 20px;
            }
            a {
                font-size: 16px;
                padding: 10px 20px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="illustration">
            <svg width="100" height="100" viewBox="0 0 24 24" fill="#e74c3c" xmlns="http://www.w3.org/2000/svg">
                <path d="M12 0C5.371 0 0 5.371 0 12c0 6.629 5.371 12 12 12s12-5.371 12-12C24 5.371 18.629 0 12 0zm5.707 16.293L16.293 17.707 12 13.414 7.707 17.707 6.293 16.293 10.586 12 6.293 7.707 7.707 6.293 12 10.586 16.293 6.293 17.707 7.707 13.414 12 17.707z"/>
            </svg>
        </div>
        <h1>404</h1>
        <p>)" << msg << R"(</p>
        <a href="/">Back to Main Page</a>
    </div>
</body>
</html>
)";
    if (gen_header)
    {
        response << "HTTP/1.1 404 Not Found\r\n"
                << "Content-Type: text/html; charset=UTF-8\r\n"
                << "Content-Length: " << body.str().size() << "\r\n"
                << "Connection: close\r\n\r\n";
    }
    response << body.str();
    return response.str();
}

std::string FlaskCpp::generate500Error(const std::string& msg, bool gen_header) {
    std::ostringstream response;
    std::ostringstream body;
    body << R"(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>500 Internal Server Error</title>
    <style>
        body {
            background-color: #f8d7da;
            color: #721c24;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            text-align: center;
        }
        .container {
            background-color: #f5c6cb;
            padding: 40px 60px;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            max-width: 600px;
            margin: 20px;
        }
        h1 {
            font-size: 80px;
            margin-bottom: 20px;
            color: #c82333;
        }
        p {
            font-size: 24px;
            margin-bottom: 30px;
        }
        a {
            display: inline-block;
            padding: 12px 25px;
            background-color: #c82333;
            color: #fff;
            text-decoration: none;
            border-radius: 4px;
            font-size: 18px;
            transition: background-color 0.3s ease;
        }
        a:hover {
            background-color: #a71d2a;
        }
        .illustration {
            margin-bottom: 30px;
        }
        @media (max-width: 600px) {
            .container {
                padding: 20px 30px;
            }
            h1 {
                font-size: 60px;
            }
            p {
                font-size: 20px;
            }
            a {
                font-size: 16px;
                padding: 10px 20px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="illustration">
            <!-- SVG-иллюстрация для визуального эффекта -->
            <svg width="100" height="100" viewBox="0 0 24 24" fill="#c82333" xmlns="http://www.w3.org/2000/svg">
                <path d="M12 0C5.371 0 0 5.371 0 12c0 6.629 5.371 12 12 12s12-5.371 12-12C24 5.371 18.629 0 12 0zm5.707 16.293L16.293 17.707 12 13.414 7.707 17.707 6.293 16.293 10.586 12 6.293 7.707 7.707 6.293 12 10.586 16.293 6.293 17.707 7.707 13.414 12 17.707z"/>
            </svg>
        </div>
        <h1>500</h1>
        <p>)" << msg << R"(</p>
        <a href="/">Back to Main page</a>
    </div>
</body>
</html>
)";
    if (gen_header)
    {
        response << "HTTP/1.1 500 Internal Server Error\r\n"
                 << "Content-Type: text/html; charset=UTF-8\r\n"
                 << "Content-Length: " << body.str().size() << "\r\n"
                 << "Connection: close\r\n\r\n";
    }
    response << body.str();
    
    return response.str();
}

// 解析Cookie的实现
void FlaskCpp::parseCookies(const std::string& cookieHeader, std::map<std::string, std::string>& cookies) {
    std::istringstream stream(cookieHeader);
    std::string pair;

    while (std::getline(stream, pair, ';')) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            // 删除键开头的空格
            key.erase(0, key.find_first_not_of(' '));
            cookies[key] = urlDecode(value);
        }
    }

}

// urlDecode 的实现
std::string FlaskCpp::urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded_char;
            i += 2;
        } else if (value[i] == '+') {
            result += ' ';
        } else {
            result += value[i];
        }
    }
    return result;
}

#ifdef ENABLE_PHP
// 通过php-cgi使用popen实现executePHP
std::string FlaskCpp::executePHP(const RequestData& reqData, const std::filesystem::path& scriptPath) {
    // 用于执行php-cgi的命令
    std::string command = "php-cgi " + scriptPath.string();

    // 打开进程以读取PHP脚本的输出
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return generate500Error("Failed to execute PHP script");
    }

    // 读取PHP脚本的输出
    std::string phpOutput;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        phpOutput += buffer;
    }

    int returnCode = pclose(pipe);
    if (returnCode != 0) {
        return generate500Error("PHP script execution failed");
    }

    // 检查PHP输出是否以“Status:”开始
    // 如果是，则提取状态并从输出中删除
    std::string statusLine = "HTTP/1.1 200 OK\r\n"; // 默认
    size_t statusPos = phpOutput.find("Status:");
    if(statusPos != std::string::npos){
        size_t endLine = phpOutput.find("\r\n", statusPos);
        if(endLine != std::string::npos){
            statusLine = phpOutput.substr(statusPos, endLine - statusPos) + "\r\n";
            phpOutput.erase(statusPos, endLine - statusPos + 2); // 从输出中删除状态行
        }
    }

    // 形成最终答复
    std::string finalResponse = statusLine + phpOutput;
    return finalResponse;
}
#endif
