#include "FlaskCpp/utils/file.h"
#include <sys/stat.h>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace flaskcpp
{

static inline bool openFileIfClosed(std::ifstream& f, const std::string& filename, size_t& current_pos, size_t start=0) {
    if (!f.is_open()) {
        f.open(filename, std::ios::binary);
        if (!f.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }
        current_pos = 0;
        if (start)
        {
            f.seekg(start, std::ios::beg);
            current_pos = start;
        }
    }
    return true;
}

static inline bool closeFileIfOpened(std::ifstream& f) {
    if (f.is_open()) {
        f.close();
        return true;
    }
    return false;
}


FileHandler::FileHandler(std::string path, std::string file_name, bool as_attachment)
{
    init(path, file_name, as_attachment);
}

void FileHandler::setRange(size_t start, size_t end)
{
    if (end_ < start_) return;
    start_ = start;
    end_ = end;
}

void FileHandler::init(std::string path, std::string file_name, bool as_attachment)
{
    closeFileIfOpened(file);
    this->path = path;
    this->file_name = file_name;
    this->as_attachment = as_attachment;
    if (!this->file_name.size())
    {
        int k = -1;
        for (int i=0;i<path.size();i++)
        {
            if (path[i] == '/')
            {
                k = i;
            }
        }
        this->file_name = path.substr(k+1);
    }
    content_type = getFileTypeString(FLASK_FILE_AUTO, this->file_name);
    // std::cout << file_name << " " << content_type << std::endl;
    init_ = true;
}

bool FileHandler::isFileExist()
{
    if (!init_) return false;
    if (!file_data.empty()) return true;
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // 目录不存在
    } else if (info.st_mode & S_IFDIR) {
        return false; // 目录存在
    } else {
        return true; // 路径存在但不是目录
    }
}

void FileHandler::setFileData(std::vector<char> data)
{
    this->file_data = data;
}

std::string FileHandler::generateHeader(const std::vector<std::pair<std::string, std::string>>& extra_headers)
{
    std::ostringstream response;
    bool no_partial = start_ < 0 && end_ < 0;
    if (no_partial)
    {
        response << "HTTP/1.1 200 OK" << "\r\n";
    }
    else
    {
        response << "HTTP/1.1 206 Partial Content" << "\r\n";
        
    }
    
    response << "Content-Type: " << content_type;

    // Добавляем charset=utf-8 для текстовых типов контента
    if (content_type.find("text/") != std::string::npos || content_type.find("application/json") != std::string::npos) {
        response << "; charset=utf-8";
    }
    response << "\r\n";

    if (!file_data.empty())
    {
        if (no_partial)
        {
            response << "Content-Length: " << file_data.size() << "\r\n";
        }
        else
        {
            if (end_ >= file_data.size())
            {
                end_ = file_data.size() - 1;
                if (start_ > end_) return "";
            }
            response << "Accept-Ranges: bytes\r\n";
            response << "Content-Length: " << (end_>0?end_:(file_data.size()-1)) - start_ + 1 << "\r\n";
            response << "Content-Range: " << start_ << "-" << (end_>0?end_:(file_data.size()-1)) << "/" << file_data.size() << "\r\n";
        }
    }
    else
    {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) == 0) 
        {
            if (no_partial)
            {
                response << "Content-Length: " << buffer.st_size << "\r\n";
            }
            else
            {
                if (end_ >= buffer.st_size)
                {
                    end_ = buffer.st_size - 1;
                    if (start_ > end_) return "";
                }
                response << "Accept-Ranges: bytes\r\n";
                response << "Content-Length: " << (end_>0?end_:(buffer.st_size-1)) - start_ + 1 << "\r\n";
                response << "Content-Range: " << start_ << "-" << (end_>0?end_:(buffer.st_size-1)) << "/" << buffer.st_size << "\r\n";
            }
        }
        else
        {
            return "";
        }
    }

    auto fileExtra = genFileExtraSettings(file_name, as_attachment);
    response << fileExtra.first << ": " << fileExtra.second << "\r\n";

    // Добавление дополнительных заголовков, включая несколько Set-Cookie
    for (const auto& header : extra_headers) {
        if (header.first.empty()) continue;
        response << header.first << ": " << header.second << "\r\n";
    }

    response << "Connection: close\r\n\r\n";

    return response.str();
}


FileHandler::~FileHandler()
{
    closeFileIfOpened(file);
}

void FileHandler::copyTo(FileHandler& hdl)
{
    closeFileIfOpened(file);
    hdl.init(path, file_name, as_attachment);
    if (start_ > 0)
    {
        hdl.setStart(start_);
    }
    if (end_ > 0)
    {
        hdl.setEnd(end_);
    }
}

void FileHandler::setStart(size_t start)
{
    start_ = start;
}

void FileHandler::setEnd(size_t end)
{
    if (end > start_) end_ = end;
}

size_t FileHandler::read(std::vector<char>& data)
{
    if (end_read || (current_pos > end_))
    {
        return 0;
    }
    
    if (file_data.empty())
    {
        bool success = openFileIfClosed(file, path, current_pos, ((start_>0)?start_:0));
        if (!success)
        {
            closeFileIfOpened(file);
            end_read = true;
            return 0;
        }
    }

    size_t sz = data.size();
    if (!sz)
    {
        sz = 8192;
        data.resize(sz);
    }

    if (end_ > 0)
    {
        if (end_ - current_pos + 1 < sz)
        {
            sz = end_ - current_pos+1;
            data.resize(sz);
        }
    }
    size_t bytes_read;

    if (file_data.empty())
    {
        file.read(data.data(), sz);
        bytes_read = file.gcount();
    }
    else
    {
        bytes_read = ((current_pos + sz) < file_data.size())?sz:file_data.size()-current_pos;
        // std::cout << bytes_read << ", " << sz << ", " << current_pos << std::endl;
        if (bytes_read)
        {
            memcpy(data.data(), file_data.data()+current_pos, bytes_read);
        }
    }
    
    if (!bytes_read)
    {
        closeFileIfOpened(file);
        end_read = true;
    }
    else
    {
        current_pos += bytes_read;
    }
    return bytes_read;
}


static inline bool isdir(std::string path) 
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

static inline bool isfile(std::string path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // 目录不存在
    } else if (info.st_mode & S_IFDIR) {
        return false; // 目录存在
    } else {
        return true; // 路径存在但不是目录
    }
}

static inline bool create_directory_recursive(const std::string& path, bool is_path_file=false) {
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


std::string save_file(const RequestData::File& file, const std::string& path, bool path_is_file)
{
    std::string p = path;
    
    if (isdir(path) || !path_is_file)
    {
        if (p[p.size()-1] != '/')
        {
            p += "/";
        }
        p += file.file_name;
    }

    if (isfile(p))
    {
        return "file already exist!";
    }

    create_directory_recursive(p, true);

    std::ofstream f(p, std::ios::binary);
    if (!f) {
        std::cerr << "can not open: " << p << std::endl;
        return "can not open: " + p;
    }
    f.write(file.data.data(), file.data.size());
    f.close();
    return "";
}


}