/**
 * Author: https://github.com/LSH9832
 * 感谢 https://github.com/0382, 本代码借鉴了 https://github.com/0382/util/blob/main/cpp/argparse/argparse.hpp 思路并完全重构，更接近于python的argparse用法
 */
#pragma once
#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#define STORE_TRUE argparse::ACTION_OPT::ACTION_OPT_TRUE
#define STORE_FALSE argparse::ACTION_OPT::ACTION_OPT_FALSE

namespace argparse
{
    class __argparse_string__ 
    {

        std::string str_;

    public:

        __argparse_string__() {

        }

        __argparse_string__(std::string str) {
            str_ = str;
        }

        __argparse_string__(const char * str) {
            str_ = str;
        }

        __argparse_string__(bool str)
        {
            if (str) str_ = "true";
            else str_ = "false";
        }

        __argparse_string__(int str)
        {
            str_ = std::to_string(str);
        }

        __argparse_string__(double str)
        {
            str_ = std::to_string(str);
        }

        // __argparse_string__(std::vector<int> )

        bool operator==(__argparse_string__ &str) {
            return str_ == str.str();
        }

        bool operator==(std::string &str) {
            return str_ == str;
        }

        bool operator==(const char str[]) {
            return str_ == str;
        }

        __argparse_string__ operator+(__argparse_string__ &str) {
            return __argparse_string__(str_ + str.str());
        }

        __argparse_string__ operator+(std::string str) {
            return __argparse_string__(str_ + str);
        }

        __argparse_string__ operator+(const char * str) {
            return __argparse_string__(str_ + str);
        }

        __argparse_string__ operator+(const char c) {
            return __argparse_string__(str_ + c);
        }

        __argparse_string__ operator+(const int value) {
            return __argparse_string__(str_ + std::to_string(value));
        }

        __argparse_string__ operator+(const double value) {
            return __argparse_string__(str_ + std::to_string(value));
        }

        void operator+=(__argparse_string__ str) {
            str_ += str.str();
        }

        void operator+=(std::string str) {
            str_ += str;
        }

        void operator+=(const char * str) {
            str_ += str;
        }

        void operator+=(const char c) {
            str_ += c;
        }

        char& operator[](int idx){
            return str_[idx<0?str_.length()+idx:idx];
        }

        operator int() const {
            return std::atoi(str_.c_str());
        }

        operator float() const {
            return std::atof(str_.c_str());
        }

        operator double() const {
            return std::atof(str_.c_str());
        }

        operator std::string() const {
            return str_;
        }

        operator bool() const {
            auto s = __argparse_string__(str_).lower();
            if (s == "false" || s == "0")
            {
                return false;
            }
            else if (s == "true" || (s.isdigit() && (((int)s) > 0)))
            {
                return true;
            }
            else
            {
                std::cerr << "can not convert '" << str_ << "' to bool" << std::endl;
                exit(-1);
            }
        }

        __argparse_string__ substr(int start, int end)
        {
            return str_.substr(start, end);
        }

        __argparse_string__ operator*(int value) {
            std::ostringstream oss;
            if(str_.size())
            {
                for(int i=0;i<value;i++)
                {
                    oss << str_;
                }
            }
            return __argparse_string__(oss.str());
        }

        void operator*=(int value) {
            std::ostringstream oss;
            if(str_.size())
            {
                for(int i=0;i<value;i++)
                {
                    oss << str_;
                }
            }
            str_ = oss.str();
        }

        bool startswith(const std::string& prefix) {
            size_t str_len = str_.length();
            size_t prefix_len = prefix.length();
            if (prefix_len > str_len) return false;
            return str_.find(prefix) == 0;
        }

        bool endswith(const std::string& suffix) {
            size_t str_len = str_.length();
            size_t suffix_len = suffix.length();
            if (suffix_len > str_len) return false;
            return (str_.find(suffix, str_len - suffix_len) == (str_len - suffix_len));
        }

        bool startswith(__argparse_string__ &prefix) {
            size_t str_len = str_.length();
            size_t prefix_len = prefix.str().length();
            if (prefix_len > str_len) return false;
            return str_.find(prefix.str()) == 0;
        }

        bool endswith(__argparse_string__ &suffix) {
            size_t str_len = str_.length();
            size_t suffix_len = suffix.str().length();
            if (suffix_len > str_len) return false;
            return (str_.find(suffix.str(), str_len - suffix_len) == (str_len - suffix_len));
        }

        inline char &at(size_t idx) {
            return str_.at(idx);
        }

        inline void append(std::initializer_list<char> __l) {
            str_.append(__l);
        }

        size_t length() {
            return str_.length();
        }

        inline bool empty(){
            return str_.empty();
        }

        inline char &back() {
            return str_.back();
        }

        inline void pop_back() {
            return str_.pop_back();
        }

        friend std::ostream& operator<<(std::ostream& _os, __argparse_string__ obj) {
            _os << obj.str_;
            return _os;
        }

        std::vector<__argparse_string__> split(std::string delimiter) {
            if (str_.empty()) return {""};
            std::vector<__argparse_string__> tokens;
            size_t pos = 0;
            std::string::size_type prev_pos = 0;
            while ((pos = str_.find(delimiter, prev_pos)) != std::string::npos) {
                tokens.push_back(__argparse_string__(str_.substr(prev_pos, pos - prev_pos)));
                prev_pos = pos + delimiter.length();
            }
            if (prev_pos < str_.length()) tokens.push_back(__argparse_string__(str_.substr(prev_pos)));
            if (endswith(delimiter)) tokens.push_back("");
            return tokens;
        }

        std::vector<__argparse_string__> split(__argparse_string__ delimiter) {
            return this->split(delimiter.str());
        }

        std::vector<__argparse_string__> split(const char* delimiter) {
            return this->split(__argparse_string__(delimiter).str());
        }

        std::vector<__argparse_string__> split() {
            if (str_.empty()) return {""};
            std::vector<__argparse_string__> tokens;
            size_t pos = 0;
            int prev_pos = -1;

            for (size_t i=0; i<str_.length();i++) {
                if (i < prev_pos) continue;
                if ((pos = std::min(str_.find("\n", i), std::min(str_.find(" ", i), str_.find("\t", i)))) != std::string::npos) {
                    if (i == pos) {
                        prev_pos = i;
                        continue;
                    }
                    else {
                        __argparse_string__ substr = str_.substr(prev_pos+1, pos - prev_pos - 1);
                        if (substr.length()) {
                            tokens.push_back(substr);
                            prev_pos = pos;
                        }
                    }
                }
            }
            __argparse_string__ last_substr = str_.substr(prev_pos+1);
            if (prev_pos < str_.length() && last_substr.length()) tokens.push_back(last_substr);
            return tokens;
        }

        __argparse_string__ replace(std::string old_substr, std::string new_substr) {
            size_t pos = 0;
            while ((pos = str_.find(old_substr, pos)) != std::string::npos) {
                str_.replace(pos, old_substr.length(), new_substr);
                pos += new_substr.length(); // 更新位置以继续搜索
            }
            return str_;
        }

        __argparse_string__ replace(__argparse_string__ old_substr, __argparse_string__ new_substr) {
            return this->replace(old_substr.str(), new_substr.str());
        }

        __argparse_string__ replace(const char* old_substr, const char* new_substr) {
            return this->replace(__argparse_string__(old_substr), __argparse_string__(new_substr));
        }

        __argparse_string__ zfill(int len) {
            std::string ret = str_;
            while (ret.length() < len) {
                ret = "0" + ret;
            }
            return __argparse_string__(ret);
        }

        bool isdigit() {
            for (char c : str_) if (!std::isdigit(c)) return false;
            return true;
        }

        __argparse_string__ ljust(int length) {
            std::string ret = str_;
            if (str_.length()>=length) return str_;
            for (int i=0;i<length-str_.length();i++) ret += " ";
            return ret;
        }

        __argparse_string__ rjust(int length) {
            std::string ret = str_;
            if (str_.length()>=length) return str_;
            for (int i=0;i<length-str_.length();i++) ret = " " + ret;
            return ret;
        }

        const std::string str() {
            return str_;
        }

        inline const char* c_str() {
            return str_.c_str();
        }

        __argparse_string__ upper() {
            __argparse_string__ ret = "";
            for (int i=0;i<str_.length();i++) {
                int assic = str_[i];
                if (assic < 123 && assic > 96) {
                    ret += (char)(assic - 32);
                }
                else ret += str_[i];
            }
            return ret;
        }

        __argparse_string__ lower() {
            __argparse_string__ ret = "";
            for (int i=0;i<str_.length();i++) {
                int assic = str_[i];
                if (assic < 91 && assic > 64) {
                    ret += (char)(assic + 32);
                }
                else ret += str_[i];
            }
            return ret;
        }


    };


    enum class ACTION_OPT
    {
        ACTION_OPT_TRUE,
        ACTION_OPT_FALSE
    };

    class OneArgument
    {
    public:
        OneArgument() {}
        OneArgument(__argparse_string__ sname, __argparse_string__ name, __argparse_string__ value="", __argparse_string__ help="", bool isRequired=false, bool isPosition=false, __argparse_string__ nargs="1", bool isBool=false)
        {
            sname_ = sname;
            name_ = name;
            help_ = help;
            required = isRequired;
            nargs_ = nargs;
            if (!nargs_.isdigit())
            {
                bool flag = (nargs_ == "+") || (nargs_ == "*") || (nargs_ == "?");
                if (!flag)
                {
                    std::cerr << "'" << name << "': nargs not valid!" << std::endl;
                    exit(-1);
                }
            }
            isBool_ = isBool;
            isPosition_ = isPosition;
            hasDefault = !value.empty();

            defaultValues_ = value.split(" ");
            isNargs_ = defaultValues_.size() > 1;
            
        }

        bool isValid()
        {
            return (values_.size() || defaultValues_.size());
        }

        bool isPosArg()
        {
            return isPosition_;
        }

        int numArgs()
        {
            if (nargs_.isdigit())
            {
                return nargs_;
            }
            return -1;
        }

        bool isRequired()
        {
            return required;
        }

        bool isRequiredButNotSet()
        {
            return required && useDefault;
        }

        void parseValue(__argparse_string__ value="")
        {
            if (!value.empty())
            {
                values_ = value.split(" ");
                isNargs_ = values_.size() > 1;
                useDefault =false;
            }
        }

        size_t size()
        {
            return (useDefault?defaultValues_:values_).size();
        }

        OneArgument at(size_t idx)
        {
            if (!isNargs_)
            {
                std::cerr << name_ << " is not iterable." << std::endl;
                exit(-1);
            }
            __argparse_string__ subValue = (useDefault?defaultValues_:values_)[idx];
            return OneArgument("", name_ + "[" + std::to_string(idx) + "]", subValue, "", false, false);
        }

        OneArgument operator[](size_t idx)
        {
            return at(idx<0?(useDefault?defaultValues_:values_).size()+idx:idx);
        }

        template <class T>
        T as()
        {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (T)((useDefault?defaultValues_:values_)[0]);
        }

        operator int() const {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (int)((useDefault?defaultValues_:values_)[0]);
        }
    
        operator float() const {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (float)((useDefault?defaultValues_:values_)[0]);
        }
    
        operator double() const {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (double)((useDefault?defaultValues_:values_)[0]);
        }
    
        operator std::string() const {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (std::string)((useDefault?defaultValues_:values_)[0]);
        }

        operator __argparse_string__() const {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (__argparse_string__)((useDefault?defaultValues_:values_)[0]);
        }
    
        operator bool() const {
            if (isNargs_ && (useDefault?defaultValues_:values_).size() > 1) 
            {
                std::cout << name_ << " is a list, use asList() instead of as(), return first value of the list" << std::endl;
            }
            return (bool)((useDefault?defaultValues_:values_)[0]);
        }

        template <class T>
        std::vector<T> asList()
        {
            std::vector<T> rets;
            for (T ret: (useDefault?defaultValues_:values_))
            {
                rets.push_back(ret);
            }
            return rets;
        }

        __argparse_string__ asString()
        {
            if (isNargs_)
            {
                std::ostringstream oss;
                oss << "[";
                for (__argparse_string__ ret: (useDefault?defaultValues_:values_))
                {
                    oss << ret << ",";
                }
                __argparse_string__ ret = oss.str();
                ret[-1] = ']';
                return ret;
            }
            return (useDefault?defaultValues_:values_)[0];
        }

        __argparse_string__ sname_="", name_="", help_="", nargs_="";
        bool isBool_=false;

    private:
        bool required=false;
        bool hasDefault=false;
        bool useDefault=true;
        bool isNargs_=false;
        bool isPosition_=false;
        
        std::vector<__argparse_string__> values_={};

        std::vector<__argparse_string__> defaultValues_={};
    };

    class ArgumentParser
    {
    public:
        ArgumentParser(std::string description="", int argc=0, char** argv=nullptr)
        {
            description_ = description;
            argc_ = argc;
            argv_ = argv;
        }

        // void add_argument(__argparse_string__ name, ACTION_OPT action, __argparse_string__ help="")
        // {
        //     add_argument({name}, action, help);
        // }

        // void add_argument(__argparse_string__ name, __argparse_string__ default_value, __argparse_string__ help="", __argparse_string__ nargs="1")
        // {
        //     add_argument({name}, default_value, help, nargs);
        // }

        // template <class T>
        // void add_argument(__argparse_string__ name, std::vector<T> default_value, __argparse_string__ help="", __argparse_string__ nargs="1")
        // {
        //     add_argument({name}, default_value, help, nargs);
        // }
        
        // bool
        void add_argument(std::vector<__argparse_string__> names, ACTION_OPT action, __argparse_string__ help="")
        {
            if (names.empty() || names.size() > 2)
            {
                std::cerr << "number of argument name should be 1 or 2!" << std::endl;
                exit(-1);
            }

            __argparse_string__ shortName="", longName = names[0];
            if (names.size() == 2)
            {
                shortName = names[0];
                longName = names[1];
            }

            bool isPos = checkName(shortName, longName);

            if (isPos)
            {
                std::cerr << "bool argument should not be a position argument." << std::endl;
                exit(-1);
            }

            __argparse_string__ name = longName.substr(2, longName.length());
            args_.push_back(OneArgument(shortName, longName, (action==ACTION_OPT::ACTION_OPT_TRUE)?"false":"true", help, false, false, "1", true));

            if (!shortName.empty()) 
            {
                snames.push_back(shortName);
                maxSlen = (shortName.length()>maxSlen)?shortName.length():maxSlen;
            }
            maxLlen = (longName.length()>maxLlen)?longName.length():maxLlen;
            this->names.push_back(name);
        }

        void add_argument(std::vector<__argparse_string__> names, __argparse_string__ default_value, __argparse_string__ help="", __argparse_string__ nargs="1")
        {
            if (default_value == "true" || default_value == "false")
            {
                add_argument_required(names, default_value, help, nargs);
                return;
            }
            if (names.empty() || names.size() > 2)
            {
                std::cerr << "number of argument name should be 1 or 2!" << std::endl;
                exit(-1);
            }

            __argparse_string__ shortName="", longName = names[0];
            if (names.size() == 2)
            {
                shortName = names[0];
                longName = names[1];
            }
            bool isPos = checkName(shortName, longName);
            if (isPos)
            {
                std::cerr << "position argument should not set default value." << std::endl;
                exit(-1);
            }
            __argparse_string__ name = isPos?longName:longName.substr(2, longName.length());

            args_.push_back(OneArgument(shortName, longName, default_value, help, false, isPos, nargs));

            if (!shortName.empty()) 
            {
                snames.push_back(shortName);
                maxSlen = (shortName.length()>maxSlen)?shortName.length():maxSlen;
            }
            maxLlen = (longName.length()>maxLlen)?longName.length():maxLlen;
            this->names.push_back(name);
        }
        
        // if no default value, you should std::coutrm whether this argument is required.
        void add_argument_required(std::vector<__argparse_string__> names, bool required, __argparse_string__ help="", __argparse_string__ nargs="1")
        {
            if (names.empty() || names.size() > 2)
            {
                std::cerr << "number of argument name should be 1 or 2!" << std::endl;
                exit(-1);
            }

            __argparse_string__ shortName="", longName = names[0];
            if (names.size() == 2)
            {
                shortName = names[0];
                longName = names[1];
            }
            bool isPos = checkName(shortName, longName);
            
            __argparse_string__ name = isPos?longName:longName.substr(2, longName.length());
            if (isPos)
            {
                numPosArgs++;
                posNames.push_back(name);
            }
            args_.push_back(OneArgument(shortName, longName, "", help, isPos?true:required, isPos, nargs));

            if (!shortName.empty()) 
            {
                snames.push_back(shortName);
                maxSlen = (shortName.length()>maxSlen)?shortName.length():maxSlen;
            }
            maxLlen = (longName.length()>maxLlen)?longName.length():maxLlen;
            this->names.push_back(name);
        }

        template <class T>
        void add_argument(std::vector<__argparse_string__> names, std::vector<T> default_value, __argparse_string__ help="", __argparse_string__ nargs="1")
        {
            if (names.empty() || names.size() > 2)
            {
                std::cerr << "number of argument name should be 1 or 2!" << std::endl;
                exit(-1);
            }

            __argparse_string__ shortName="", longName = names[0];
            if (names.size() == 2)
            {
                shortName = names[0];
                longName = names[1];
            }
            bool isPos = checkName(shortName, longName);
            __argparse_string__ name = isPos?longName:longName.substr(2, longName.length());

            if (isPos)
            {
                std::cerr << "position argument should not set default value." << std::endl;
                exit(-1);
            }

            std::ostringstream oss;
            for (int i=0;i<default_value.size();i++)
            {
                if (i) oss << " ";
                oss << default_value[i];
            }
            args_.push_back(OneArgument(shortName, longName, oss.str(), help, false, isPos, nargs));

            if (!shortName.empty()) 
            {
                snames.push_back(shortName);
                maxSlen = (shortName.length()>maxSlen)?shortName.length():maxSlen;
            }
            maxLlen = (longName.length()>maxLlen)?longName.length():maxLlen;
            this->names.push_back(name);
        }

        ArgumentParser parse_args(bool add_log=false)
        {
            // std::cout << names.size() << ", " << args_.size() << std::endl;
            checkHelp();
            int idx = 1;
            __argparse_string__ currentName = "";
            while (idx < argc_)
            {
                // std::cout << idx << "/" << argc_ << std::endl;
                __argparse_string__ arg = argv_[idx];

                // std::cout << arg << "," << numPosArgs << std::endl;

                if (idx < numPosArgs+1)
                {
                    if (arg.startswith("-"))
                    {
                        std::cerr << "parse std::cerr" << std::endl;
                        showHelpMsg();
                        exit(-1);
                    }
                    
                    // 
                    at(posNames[idx-1]).parseValue(arg);
                }
                else
                {
                    // std::cout << "opt arg" << std::endl;
                    if (!arg.startswith("-"))
                    {
                        std::cerr << "parse std::cerr at '" << arg << "'." << std::endl;
                        showHelpMsg();
                        exit(-1);
                    }

                    if (arg.startswith("--"))
                    {
                        // std::cout << "long name" << std::endl;
                        currentName = arg.substr(2, arg.length());
                        bool existName = false;
                        for (__argparse_string__ name: names)
                        {
                            if (name == currentName)
                            {
                                existName = true;
                                break;
                            }
                        }
                        if (!existName)
                        {
                            std::cerr << "std::cerr: arg '" << arg << "' not exist!" << std::endl;
                            showHelpMsg();
                            exit(-1);
                        }
                    }
                    else
                    {
                        // std::cout << "short name" << std::endl;
                        bool existSName = false;
                        for (__argparse_string__ sname: snames)
                        {
                            if (sname == arg)
                            {
                                existSName = true;
                                break;
                            }
                        }
                        if (!existSName)
                        {
                            std::cerr << "std::cerr: arg '" << arg << "' not exist!" << std::endl;
                            showHelpMsg();
                            exit(-1);
                        }
                        // std::cout << "finding arg std::cout" << std::endl;
                        for (OneArgument nowarg: args_)
                        {
                            if (nowarg.sname_ == arg)
                            {
                                currentName = nowarg.name_;
                                if (!nowarg.isPosArg())
                                {
                                    currentName = currentName.substr(2, currentName.length());
                                }
                                else
                                {
                                    std::cerr << "positional argument should not have short name" << std::endl;
                                    exit(-1);
                                }
                                break;
                            }
                        }
                        // std::cout << "finding arg std::cout end" << std::endl;

                    }

                    if (at(currentName).isBool_)
                    {
                        at(currentName).parseValue(!at(currentName).as<bool>()); // 取反
                    }
                    else
                    {
                        // 如果不为bool，则arg不能是最后一个位置，下一个位置也不能是下一个arg
                        if (idx == argc_)
                        {
                            std::cerr << "arg '" << arg << "' has no value!" << std::endl;
                            exit(-1);
                        }
                        if (__argparse_string__(argv_[idx+1]).startswith("-"))
                        {
                            std::cerr << "arg '" << arg << "' has no value!" << std::endl;
                            exit(-1);
                        }
                        // 直到下一个arg之前的所有值
                        int validNum = at(currentName).numArgs();
                        std::ostringstream oss;
                        int count = 0;
                        while (idx < argc_-1)
                        {
                            idx++;
                            __argparse_string__ nowStr = argv_[idx];
                            if (nowStr.startswith("-"))
                            {
                                idx--;
                                break;
                            }
                            if (count == validNum) continue;
                            if (count) oss << " ";
                            oss << nowStr;
                            count++;
                        }
                        at(currentName).parseValue(oss.str());

                    }
                }
                idx++;
            }

            for(__argparse_string__ name: names)
            {
                if (at(name).isRequiredButNotSet())
                {
                    std::cerr << "arg '" << name << "' is required." << std::endl;
                    showHelpMsg();
                    exit(-1);
                }
            }
            return *this;
        }

        OneArgument& at(__argparse_string__ name){
            if (!pyin(name, names))
            {
                // std::cerr << "no arg named " << name << "!" << std::endl;
                // exit(-1);
                // auto ret = OneArgument();
                return unknown_arg;
            }
            for (int i=0;i<args_.size();i++)
            {
                if(args_[i].isPosArg())
                {
                    if (args_[i].name_ == name)
                    {
                        return args_[i];
                    }
                }
                else
                {
                    if (args_[i].name_.substr(2, args_[i].name_.length()) == name)
                    {
                        return args_[i];
                    }
                }
            }

            return unknown_arg;
        }
        
        OneArgument& operator[](__argparse_string__ name){
            return at(name);
        }
    
    private:
    
        template <class T>
        bool pyin(T element, std::vector<T> list) {
            for (T &e: list) {
	        if (e == element) return true;
            }
            return false;
        }

        void checkHelp()
        {
            int idx = 0;
            while (idx < argc_)
            {
                __argparse_string__ arg = argv_[idx];
                if (arg == "-?" || arg == "--help")
                {
                    showHelpMsg();
                    exit(0);
                }
                else if (arg == "--?")
                {
                    showHelpMsg(true);
                    exit(0);
                }
                
                idx++;
            }
        }    


        bool checkName(__argparse_string__ shortName, __argparse_string__ longName)
        {

            bool svalid = shortName.empty() || (shortName.length()>1 && shortName.startswith("-") && (!shortName.startswith("--")));
            bool lvalid = longName.length()>2 && longName.startswith("--");

            if (svalid && lvalid)
            {
                if (!shortName.empty())
                {
                    for (__argparse_string__ sname: snames)
                    {
                        if (sname == shortName)
                        {
                            std::cerr << "short name '" << sname << "' already exist!" << std::endl;
                            exit(-1);
                        }
                    }
                }
                for (__argparse_string__ name: names)
                {
                    if (longName.substr(2, longName.length()) == name)
                    {
                        std::cerr << "name '" << name << "' already exist!" << std::endl;
                        exit(-1);
                    }
                }

                return false;
            }
            else
            {
                svalid = shortName.empty() || (shortName.length() && !shortName.startswith("-"));
                lvalid = longName.length() && !longName.startswith("-");

                

                if (svalid && lvalid)
                {
                    for (__argparse_string__ name: names)
                    {
                        if (longName == name)
                        {
                            std::cerr << "name '" << name << "' already exist!" << std::endl;
                            exit(-1);
                        }
                    }
                    return true;
                }
                else
                {
                    std::cerr << "short name or long name not valid: [" << shortName << ", " << longName << "]" << std::endl;
                    exit(-1);
                }
            }
            
        }

        void showHelpMsg(bool simple=false)
        {
            std::vector<__argparse_string__> posUsages, optUsages;
            std::vector<__argparse_string__> posHelps, optHelps;

            std::ostringstream oneLineHelp, posOneLine, optOneLine, simpleHelp;
            oneLineHelp << "usage: " << argv_[0] << " [-?]";
            // optOneLine ;
            
            int maxUsageLen=0;
            int count = 0;
            for (auto name: names)
            {
                if (count++)
                {
                    simpleHelp << "\n";
                }
                std::ostringstream thisUsage;
                auto arg = at(name);
                if (arg.isPosArg())
                {
                    posOneLine << " [" << name << "]";
                    thisUsage << "  " << name;
                    simpleHelp << name;

                    posUsages.push_back(thisUsage.str());
                    posHelps.push_back(arg.help_);
                }
                else
                {
                    optOneLine << " [";
                    if (arg.sname_.length())
                    {
                        optOneLine << arg.sname_;
                    }
                    else
                    {
                        optOneLine << arg.name_;
                    }
                    simpleHelp << arg.name_;

                    if(!arg.isBool_)
                    {
                        optOneLine << " [" << name.upper().replace("-", "_");
                        if (arg.numArgs() != 1)
                        {
                            optOneLine << " ...";
                            simpleHelp << " []";
                        }
                        else
                        {
                            simpleHelp << " " << (arg.asString().length()?arg.asString():"\"\"");
                        }
                        optOneLine << "]";
                    }
                    else
                    {
                        simpleHelp << " false";
                    }
                    optOneLine << "]";

                    // 
                    thisUsage << "  ";
                    if (arg.sname_.length())
                    {
                        thisUsage << arg.sname_ << ", ";
                    }
                    thisUsage << arg.name_;
                    if(!arg.isBool_)
                    {
                        thisUsage << " [" << name.upper().replace("-", "_");
                        if (arg.numArgs() != 1)
                        {
                            thisUsage << " ...";
                        }
                        thisUsage << "]";
                    }
                    optUsages.push_back(thisUsage.str());
                    optHelps.push_back(arg.help_);
                }
                
                int thisLength = thisUsage.str().length();
                maxUsageLen = (thisLength>maxUsageLen)?thisLength:maxUsageLen;
            }
            maxUsageLen += 8;

            std::ostringstream multiLineHelp, posMultiLine, optMultiLine;
            optMultiLine << "optional arguments:\n";
            optMultiLine << __argparse_string__("  -?, --help").ljust(maxUsageLen) << "show this help message and exit\n";
            for (int i=0;i<optUsages.size();i++)
            {
                optMultiLine << optUsages[i].ljust(maxUsageLen) << optHelps[i] << "\n";
            }

            posMultiLine << "positional arguments:\n";
            for (int i=0;i<posUsages.size();i++)
            {
                posMultiLine << posUsages[i].ljust(maxUsageLen) << posHelps[i] << "\n";
            }

            oneLineHelp << posOneLine.str() << optOneLine.str();
            if (posUsages.size())
            {
                multiLineHelp << posMultiLine.str() << "\n";
            }
            multiLineHelp << optMultiLine.str();
            
            if (simple) 
            {
                std::cout << simpleHelp.str() << std::endl;
            }
            else
            {
                std::cout << description_ << (description_.empty()?"":"\n") << oneLineHelp.str() << "\n\n" << multiLineHelp.str() << std::endl;
            }
            

            // exit(0);
        }

        int argc_ = 0;
        char** argv_ = nullptr;

        int numPosArgs=0;

        __argparse_string__ description_="";

        std::vector<__argparse_string__> snames, names;
        std::vector<__argparse_string__> posNames;

        std::vector<OneArgument> args_;
        OneArgument unknown_arg;
        int maxSlen=0, maxLlen=0;
        
    };


    
}

#endif
