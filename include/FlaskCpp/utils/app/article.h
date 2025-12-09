#ifndef FLASKCPP_UTILS_APP_ARTICLE_H
#define FLASKCPP_UTILS_APP_ARTICLE_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <sys/stat.h>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "../../TemplateEngine.h"
#include "../parse/json.hpp"
#include <dirent.h>

namespace flaskcpp
{
namespace app
{

class ArticleEngine
{
public:
    ArticleEngine() {}

    ArticleEngine(const std::string& root)
    {
        setRoot(root);
    }

    ~ArticleEngine()
    {
        // save
        saveAllData();
    }

    void setRoot(const std::string& root)
    {
        root_ = root;
        init_ = true;
        update();
    }

    bool isInit()
    {
        return init_;
    }

    std::vector<std::string> update()
    {
        // auto ret = glob(root_)[0];
        std::vector<std::string> update_info;
        auto ret = glob(root_)[0];
        for (auto it: ret)
        {
            std::string cur_path = root_ + "/" + it + "/";
            std::string content_path = cur_path + "content.md";
            std::string data_path = cur_path + "data.json";
            
            if (isfile(content_path) && isfile(data_path))
            {
                bool update_this_article = false;
                if (articles.find(it) == articles.end())
                {
                    update_info.push_back(it + " -> Add");
                    update_this_article = true;
                }
                else
                {
                    if(articles[it].last_edit_time_content != getFileModificationTime(content_path.c_str()) ||
                       articles[it].last_edit_time_data != getFileModificationTime(data_path.c_str()))
                    {
                        update_info.push_back(it + " -> Update");
                        update_this_article = true;
                    }
                    // else
                    // {
                    //     std::cout << "no need to update " << it << std::endl;
                    // }
                }
                if (update_this_article)
                {
                    articles[it] = readArticleData(content_path, data_path, it);
                }
            }
            
        }
        // 检查是否有被删掉的文章
        std::vector<std::string> a2delete;
        for (auto& it: articles)
        {
            std::string cur_path = root_ + "/" + it.first;
            if (!isdir(cur_path))
            {
                update_info.push_back(it.first + " -> Delete");
                a2delete.push_back(it.first);
            }
        }
        for(auto& a2d: a2delete)
        {
            articles.erase(a2d);
        }
        return update_info;
    }

    void singleArticleToContext(std::string id, TemplateEngine::Context& map, bool edit=false)
    {
        if (articles.find(id) == articles.end()) return;
        auto& ori_dat = articles[id];
        
        map["article_title"] = ori_dat.title;
        std::ostringstream info;
        info << "<i class=\"fas fa-calendar\"></i>&nbsp;" << fileTimeToString(ori_dat.last_edit_time_content, "%Y-%m-%d %H:%M:%S") << "&nbsp;&nbsp;&nbsp;&nbsp;";
        info << "<i class=\"fas fa-tag\"></i>&nbsp;" << ori_dat.tags << "&nbsp;&nbsp;&nbsp;&nbsp;"; // << "&nbsp;&nbsp;&nbsp;&nbsp;";
        info << "<i class=\"fas fa-eye\"></i>&nbsp;" << ori_dat.view_times;
        
        map["article_info"] = info.str();

        map["article_content"] = ori_dat.content;
        if (!edit)
        {
            ori_dat.view_times++;
            ori_dat.data_update = true;
        }
    }

    void setToContext(TemplateEngine::Context& map)
    {
        JsonList list;
        list.reserve(articles.size());
        // std::cout << articles.size() << std::endl;
        for (auto& article: articles)
        {
            // std::cout << "add: " << article.first << std::endl;
            list.push_back({
                {"title", article.second.title},
                {"abstract", article.second.abstract},
                {"view_times", std::to_string(article.second.view_times)},
                {"id", article.second.id},
                {"last_edit_time", fileTimeToString(article.second.last_edit_time_content, "%Y-%m-%d %H:%M:%S")},
                {"tags", article.second.tags}
            });
        }
        map["articles"] = list;
    }

    void saveAllData()
    {
        for(auto& article: articles)
        {
            if (!article.second.data_update && !article.second.content_update) continue;
            std::string cur_path = root_ + "/" + article.first + "/";
            std::string content_path = cur_path + "content.md";
            std::string data_path = cur_path + "data.json";

            if (article.second.data_update)
            {
                nlohmann::json data;
                data["title"] = article.second.title;
                data["abstract"] = article.second.abstract;
                data["view_times"] = article.second.view_times;
                data["id"] = article.second.id;
                data["tags"] = article.second.tags;

                std::ofstream file_json(data_path);
                if (file_json.is_open()) {
                    file_json << std::setw(4) << data << std::endl;
                    file_json.close();
                } else {
                    std::cerr << "Failed to open file '" << data_path << std::endl;
                }
            }
            if (article.second.content_update)
            {
                std::ofstream file_content(content_path);
                if (file_content.is_open())
                {
                    file_content << article.second.content;
                    file_content.close();
                }
                else
                {
                    std::cerr << "Failed to open file '" << content_path << std::endl;
                }
            }
            
        } 
    }


private:
    struct Data
    {
        std::string title;
        std::string id;
        std::string abstract;
        std::string tags;
        time_t last_edit_time_content, last_edit_time_data;
        size_t view_times;

        bool content_update=false, data_update=false;

        std::string content;
    };
    
    std::map<std::string, Data> articles;

    std::string root_;
    bool init_=false;

    time_t getFileModificationTime(const char* filePath) {
        struct stat fileStat;
        if (stat(filePath, &fileStat) == 0) {
            return fileStat.st_mtime;
        }
        return -1; // 文件不存在或错误
    }

    std::string fileTimeToString(time_t& time) {
        return std::ctime(&time);
    }

    std::string fileTimeToString(time_t& t, const std::string& format) {
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t), format.c_str());
        return oss.str();
    }

    Data readArticleData(std::string content_path, std::string data_path, std::string id)
    {
        Data ret;
        std::ifstream f(data_path), fc(content_path);
        nlohmann::json data = nlohmann::json::parse(f);
        
        if (!fc.is_open()) {
            ret.content = "no content";
        }
        std::stringstream buffer;
        buffer << fc.rdbuf();
        ret.content = buffer.str();
        ret.id = id;
        ret.title = data["title"];
        ret.view_times = data["view_times"];
        ret.tags = data["tags"];
        ret.abstract = (data["abstract"].is_string()?(std::string)data["abstract"]:ret.content.substr(0, 100));
        
        ret.last_edit_time_content = getFileModificationTime(content_path.c_str());
        ret.last_edit_time_data = getFileModificationTime(data_path.c_str());

        return std::move(ret);
    }

    bool isdir(const std::string& path) const
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

    bool isfile(const std::string& path) const {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            return false; // 目录不存在
        } else if (info.st_mode & S_IFDIR) {
            return false; // 目录存在
        } else {
            return true; // 路径存在但不是目录
        }
    }

    std::vector<std::vector<std::string>> glob(const std::string& path_)
    {
        std::string path = path_;
        std::vector<std::string> dirs, files;
        DIR *dir;
        struct dirent *entry;

        if ((dir = opendir(path.c_str())) == NULL) {
            std::cerr << "can not open dir: " << path << std::endl;
            return {};
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

};


}
}


#endif