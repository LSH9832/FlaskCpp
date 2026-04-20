#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <map>
#include <cstring>
#include <iostream>

// #include <jwt/jwt.hpp>

class URLSafeSerializer {
private:
    std::string secret_key;
    
    // Base64 URL安全字符表
    const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-_";
    
    // HMAC-SHA1实现
    std::string hmac_sha1(const std::string& data) const {
        // 块大小（字节）
        const size_t block_size = 64;
        
        // 密钥预处理
        std::string key = secret_key;
        if (key.length() > block_size) {
            // 如果密钥太长，先进行哈希（这里简化处理）
            key = key.substr(0, block_size);
        }
        if (key.length() < block_size) {
            key.append(block_size - key.length(), '\0');
        }
        
        // 创建ipad和opad
        std::string ipad(block_size, 0x36);
        std::string opad(block_size, 0x5C);
        
        // 密钥与ipad/opad异或
        std::string i_key_pad, o_key_pad;
        for (size_t i = 0; i < block_size; ++i) {
            i_key_pad.push_back(key[i] ^ ipad[i]);
            o_key_pad.push_back(key[i] ^ opad[i]);
        }
        
        // 内部哈希：hash(i_key_pad + message)
        std::string inner_hash_data = i_key_pad + data;
        std::string inner_hash = sha1_hash(inner_hash_data);
        
        // 外部哈希：hash(o_key_pad + inner_hash)
        std::string outer_hash_data = o_key_pad + inner_hash;
        std::string outer_hash = sha1_hash(outer_hash_data);
        
        return outer_hash;
    }
    
    // SHA1哈希函数简化实现
    std::string sha1_hash(const std::string& data) const {
        // 初始化哈希值
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        
        
        // 预处理：填充数据
        std::string padded_data = data;
        padded_data.push_back(static_cast<char>(0x80));

        
        
        // 填充0直到长度 ≡ 448 (mod 512)
        while ((padded_data.size() * 8) % 512 != 448) {
            padded_data.push_back('\0');
        }

        
        
        // 附加原始数据长度（64位）
        uint64_t original_bit_length = data.size() * 8;
        for (int i = 7; i >= 0; --i) {
            padded_data.push_back(static_cast<char>((original_bit_length >> (8 * i)) & 0xFF));
        }

        
        
        // 处理512位块
        for (size_t i = 0; i < padded_data.size(); i += 64) {
            std::vector<uint32_t> w(80, 0);
            
            // 将块分解为16个32位字
            for (size_t j = 0; j < 16; ++j) {
                w[j] = (static_cast<uint32_t>(padded_data[i + j * 4]) << 24) |
                         (static_cast<uint32_t>(padded_data[i + j * 4 + 1]) << 16) |
                         (static_cast<uint32_t>(padded_data[i + j * 4 + 2]) << 8) |
                         (static_cast<uint32_t>(padded_data[i + j * 4 + 3]));
            }
            
            // 扩展16个字为80个字
            for (size_t j = 16; j < 80; ++j) {
                w[j] = left_rotate(w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16], 1);
            }
            
            // 初始化工作变量
            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
            
            // 主循环
            for (size_t j = 0; j < 80; ++j) {
                uint32_t f, k;
                
                if (j < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (j < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (j < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                
                uint32_t temp = left_rotate(a, 5) + f + e + k + w[j];
                e = d;
                d = c;
                c = left_rotate(b, 30);
                b = a;
                a = temp;
            }
            
            // 添加到当前哈希值
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }
        
        

        // 转换为字节字符串
        std::string result;
        for (auto val : {h0, h1, h2, h3, h4}) {
            for (int i = 3; i >= 0; --i) {
                result.push_back(static_cast<char>((val >> (8 * i)) & 0xFF));
            }
        }

        
            
        return result;
    }
    
    // 左旋转函数
    uint32_t left_rotate(uint32_t value, size_t count) const {
        return (value << count) | (value >> (32 - count));
    }

    std::string base64_encode(const std::string& data) {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (data.size() - i >= 3) {
            char_array_3[0] = data[i + 0];
            char_array_3[1] = data[i + 1];
            char_array_3[2] = data[i + 2];

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; j < 4; j++) 
                ret += base64_chars[char_array_4[j]];
            i += 3;
        }

        if (data.size() - i) {
            int count = 0;
            for (j = 0; j < 3; j++)
            {
                char_array_3[j] = (j < data.size() - i)?data[i + j]:'\0';
                // if (j < data.size() - i) std::cout << data[i+j] << ": value: " << (int)data[i + j] << std::endl;
            }

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; j < 4; j++) 
                ret += base64_chars[char_array_4[j]];
            
            // std::cout << (data.size() - i) + 1 << std::endl;
            // for (j = 0; (j < (int)(data.size() - i) + 1); j++) 
            //     ret += base64_chars[char_array_4[j]];

            // std::cout << data.size() << ", " << i << std::endl;
            // while ((data.size() - ret.size()) < 3) 
            //     ret += '=';
            // ret += "\0";
            // ret += '=';
        }
        // std::cout << "encode_size: " << data.size() << std::endl;
        return ret;
    }
    // Base64 URL安全解码
    std::string base64_decode(const std::string& encoded) {
        std::string ret;
        int i = 0;
        int in_len = encoded.size();
        int i_end = in_len - 4;
        unsigned char char_array_4[4], char_array_3[3];

        while (i <= i_end) {
            for (int j = 0; j < 4; ++j) 
                char_array_4[j] = base64_chars.find(encoded[i++]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (int j = 0; j < 3; ++j) 
                if (char_array_3[j] != 0)
                    ret += char_array_3[j];
        }

        if (i < in_len) {
            for (int j = i; j < in_len; ++j) 
                char_array_4[j - i] = base64_chars.find(encoded[j]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            
            std::cout << in_len - i << std::endl;

            if (i == (in_len - 1)) 
            {
                if (char_array_3[0] != 0)
                    ret += char_array_3[0];
            }
            else
            {
                if (char_array_3[0] != 0)
                    ret += char_array_3[0];

                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                
                if (char_array_3[1] != 0)
                    ret += char_array_3[1];
                if (i == (in_len - 3))
                {
                    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                    if (char_array_3[2] != 0)
                        ret += char_array_3[2];
                }
                
            }
            
            // if (char_array_3[2] != 0)
        }

        // std::cout << "decode size: " << ret.size() << std::endl;

        return ret;
    }
public:
    URLSafeSerializer() {}

    URLSafeSerializer(const std::string& key) : secret_key(key) {}

    void setKey(const std::string& key)
    {
        secret_key = key;
    }

    std::string map2str(const std::map<std::string, std::string>& map)
    {
        std::ostringstream oss;
        int count = 0;
        for (auto& it: map)
        {
            if (count++) oss << ",";
            oss << it.first << "=" <<it.second;
        }
        return oss.str();
    }

    void str2map(const std::string& data, std::map<std::string, std::string>& map)
    {
        std::string str = data;
        std::vector<std::string> pairs;
        std::string delimiter = ",";
        size_t pos = 0;
        std::string token;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            token = str.substr(0, pos);
            pairs.push_back(token);
            str.erase(0, pos + delimiter.length());
        }
        pairs.push_back(str); // Add the last pair

        for (const auto& pair : pairs) {
            size_t equalPos = pair.find("=");
            if (equalPos != std::string::npos) {
                std::string key = pair.substr(0, equalPos);
                std::string value = pair.substr(equalPos + 1);
                map[key] = value;
            }
        }
    }

    
    // 序列化并签名数据
    std::string dumps(const std::string& data) {
        // 生成签名
        std::string signature = hmac_sha1(data);
        
        // 组合数据和签名
        std::ostringstream oss;
        oss << base64_encode(data) << "." << base64_encode(signature);

        // std::ostringstream oss_s;
        // std::cout << data << ", " << data.size() << std::endl;

        // for(auto&c: data)
        // {
        //     oss_s << (int)c << " ";
        // }
        // std::cout << oss_s.str() << std::endl;

        // std::cout << "signature: " << signature << std::endl;
        
        return oss.str();
    }
    
    // 验证签名并反序列化数据
    std::string loads(const std::string& signed_data) {
        // 分离数据和签名
        size_t dot_pos = signed_data.find_last_of('.');
        if (dot_pos == std::string::npos) {
            // std::cout << payload << std::endl;
            return "";
        }

        std::string data_part = base64_decode(signed_data.substr(0, dot_pos));
        std::string signature_part = base64_decode(signed_data.substr(dot_pos + 1));


        
        // 验证签名
        std::string expected_signature = hmac_sha1(data_part);
        
        // std::cout << "验证签名" << std::endl;
        // std::ostringstream oss_s;
        // for(auto&c: data_part)
        // {
        //     oss_s << (int)c << " ";
        // }
        // std::cout << oss_s.str() << std::endl;
        // std::cout << data_part << ", " << data_part.size() << std::endl;
        // std::cout << signature_part << std::endl;
        // std::cout << expected_signature << std::endl;
        if (signature_part != expected_signature) {
            // std::cerr << "signature not match!" << std::endl;
            // std::ostringstream oss1, oss2;
            // for (auto&c:signature_part) oss1 << (int)c << " ";
            // for (auto&c:expected_signature) oss2 << (int)c << " ";
            // std::cout << oss1.str() << std::endl;
            // std::cout << oss2.str() << std::endl;
            return "";
        }
        
        // 反序列化数据
        return data_part;
    }
};

// std::string main() {
//     URLSafeSerializer serializer("secret-key");
//     std::string signed_data = serializer.dumps("123");
//     std::string result = "序列化结果: " + signed_data;
    
//     try {
//         std::string deserialized_data = serializer.loads(signed_data);
//         result += ", 反序列化结果: " + deserialized_data;
//     } catch (const std::exception& e) {
//         result += ", 错误: " + std::string(e.what());
//     }
    
//     return result;
// }
