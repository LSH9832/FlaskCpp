#include <FlaskCpp/FlaskTypes.h>
#include <sstream>
#include <vector>
#include <string>
// #include <

std::string stringLower(std::string src)
{
    std::string ret = "";
    for (int i=0;i<src.length();i++) {
        int assic = src[i];
        if (assic < 91 && assic > 64) {
            ret += (char)(assic + 32);
        }
        else ret += src[i];
    }
    return ret;
}

bool stringEndswith(std::string src, const std::string& suffix) {
    size_t str_len = src.length();
    size_t suffix_len = suffix.length();
    if (suffix_len > str_len) return false;
    return (src.find(suffix, str_len - suffix_len) == (str_len - suffix_len));
}


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
        if (stringEndswith(file_name, ".json"))
        {
            type = FLASK_FILE_APP_JSON;
        }
        else if (stringEndswith(file_name, ".jpg") || stringEndswith(file_name, ".jpeg"))
        {
            type = FLASK_FILE_JPEG;
        }
        else if (stringEndswith(file_name, "png"))
        {
            type = FLASK_FILE_PNG;
        }
        else if (stringEndswith(file_name, ".gif"))
        {
            type = FLASK_FILE_GIF;
        }
        else if (stringEndswith(file_name, ".webp"))
        {
            type = FLASK_FILE_WEBP;
        }
        else if (stringEndswith(file_name, ".pdf"))
        {
            type = FLASK_FILE_PDF;
        }
        else if (stringEndswith(file_name, ".doc") || stringEndswith(file_name, "docx"))
        {
            type = FLASK_FILE_MSWORD;
        }
        else if (stringEndswith(file_name, ".zip"))
        {
            type = FLASK_FILE_ZIP;
        }
        else if (stringEndswith(file_name, ".bmp"))
        {
            type = FLASK_FILE_BMP;
        }
        else if (stringEndswith(file_name, ".mp4"))
        {
            type = FLASK_FILE_MP4;
        }
        else if (stringEndswith(file_name, ".mp3"))
        {
            type = FLASK_FILE_MP3;
        }
        else if (stringEndswith(file_name, ".wav"))
        {
            type = FLASK_FILE_WAV;
        }
        else if (stringEndswith(file_name, ".ogg"))
        {
            type = FLASK_FILE_AUDIO_OGG;  // 该格式不建议自动识别
        }
        else if (stringEndswith(file_name, ".webm"))
        {
            type = FLASK_FILE_VIDEO_WEBM;   // 同上，不建议
        }
        else if (stringEndswith(file_name, ".avi"))
        {
            type = FLASK_FILE_AVI;
        }
        else if (stringEndswith(file_name, ".html"))
        {
            type = FLASK_FILE_TEXT_HTML;
        }
        else if (stringEndswith(file_name, ".css"))
        {
            type = FLASK_FILE_TEXT_CSS;
        }
        else if (stringEndswith(file_name, ".js"))
        {
            type = FLASK_FILE_TEXT_JS;
        }
        else if (stringEndswith(file_name, ".csv"))
        {
            type = FLASK_FILE_TEXT_CSV;
        }
        else if (stringEndswith(file_name, ".xml"))
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