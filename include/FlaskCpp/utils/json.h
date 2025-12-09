#ifndef FlaskCpp_UTILS_JSON
#define FlaskCpp_UTILS_JSON

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
// #include <ostream>
#include <regex>
#include <unordered_map>

namespace flaskcpp
{
class JsonGenerator
{
public:
    JsonGenerator(int precision=8) {setPrecision(precision);}

    void setPrecision(int precision=8){this->precision = precision;}

    template<typename T>
    std::string add(std::string key, std::vector<T> values)
    {
        std::ostringstream oss;
        oss << "[";
        int i=0;
        for (T& value: values)
        {
            if (i) oss << ",";
            oss << this->add("", value);
            i++;
        }
        oss << "]";
        if (key.size()) json_data[key] = oss.str();
        return oss.str();
    }

    std::string add(std::string key, bool value)
    {
        if (key.size()) json_data[key] = std::string(value?"true":"false");
        return std::string(value?"true":"false");
    }

    std::string add(std::string key, JsonGenerator& json)
    {
        // if (this == &json) 
        // {
        //     std::cerr << "can not add self!" << std::endl;
        //     return json.toString();
        // }
        if (key.size()) json_data[key] = json.toString();
        return json.toString();
    }

    template<typename T>
    std::string add(std::string key, T value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(this->precision);
        oss << value;
        std::string v = oss.str();
        if (key.size()) json_data[key] = v;
        return v;
    }

    std::string toString()
    {
        std::ostringstream oss;
        oss << "{";
        int i=0;
        for(const auto& pair : json_data)
        {
            if (i) oss << ",";
            auto sec = pair.second;
            if (!isNumber(sec)) sec = std::string("\"") + sec + "\"";
            oss << "\"" << pair.first << "\":" << sec;
            i++;
        }
        oss << "}";
        return oss.str();
    }

    std::map<std::string,std::string> toMap()
    {
        std::map<std::string,std::string> map_;
        for(const auto& pair : json_data)
        {
            map_[pair.first] = pair.second;
        }
        return map_;
    }

    void clear()
    {
        json_data.clear();
    }

private:
    std::unordered_map<std::string, std::string> json_data;
    int precision=8;


    bool isNumber(const std::string& str) {
        std::regex pattern(R"([-+]?\d+(\.\d+)?)");
        return std::regex_match(str, pattern);
    }
};


}





#endif