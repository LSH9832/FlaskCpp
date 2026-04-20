#ifndef FLASKCPP_CLIENTDATA_H
#define FLASKCPP_CLIENTDATA_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "utils/str_common.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/ioctl.h>
#include <bits/stl_algo.h>
#include <sys/socket.h>
#endif

#ifndef MIN
#define MIN_DEF_TEMP
#define MIN( a , b ) (((a) < (b) )? (a) : (b))
#endif

class ClientData
{
public:
    ClientData(int clientSocket)
    : clientSocket(clientSocket)
    {

    }

    inline int getClientSocket()
    {
        return clientSocket;
    }

    inline std::string readHeader()
    {
        
        if (!header.empty()) return header;
        receiveHeader();

        // std::cout << __LINE__ << std::endl;
        // std::cout << header << std::endl;

        size_t bodyPos = header.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string headerPart = header.substr(0, bodyPos);
            std::istringstream iss(headerPart);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (line.find("Content-Length:") != std::string::npos) {
                    std::string lenStr = line.substr(line.find(":")+1);
                    lenStr.erase(0,lenStr.find_first_not_of(' '));
                    contentLength = std::stoi(lenStr);
                }
                if (line.find("Content-Type:") != std::string::npos) {
                    std::string lenStr = line.substr(line.find(":")+1);
                    lenStr.erase(0,lenStr.find_first_not_of(' '));
                    for(auto&line: stringSplit(lenStr, "; "))
                    {
                        if (stringStartsWith(line, "boundary="))
                        {
                            file_head = line.substr(9);
                        }
                    }
                }
            }
        }

        return header;

        // // 读取一点body获取信息
        // char bodyBuffer[512];
        

        // int bytesAvailable = 0;
        // int result = ioctl(clientSocket, FIONREAD, &bytesAvailable);
        
        // if (result < 0 || bytesAvailable < 1) {
        //     return header; // ioctl失败 || 无数据可读
        // }
        // std::string bodyStart;
        // ssize_t bytesRead = recv(clientSocket, bodyBuffer, MIN(sizeof(bodyBuffer), contentLength), MSG_PEEK);

        // if (bytesRead > 0)
        // {
        //     // std::cout << __LINE__ << ":" << bytesRead << std::endl;
        //     bodyStart.append(bodyBuffer, bytesRead);
        //     // std::cout << bodyStart << std::endl;
        //     // std::cout << __LINE__ << std::endl;
        //     if (file_head.length())
        //     {
        //         size_t loc = bodyStart.find(file_head);

        //         if (loc == bodyStart.npos) return header;
                
        //         size_t line_loc = bodyStart.substr(loc).find("\n");
        //         file_head = bodyStart.substr(loc, line_loc-1);
        //         std::string content = bodyStart.substr(loc+line_loc+1);
        //         size_t disp_loc = content.find("Content-Disposition: ");

        //         if (disp_loc == content.npos) return header;

        //         size_t disp_loc_end = content.find("\r\n");
        //         if (disp_loc_end == content.npos) return header;

        //         for(auto& p: stringSplit(content.substr(disp_loc + 21, disp_loc_end - disp_loc - 21), "; "))
        //         {
        //             // std::cout << p << std::endl;
        //             if (stringStartsWith(p, "name=\""))
        //             {
        //                 file_name = p.substr(6, p.size()-7);
        //                 // if (file_name != "file") return header;
        //             }
        //             else if (stringStartsWith(p, "filename=\""))
        //             {
        //                 file_filename = p.substr(10, p.size()-11);
        //             }
        //         }

        //         size_t type_loc = content.find("Content-Type: ", disp_loc_end);
        //         if (type_loc != content.npos) 
        //         {
        //             size_t type_loc_end = content.find("\r\n", type_loc);
        //             file_type = content.substr(type_loc + 14, type_loc_end - type_loc - 14);
        //         }
        //     }
        // }
        // return header;
    }

    inline void provideFileMsg(std::string& name, std::string& filename, std::string& filetype)
    {
        name = file_name;
        filename = file_filename;
        filetype = file_type;
    }

    inline bool isFormData()
    {
        return !file_head.empty();
    }

    inline void readBody(std::vector<char>& body)
    {
        if (contentLength > 0) {
            body.resize(contentLength);
            int totalRead = 0;
            while (totalRead < contentLength) {
                ssize_t r = recv(clientSocket, body.data() + totalRead, contentLength - totalRead, 0);
                if (r <= 0) break;
                totalRead += r;
            }
        }
    }

    inline std::vector<char> readBody()
    {
        std::vector<char> body;
        if (contentLength > 0) {
            body.resize(contentLength);
            int totalRead = 0;
            while (totalRead < contentLength) {
                ssize_t r = recv(clientSocket, body.data() + totalRead, contentLength - totalRead, 0);
                if (r <= 0) break;
                totalRead += r;
            }
        }
        return body;
    }

    inline void readBodyString(std::string& stringData, bool clear=false)
    {
        if (contentLength > 0) {
            if (clear) stringData.clear();
            std::string body;
            body.resize(contentLength);
            int totalRead = 0;
            while (totalRead < contentLength) {
                ssize_t r = recv(clientSocket, &body[totalRead], contentLength - totalRead, 0);
                if (r <= 0) break;
                totalRead += r;
            }
            // std::cout << __LINE__ << std::endl;
            stringData.append(body);
            // std::cout << __LINE__ << std::endl;
        }
    }

    inline std::string readBodyString()
    {
        std::string body;
        if (contentLength > 0) {
            body.resize(contentLength);
            int totalRead = 0;
            while (totalRead < contentLength) {
                ssize_t r = recv(clientSocket, &body[totalRead], contentLength - totalRead, 0);
                if (r <= 0) break;
                totalRead += r;
            }
        }
        return body;
    }

    void readClear()
    {
        char tmp[512];
        recv(clientSocket, tmp, MIN(sizeof(tmp), contentLength), 0);
    }

private:
    int clientSocket;
    int contentLength = 0;
    std::string header, file_head, file_name, file_filename, file_type;


    inline void receiveHeader()
    {
        if (contentLength) return;


        // header.clear();
        // char buffer[1];
        // ssize_t bytesRead;

        // int count = 4;
        // static const std::string end = "\r\n\r\n";
        // while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        //     header.append(buffer, bytesRead);
        //     if (end[4-count] == buffer[0]) count--;
        //     else count = 4;
        //     if (count) continue;
        //     break;
        // }


        header.clear();
        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), MSG_PEEK)) > 0) {
            std::string temp(buffer, bytesRead);
            size_t pos = temp.find("\r\n\r\n");
            if (pos != std::string::npos) {
                recv(clientSocket, buffer, pos+4, 0);
                header.append(buffer, pos+4);
                // hasContent = true;
                break;
            } else {
                recv(clientSocket, buffer, bytesRead, 0);
                header.append(buffer, bytesRead);
            }
        }
        // std::cout << header << std::endl;
    }

};

#ifdef MIN_DEF_TEMP
#undef MIN
#undef MIN_DEF_TEMP
#endif

#endif