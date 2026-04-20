# FlaskCpp: 高效、轻量化的网页服务框架

<div align="center">

[English](README-EN.md)

</div>

<img width="971" height="594" alt="image" src="https://github.com/user-attachments/assets/335b310d-afdb-497b-ba37-dbf5e80c512a" />

## 0. 致谢

感谢 FlaskCpp 原作者[Andrew-Gomonov](https://github.com/Andrew-Gomonov)! 原项目仓库请见 [Andrew-Gomonov/FlaskCpp](https://github.com/Andrew-Gomonov/FlaskCpp).

## 1. 更新内容
- **支持在Windows系统上使用**.
- 更易使用的[新函数接口](/include/FlaskCpp/FlaskCpp.h#L65)  (`void FlaskCpp::route2(std::string, std::function<flaskcpp::Response(const RequestData&)>)`)
- 支持 [文件上传与下载](/include/FlaskCpp/utils/file.h)
- 支持 [带"/"符号的路径参数](/src/FlaskCpp.cpp#L2155)
- 支持 [自定义logger](/include/FlaskCpp/utils/log.h)
- RequestData中加入session参数
- 使用 [nlohmann/json](https://github.com/nlohmann/json) 解析json

## 2. 使用

### 2.1 编译

```shell
mkdir build && cd build
cmake ..
# windows
cmake --build . --config Release
# linux
make -j$(nproc)
# then generate `libFlaskCpp.so` or `FlaskCpp.dll`
```

### 2.2 使用

以下为使用示例

```cpp
// demo.cpp
#include <iostream>
#include <map>
#include <atomic>
#include <FlaskCpp/FlaskCpp.h>

using namespace std;

atomic<bool> globalRunning(true);

void signalHandler(int signum) {
    cout << "\nInterrupt signal (" << signum << ") received.\n";
    globalRunning = false;
}

int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    if (argc < 2)
    {
        cout << "usage: " << argv[0] << " PORT\n";
    }
    int port = std::atoi(argv[1]);

    //       service name, size range of threadPool
    FlaskCpp app(__FILE__, 2, 128);

    app.setSecretKey("thisIsASecretKey!");

    app.loadTemplatesFromDirectory("templates");  // keep the same usage.

    app.route2("/index.html", [&](const RequestData& req) {
        return flaskcpp::send_text("Hello FlaskCpp!");
    });

    string ROOT = "/Path/to/your/file/service";
    app.route2("/files/<path:file_path>", [&](const RequestData& req) {
        string path = ROOT + "/" + req.routeParams.at("file_path");
        if (/* !os.path.isfile(path) */)
        {
            return flaskcpp::send_error(
                "Can not find file " + req.routeParams.at("file_path"), 
                404
            );
        }

        //                  file path, file name, as_attachment
        return flaskcpp::send_file(path, "", true)
    });


    app.route2("/json", [&](const RequestData& req) {
        flaskcpp::JsonGenerator json;
        // key, value
        json.add("a", 1);
        json.add("b", true);
        json.add("c", 3.1415926535);
        json.add("d", "flaskcpp");
        json.add("e", {1, 2, 3, 4, 5});
        json.add("self", json);

        return flaskcpp::send_text(json);
    });

    map<string, string> tokenMap;
    map<string, string> userPwd = {
        {"admin", "password"},
        {"root", "rootPassword"}
    }
    int session_time = 3600 * 24 * 7;  // 7 days
    app.route2("/login", [&](const RequestData& req) {
        if (req.formData.find("user") != req.formData.end() &&
            req.formData.find("password") != req.formData.end() &&)
        {
            if (userPwd[req.formData.at("user")] != req.formData.at("password"))
            {
                return flaskcpp::send_error(
                    "wrong password!", 
                    403
                );
            }
        }
        else
        {
            return flaskcpp::send_error(
                "you should submit both user and password!", 
                403
            );
        }

        map<string, string> session = req.session;
        session["token"] = "ThisIsATokenForExample"; // genereate a safe token when use
        tokenMap[req.formData.at("user")] = session["token"];
        return flaskcpp::send_text(
            // route, show message, delay(second)
            app.jump_to("/index.html", "login success!", 1), 
            {app.generateSession(session, session_time)}
        );
    });

    // port verbose, templateHotReload
    app.runAsync(port, true, true);
    while (globalRunning)
    {
        this_thread::sleep_for(chrono::seconds(1));
    }
    app.stop();

    return 0;
}
```

then compile with FlaskCpp

```cmake
add_executable(demo
    demo.cpp
)

target_link_libraries(demo
    FlaskCpp
)
```
