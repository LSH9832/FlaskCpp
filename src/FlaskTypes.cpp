#include <FlaskCpp/FlaskTypes.h>
#include <sstream>
#include <vector>
#include <string>
// #include <


std::pair<std::string, std::string> genFileExtraSettings(std::string fileName, bool as_attachment)
{
    std::ostringstream oss;
    if (as_attachment) oss <<"attachment; ";
    oss << "filename=\"" << fileName << "\"";
    return {"Content-Disposition", oss.str()};
}

std::string getFileTypeString(int type, std::string file_name)
{
    if (type<0)
    {
        std::string lower = stringLower(file_name);
        if (stringEndsWith(file_name, ".json"))
        {
            type = FLASK_FILE_APP_JSON;
        }
        else if (stringEndsWith(file_name, ".jpg") || stringEndsWith(file_name, ".jpeg"))
        {
            type = FLASK_FILE_JPEG;
        }
        else if (stringEndsWith(file_name, "png"))
        {
            type = FLASK_FILE_PNG;
        }
        else if (stringEndsWith(file_name, ".gif"))
        {
            type = FLASK_FILE_GIF;
        }
        else if (stringEndsWith(file_name, ".webp"))
        {
            type = FLASK_FILE_WEBP;
        }
        else if (stringEndsWith(file_name, ".pdf"))
        {
            type = FLASK_FILE_PDF;
        }
        else if (stringEndsWith(file_name, ".doc") || stringEndsWith(file_name, "docx"))
        {
            type = FLASK_FILE_MSWORD;
        }
        else if (stringEndsWith(file_name, ".zip"))
        {
            type = FLASK_FILE_ZIP;
        }
        else if (stringEndsWith(file_name, ".bmp"))
        {
            type = FLASK_FILE_BMP;
        }
        else if (stringEndsWith(file_name, ".mp4"))
        {
            type = FLASK_FILE_MP4;
        }
        else if (stringEndsWith(file_name, ".mp3"))
        {
            type = FLASK_FILE_MP3;
        }
        else if (stringEndsWith(file_name, ".wav"))
        {
            type = FLASK_FILE_WAV;
        }
        else if (stringEndsWith(file_name, ".ogg"))
        {
            type = FLASK_FILE_AUDIO_OGG;  // 该格式不建议自动识别
        }
        else if (stringEndsWith(file_name, ".webm"))
        {
            type = FLASK_FILE_VIDEO_WEBM;   // 同上，不建议
        }
        else if (stringEndsWith(file_name, ".avi"))
        {
            type = FLASK_FILE_AVI;
        }
        else if (stringEndsWith(file_name, ".html"))
        {
            type = FLASK_FILE_TEXT_HTML;
        }
        else if (stringEndsWith(file_name, ".css"))
        {
            type = FLASK_FILE_TEXT_CSS;
        }
        else if (stringEndsWith(file_name, ".js"))
        {
            type = FLASK_FILE_TEXT_JS;
        }
        else if (stringEndsWith(file_name, ".csv"))
        {
            type = FLASK_FILE_TEXT_CSV;
        }
        else if (stringEndsWith(file_name, ".xml"))
        {
            type = FLASK_FILE_TEXT_XML;
        }
        else
        {
            type = FLASK_FILE_DEFAULT;
        }
    }
    return __flaskFileTypeMap__[type];
    
}


int getContentTypeByString(const std::string& content_type)
{
    int index = 0;
    for(auto& t: __flaskFileTypeMap__)
    {
        if (!strcmp(t.c_str(), content_type.c_str()))
        {
            return index;
        }
        index++;
    }
    return 0;
}


bool isFormReq(RequestData& req)
{
    if (req.headers.find("Content-Type") != req.headers.end())
    {
        for(auto&line: stringSplit(req.headers.at("Content-Type"), "; "))
        {
            if (stringStartsWith(line, "boundary="))
            {
                return true;
            }
        }
    }
    return false;
}