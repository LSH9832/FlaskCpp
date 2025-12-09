#include "./easycpp/argparse.h"
#include "FlaskCpp/FlaskCpp.h"
#include "FlaskCpp/utils/app/article.h"

#include <sys/stat.h>
#include <atomic>
#include <signal.h>
#include <dirent.h>
#include <vector>
#include <csignal>
#include <yaml-cpp/yaml.h>

// #define FLASKCPP_DEBUG

std::atomic<bool> globalRunning(true);
std::ofstream log_file;
using strpair = std::pair<std::string, std::string>;

argparse::ArgumentParser parse_args(int argc, char** argv)
{
    argparse::ArgumentParser parser("flaskcpp parser", argc, argv);
    parser.add_argument({"--name"}, "flask_server", "flask server name");
    parser.add_argument({"--port"}, 8080, "service port");
    parser.add_argument({"--config"}, std::string(__FILE__).substr(0, std::string(__FILE__).size()-21) + "config/config.yaml", "yaml config path");
    parser.add_argument({"-t", "--template"}, std::string(__FILE__).substr(0, std::string(__FILE__).size()-21) + "template", "path to your template dir");
    parser.add_argument({"-f", "--file"}, std::string(__FILE__).substr(0, std::string(__FILE__).size()-21) + "files", "file path");
    parser.add_argument({"--file-route"}, "/files", "request route for files");
    parser.add_argument({"--add-log"}, "", "log_path");
    // parser.add_argument({"--article-menu-route"}, "./article_menu.html", "article menu route");
    // parser.add_argument({"--article-route"}, "/article.html", "article_route");
    parser.add_argument({"--log-level"}, 1, "log level 0:debug, 1:info, 2: success, 3: warning, 4: error");
    return parser.parse_args();
}

void signalHandler(int signum) 
{
    switch (signum)
    {
    case SIGINT:
        fprintf(stderr, "\nKeyboard Interrupt.\n");
        break;
    case SIGTERM:
        fprintf(stderr, "\nTerminate.\n");
        break;
    default:
        break;
    }
    globalRunning = false;
}

static bool isdir(std::string path) 
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // 目录不存在
    } else if (info.st_mode & S_IFDIR) {
        return true; // 目录存在
    } else {
        return false; // 路径存在但不是目录
    }
}

static bool isfile(std::string path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // 目录不存在
    } else if (info.st_mode & S_IFDIR) {
        return false; // 目录存在
    } else {
        return true; // 路径存在但不是目录
    }
}

bool create_directory_recursive(const std::string& path, bool is_path_file=false) {
    std::string current_path;
    std::vector<std::string> dirs;
    
    // 分割路径
    size_t pos = 0;
    while (pos < path.length()) {
        size_t found = path.find('/', pos);
        if (found == std::string::npos) {
            dirs.push_back(path.substr(pos));
            break;
        }
        dirs.push_back(path.substr(pos, found - pos));
        pos = found + 1;
    }
    
    // 逐级创建目录
    int count = 0;
    for (const auto& dir : dirs) {
        if (is_path_file && ++count == dirs.size()) break;
        if (dir.empty()) continue;
        
        current_path += dir + "/";
        
        // 检查目录是否已存在
        struct stat st;
        if (stat(current_path.c_str(), &st) == 0) {
            if (!S_ISDIR(st.st_mode)) {
                std::cerr << "Error: " << current_path << " exists but is not a directory" << std::endl;
                return false;
            }
        } else {
            // 创建目录
            if (mkdir(current_path.c_str(), 0755) != 0) {
                std::cerr << "Error: Failed to create directory " << current_path << std::endl;
                return false;
            }
            // std::cout << "Created: " << current_path << std::endl;
        }
    }
    
    return true;
}

std::vector<std::vector<std::string>> glob(std::string path, std::string root="/")
{
    std::vector<std::string> dirs, files;
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path.c_str())) == NULL) {
        std::cerr << "can not open dir: " << path << std::endl;
        return {};
    }

    if (path != root)
    {
        dirs.push_back("..");
    }

    if (path[path.size()-1] != '/')
    {
        path += "/";
    }

    while ((entry = readdir(dir)) != NULL) {
        // 忽略 . 和 ..
        if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }
        
        // std::cout << entry->d_name << std::endl;
        (isdir(path + entry->d_name)?dirs:files).push_back(entry->d_name);
    }

    closedir(dir);

    return {dirs, files};
}

TemplateEngine::Context parse_config(std::string config_path, bool verbose=false)
{
    YAML::Node cfg = YAML::LoadFile(config_path);
    TemplateEngine::Context ctx;
    for(auto item: cfg)
    {
        
        if (item.second.IsScalar())
        {
            if (verbose) 
                std::cout << item.first.as<std::string>() << ": " << item.second.as<std::string>() << std::endl;
            ctx[item.first.as<std::string>()] = item.second.as<std::string>();
        }
        else if (item.second.IsSequence())
        {
            JsonList json_list;
            json_list.reserve(item.second.size());
            for(int i=0;i<item.second.size();++i)
            {
                std::map<std::string,std::string> json;
                for (auto sub_item: item.second[i])
                {
                    if (verbose) 
                        std::cout << item.first.as<std::string>() << "[" << i << "]." 
                                  << sub_item.first.as<std::string>() << ": " 
                                  << sub_item.second.as<std::string>() << std::endl;
                    json[sub_item.first.as<std::string>()] = sub_item.second.as<std::string>();
                }
                json_list.push_back(json);
            }
            ctx[item.first.as<std::string>()] = json_list;
        }
        else
        {
            printf("can not parse '%s'\n", item.first.as<std::string>().c_str());
        }
        
    }
    return ctx;
}

void update_config(std::string config_path, TemplateEngine::Context& ctx, bool verbose=false)
{
    YAML::Node cfg = YAML::LoadFile(config_path);
    for(auto item: cfg)
    {
        
        if (item.second.IsScalar())
        {
            if (verbose) 
                std::cout << item.first.as<std::string>() << ": " << item.second.as<std::string>() << std::endl;
            ctx[item.first.as<std::string>()] = item.second.as<std::string>();
        }
        else if (item.second.IsSequence())
        {
            JsonList json_list;
            json_list.reserve(item.second.size());
            for(int i=0;i<item.second.size();++i)
            {
                std::map<std::string,std::string> json;
                for (auto sub_item: item.second[i])
                {
                    if (verbose) 
                        std::cout << item.first.as<std::string>() << "[" << i << "]." 
                                  << sub_item.first.as<std::string>() << ": " 
                                  << sub_item.second.as<std::string>() << std::endl;
                    json[sub_item.first.as<std::string>()] = sub_item.second.as<std::string>();
                }
                json_list.push_back(json);
            }
            ctx[item.first.as<std::string>()] = json_list;
        }
        else
        {
            printf("can not parse '%s'\n", item.first.as<std::string>().c_str());
        }
        
    }
}

void update_template(FlaskCpp& app, std::string file_route, TemplateEngine::Context& ctx)
{
    JsonList js_list;
    js_list.push_back({{"name", file_route.substr(1)}});
    for (auto name: app.getAllTemplateNames())
    {
        js_list.push_back({{"name", name}});
    }
    ctx["websites"] = js_list;
}

bool isLogin(TemplateEngine::Context& ctx, const std::map<std::string, std::string>& session)
{
    bool ret = false;
    if(session.find("username") != session.end() && session.find("password") != session.end())
    {
        if (ctx.find("__username__") != ctx.end() && ctx.find("__password__") != ctx.end())
        {
            ret = (session.at("username") == std::get<std::string>(ctx["__username__"]) &&
                session.at("password") == std::get<std::string>(ctx["__password__"]));
        }
    }
    return ret;
}

bool isLogin(TemplateEngine::Context& ctx, const std::string& username, const std::string& password)
{
    return isLogin(ctx, {{"username", username}, {"password", password}});
}

std::string parse_size(double size)
{
    const std::vector<std::string> unit = {"B", "kB", "MB", "GB", "TB", "PB"};
    int i=0;
    while (i < unit.size() && size >= 1024)
    {
        size /= 1024;
        i++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << unit[i];
    return oss.str();
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // URLSafeSerializer ser;
    // ser.setKey("ILoveYou");

    flaskcpp::app::ArticleEngine articleEngine;

    // std::string ser_str = ser.map2str({{"i", "love"}, {"you", "girl"}});
    // // ser_str = "12345";
    // std::string session_str = ser.dumps(ser_str);

    
    
    // std::cout << session_str << std::endl;
    
    // std::string uncode_str = ser.loads(session_str);
    
    // std::map<std::string, std::string> ret_map;
    // ser.str2map(uncode_str, ret_map);
    
    // for (auto& it:ret_map)
    // {
    //     std::cout<< it.first << ": " << it.second << std::endl;
    // }
    // return 0;

    
    auto args = parse_args(argc, argv);

    auto ctx = parse_config(args["config"]);

    std::string file_path = args["file"];
    std::string template_path = args["template"];
    std::string log_path = args["add-log"];
    std::string file_route = args["file-route"];

    int log_level = args["log-level"];
    if (file_route.empty()) file_route = "/files";
    if (file_route[0] != '/') file_route = std::string("/") + file_route;
    if (file_route[file_route.size()-1] == '/') file_route = file_route.substr(0, file_route.size()-1);
    ctx["file_route"] = file_route;

    if (log_path.size())
    {
        if (create_directory_recursive(log_path, true))
        {
            log_file.open(log_path, std::ios::app);
        }
    }
    if (file_path[file_path.size()-1] != '/')
    {
        file_path += "/";
    }
    if (template_path[template_path.size()-1] != '/')
    {
        template_path += "/";
    }

    FlaskCpp app((std::string)args["name"]);

    app.setSecretKey("FlaskCppTestSecretKey");

    app.setLog([&](flaskcpp::LogMsg msg) {
        if (msg.level < log_level) return;
        static const std::vector<std::string> color = {"\033[34m", "\033[37m", "\033[32m", "\033[33m", "\033[31m"};
        static const std::vector<std::string> level = {"DEBUG  ", "INFO   ", "SUCCESS", "WARNING", "ERROR  "};

        std::ostringstream oss;
        auto now = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&current_time);
        oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

        std::string file = msg.file;
        int i = file.size();
        while (file[--i] != '/');
        file = file.substr(i+1, file.size()-i-1);
#ifdef FLASKCPP_DEBUG
        fprintf(stdout, "\033[32m%s\033[0m | %s\033[1m%s\033[0m | \033[36m%s:%d:<%s>\033[0m - %s\033[1m%s\033[0m\n", 
               oss.str().c_str(), color[msg.level].c_str(), level[msg.level].c_str(), file.c_str(), msg.line, 
               msg.function.c_str(), color[msg.level].c_str(), msg.content.c_str());
#else
        fprintf(stdout, "\033[32m%s\033[0m | %s\033[1m%s\033[0m | %s\033[1m%s\033[0m\n", 
               oss.str().c_str(), color[msg.level].c_str(), level[msg.level].c_str(), 
               color[msg.level].c_str(), msg.content.c_str());
#endif
        if (log_file.is_open())
        {
#ifdef FLASKCPP_DEBUG
            log_file << oss.str() << " | " << level[msg.level]
                     << " | " << level[msg.level] << " | " << msg.file << ":" << msg.line
                     << ":" << msg.function.c_str() << " - " << msg.content.c_str() << "\n";
#else
            log_file << oss.str() << " | " << level[msg.level]
                     << " | " << msg.content.c_str() << "\n";
#endif
            log_file.flush();
        }
        
    });

    app.setConfigUpdateListener(args["config"], [&](const std::string& config_path) {
        std::ostringstream oss;
        oss << "update config '" << config_path << "'";
        app.log({1, (oss.str()), __LINE__, __FILE__, __func__});
        update_config(config_path, ctx);
    });
    app.setTemplateChangedCallback([&](const std::string& template_name) {
        update_template(app, file_route, ctx);
    });
    app.loadTemplatesFromDirectory(args["template"]);

    app.route2(file_route + "/<path:address>", [&](const RequestData& req) {
        auto ctx_ = ctx;
        ctx_["isLogin"] = isLogin(ctx_, req.session);
        if (!std::get<bool>(ctx_["isLogin"]))
        {
            return flaskcpp::send_error(
                app.jump_to("/", "You are not login!", 2),
                flaskcpp::RESP_TYPE_FORBIDDEN, {FLASK_NO_CACHE}
            );
        }
        std::string this_route = req.routeParams.at("address");
        std::string address = file_path + this_route;
        
        // std::cerr << file_path << std::endl;

        bool as_attachment = true;

        

        if (isdir(address))
        {
            std::ostringstream pathList;
            pathList << "<a href='/' class=\"return-link\">Back to Main Page</a>\n";
            pathList << "<h1>Current Path: /" << this_route << "</h1>\n";
            

            // 
            auto res = glob(address, file_path);
            // 
            if (res[0].size())
                pathList << "<h2>Dirs</h2>\n";
            for(auto dir: res[0])
            {
                pathList << "<a href='" << std::string(args["file-route"]) 
                         << "/" << this_route << (this_route.size()?"/":"") << dir << "'>" << dir << "</a>\n";
            }
            // 
            if (res[1].size())
                pathList << "<h2>Files</h2>\n";
            for(auto file: res[1])
            {
                pathList << "<a href='" << std::string(args["file-route"]) 
                         << "/" << this_route << (this_route.size()?"/":"") << file << "'>" << file << "</a>\n";
            }
            // 
            std::string html = std::string(R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Files</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f9;
            margin: 0;
            padding: 20px;
        }
        h1, h2 {
            color: #333;
        }
        a {
            display: block;
            margin: 10px 0;
            padding: 10px;
            background-color: #fff;
            border-radius: 5px;
            text-decoration: none;
            color: #007BFF;
            transition: background-color 0.3s;
        }
        a:hover {
            background-color: #e0e0e0;
        }
        .return-link {
            display: inline-block;
            margin-bottom: 20px;
            padding: 10px 20px;
            background-color: #007BFF;
            color: #fff;
            border-radius: 5px;
            text-decoration: none;
        }
        .return-link:hover {
            background-color: #0056b3;
        }
    </style>
</head>
<body>
)") + pathList.str() + R"(
</body>
</html>
)";
            return flaskcpp::send_text(html, {FLASK_NO_CACHE});
        }
        else if (isfile(address))
        {
            if (req.queryParams.find("method") != req.queryParams.end())
            {
                if (!strcmp(req.queryParams.at("method").c_str(), "view"))
                {
                    as_attachment = false;
                }
            }
            return flaskcpp::send_file(address, "", as_attachment, {FLASK_NO_CACHE});
        }
        // std::cout << "no match" << std::endl;
        return flaskcpp::Response();
    });


    if (ctx.find("article_path") != ctx.end() &&
        ctx.find("article_route") != ctx.end() &&
        ctx.find("article_menu_route") != ctx.end())
    {
        articleEngine.setRoot(std::get<std::string>(ctx["article_path"]));

        app.addCheckTask([&articleEngine, &app]{
            for(auto msg: articleEngine.update())
            {
                app.log({1, "Article Change: " + msg, __LINE__, __FILE__, __func__});
            }
        });
        // app.route2(std::get<std::string>(ctx["article_route"]), [&](const RequestData& req) {
        // });
    }
    
    app.route2("/login_out", [&](const RequestData& req) {

        auto ctx_ = ctx;
        bool is_login = !isLogin(ctx_, req.session);

        if (is_login)
        {
            if (req.json.find("username") == req.json.end() || 
                req.json.find("password") == req.json.end())
            {
                is_login = false;
            }
            else
            {
                is_login = isLogin(ctx_, req.json.at("username"), req.json.at("password"));
            }
        }
        ctx_["isLogin"] = is_login;

        // for (auto& it: req.formData)
        // {
        //     std::cout << it.first << ": " << it.second << std::endl;
        // }

        // for (auto& it: req.queryParams)
        // {
        //     std::cout << it.first << ": " << it.second << std::endl;
        // }
        // std::cout << req.body << std::endl;

        
        
        std::map<std::string, std::string> temp_session;
        if (ctx_.find("__username__") != ctx_.end() && ctx_.find("__password__") != ctx_.end())
        {
            temp_session["username"] = std::get<std::string>(ctx_["__username__"]);
            temp_session["password"] = std::get<std::string>(ctx_["__password__"]);
        }

        std::string jump_html = "/";
        if (req.headers.find("Host") != req.headers.end() && 
            req.headers.find("Referer") != req.headers.end())
        {
            std::string host = req.headers.at("Host");
            std::string refer = req.headers.at("Referer");
            size_t loc = refer.find(host);
            if (loc != refer.npos)
            {
                jump_html = refer.substr(loc + host.size());
            }
        }
        // for (auto& h: req.headers)
        // {
        //     std::cout << h.first << ": " << h.second << std::endl;
        // }

        if (req.queryParams.find("ret") != req.queryParams.end())
        {
            if (!strcmp(req.queryParams.at("ret").c_str(), "json"))
            {
                flaskcpp::JsonGenerator json;
                json.add("login", is_login);
                if (req.queryParams.find("redirect") != req.queryParams.end())
                {
                    json.add("redirect", req.queryParams.at("redirect"));
                }
                return flaskcpp::send_text(
                    json, 
                    {(is_login?app.generateSession(temp_session):(strpair)FLASK_DELETE_SESSION)}
                );
            }
        }
        
        return flaskcpp::send_text(
            app.jump_to(jump_html, (std::string("status: ") + (is_login?"login":"logout")), 1), 
            {FLASK_NO_CACHE, (is_login?app.generateSession(temp_session):(strpair)FLASK_DELETE_SESSION)}
        );
    });

    app.route2("/upload", [&](const RequestData& req) {
        // req.
        // std::cout << req.body << std::endl;
        // for (auto& it:req.formData)
        // {
        //     std::cout << "size of '" << it.first << "': " << it.second.size() << std::endl;
        // }
        flaskcpp::JsonGenerator json;
        
        for (auto& it: req.files)
        {
            std::ostringstream oss;
            oss << it.first << ": size -> " << parse_size(it.second.data.size());
            app.log({0, oss.str(), __LINE__, __FILE__, __func__});

            std::string reason = flaskcpp::save_file(it.second, file_path);
            if (!reason.empty())
            {
                json.add("message", it.second.file_name + ": " + reason);
                oss.str("");
                oss << it.second.file_name << ": " << reason;
                app.log({4, oss.str(), __LINE__, __FILE__, __func__});
                return flaskcpp::send_error(json, flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR);
            }
            oss.str("");
            oss << it.first << " saved to " << file_path;
            app.log({0, oss.str(), __LINE__, __FILE__, __func__});
        }   

        json.add("message", "success");
        return flaskcpp::send_text(json, {FLASK_NO_CACHE});
    });

    app.route2("/", [&](const RequestData& req) {
        // update_config(args["config"], ctx);
        // std::cout << "------------------------------------" << std::endl;
        // // std::cout << "session data:" << std::endl;
        // for(auto& c: req.session)
        // {
        //     std::cout << c.first << ": " << c.second << std::endl;
        // }
        // std::cout << "------------------------------------" << std::endl;

        // if(req.cookies.find("token") != req.cookies.end())
        // {
        //     auto token = req.cookies.at("token");
        //     std::cout << token << std::endl;
        // }
        auto ctx_ = ctx;
        ctx_["isLogin"] = isLogin(ctx_, req.session);
        if (req.queryParams.find("redirect") != req.queryParams.end())
        {
            ctx_["redirect_route"] = req.queryParams.at("redirect");
        }
        else
        {
            ctx_["redirect_route"] = req.path;
        }
        
        std::string address = template_path + "index.html";
        if (isfile(address))
        {
            auto res = app.renderTemplate("index.html", ctx_);
            // std::map<std::string, std::string> cookies={};
            
            return flaskcpp::send_text(res, {FLASK_NO_CACHE});
            // return flaskcpp::send_file(address, "", false, {FLASK_NO_CACHE});
        }
        return flaskcpp::send_text(
            "<!DOCTYPE html><html><header><title>Main Page</title></header><body><h1>Hello FlaskCpp!</h1></body></html>", 
            {FLASK_NO_CACHE}
        );
    });


    app.route2("/<path:html>", [&](const RequestData& req) {
        auto ctx_ = ctx;
        ctx_["isLogin"] = isLogin(ctx_, req.session);
        
        if (req.queryParams.find("redirect") != req.queryParams.end())
        {
            ctx_["redirect_route"] = req.queryParams.at("redirect");
        }
        else
        {
            ctx_["redirect_route"] = req.path;
        }

        if (articleEngine.isInit())
        {
            if (!strcmp(req.path.c_str(), std::get<std::string>(ctx["article_menu_route"]).c_str()))
            {
                // articleEngine.update();
                articleEngine.setToContext(ctx_);
            }
            else if (!strcmp(req.path.c_str(), std::get<std::string>(ctx["article_route"]).c_str()))
            {
                if (req.queryParams.find("id") != req.queryParams.end())
                {
                    std::string article_id = req.queryParams.at("id");
                    // articleEngine.update();
                    articleEngine.singleArticleToContext(article_id, ctx_);
                }
            }
        }
        
        // update_config(args["config"], ctx);
        std::string html_file = req.routeParams.at("html");
        std::string address = template_path + html_file;

        if (isfile(address))
        {
            auto res = app.renderTemplate(html_file, ctx_);
            if (res.size())
                return flaskcpp::send_text(res, {FLASK_NO_CACHE});
            return flaskcpp::send_file(address, "", false, {FLASK_NO_CACHE});
        }

        return flaskcpp::send_error(
            app.generate404Error(std::string("can not find file '") + html_file + "'", false), 
            flaskcpp::RESP_TYPE_NOT_FOUND
        );
    });

    update_template(app, file_route, ctx);
    
    
    app.runAsync(args["port"], true, true);

    while(globalRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    app.stop();
    articleEngine.saveAllData();
    if(log_file.is_open()) log_file.close();
    return 0;
}
