#ifndef FLASKCPP_UTILS_FILE_H
#define FLASKCPP_UTILS_FILE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <FlaskCpp/FlaskTypes.h>

namespace flaskcpp
{

class FileHandler
{
public:
    FileHandler() {}

    ~FileHandler();

    FileHandler(std::string path, std::string file_name="", bool as_attachment=false);

    void init(std::string path, std::string file_name="", bool as_attachment=false);

    void setRange(size_t start, size_t end);

    void setStart(size_t start);

    void setEnd(size_t end);

    void copyTo(FileHandler& hdl);

    bool isFileExist();

    std::string generateHeader(const std::vector<std::pair<std::string, std::string>>& extra_headers={});

    size_t read(std::vector<char>& data);

    void setFileData(std::vector<char> data);
    
private:
    std::string path, file_name;
    bool as_attachment=false;
    std::string content_type;
    std::ifstream file;
    bool end_read=false;
    int64_t start_=-1, end_=-1;
    bool init_=false;

    std::vector<char> file_data={};

    size_t current_pos = 0;
};

std::string save_file(const RequestData::File& file, const std::string& path, bool path_is_file=false);

}


#endif // FLASKCPP_UTILS_FILE_H