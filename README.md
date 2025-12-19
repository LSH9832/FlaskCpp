# FlaskCpp: A Efficient & Extreme-Lightweight C++ Web Framework

## 0. Thanks

Thanks to original author of FlaskCpp [Andrew-Gomonov](https://github.com/Andrew-Gomonov)! The origin source code of this repository is [Andrew-Gomonov/FlaskCpp](https://github.com/Andrew-Gomonov/FlaskCpp), and I make it easier to use.

## 1. What's New?
- [new function](/include/FlaskCpp/FlaskCpp.h#L57) for easier use (`void FlaskCpp::route2(std::string, std::function<flaskcpp::Response(const RequestData&)>)`)
- support file [upload and download](/include/FlaskCpp/utils/file.h)
- support [routeParam with "/"](/src/FlaskCpp.cpp#L1498)
- support custom [simple log system](/include/FlaskCpp/utils/log.h)
- add session in RequestData
- auto parse json by using [nlohmann/json](https://github.com/nlohmann/json)

## 2. Usage

### 2.1 Compile

```shell
mkdir build && cd build
cmake ..
make -j$(nproc)
# then generate libFlaskCpp.so
```

### 2.2 Use FlaskCpp in your project!

Here's a demo

```cpp
// demo.cpp
#include <iostream>
#include <map>
#include <atomic>
#include <FlaskCpp/FlaskCpp.h>

using namespace std;

atomic<bool> globalRunning(true);

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    globalRunning = false;
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    if (argc < 2)
    {
        cout << "usage: " << argv[0] << " PORT" << endl;
    }
    int port = std::atoi(argv[1]);

    //       service name, size range of threadPool
    FlaskCpp app(__FILE__, 2, 128);

    app.setSecretKey("thisIsASecretKey!");

    app.loadTemplatesFromDirectory("templates");  // keep the same usage.

    app.route2("/index.html", [&](const RequestData& req) {
        return flaskcpp::send_text("Hello FlaskCpp!");
    });

    std::string ROOT = "/Path/to/your/file/service";
    app.route2("/files/<path:file_path>", [&](const RequestData& req) {
        std::string path = ROOT + "/" + req.routeParams.at("file_path");
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
