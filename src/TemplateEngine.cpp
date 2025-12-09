#include "FlaskCpp/TemplateEngine.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>

// Реализация методов класса TemplateEngine

void TemplateEngine::setTemplate(const std::string& name, const std::string& content) {
    templates[name] = content;
}

std::string TemplateEngine::getTemplateContent(const std::string& name) const {
    auto it = templates.find(name);
    if (it != templates.end()) return it->second;
    return "";
}

std::vector<std::string> TemplateEngine::getAllTemplateNames()
{
    std::vector<std::string> names;
    names.reserve(templates.size());
    for (auto t: templates)
    {
        names.push_back(t.first);
    }
    return names;
}

void TemplateEngine::addConfigUpdateListener(const std::string& config_path, FileChangedCallback callback)
{
    namespace fs = std::filesystem;
    config_listeners[config_path] = {
        [callback](const std::string& config_path){callback(config_path);}, 
        fs::last_write_time(config_path)
    };
}

void TemplateEngine::addCheckTask(CheckTask task)
{
    CheckTask t_ = {
        [task](){task();}
    };
    checkTasks.push_back(std::move(t_));
}

void TemplateEngine::checkConfigOnce()
{
    namespace fs = std::filesystem;
    for (auto& cl: config_listeners)
    {
        auto lastUpdateTime = fs::last_write_time(cl.first);
        if (lastUpdateTime != cl.second.update_time)
        {
            cl.second.callback(cl.first);
            config_listeners[cl.first].update_time = lastUpdateTime;
        }
    }
}



std::string TemplateEngine::render(const std::string& templateName, const Context& context) const {
    std::string tpl = getTemplateContent(templateName);
    if (tpl.empty()) {
        return "";
    }
    return renderTemplateContent(tpl, context);
}

// Хэширование контекста
size_t TemplateEngine::hashContext(const Context& context) const {
    size_t seed = 0;
    for (const auto& [key, value] : context) {
        seed ^= std::hash<std::string>()(key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        if (std::holds_alternative<std::string>(value)) {
            seed ^= std::hash<std::string>()(std::get<std::string>(value)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        } else if (std::holds_alternative<bool>(value)) {
            seed ^= std::hash<bool>()(std::get<bool>(value)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        } else if (std::holds_alternative<std::vector<std::map<std::string, std::string>>>(value)) {
            const auto& vec = std::get<std::vector<std::map<std::string, std::string>>>(value);
            for (const auto& mapItem : vec) {
                for (const auto& [k, v] : mapItem) {
                    seed ^= std::hash<std::string>()(k) ^ std::hash<std::string>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
            }
        }
    }
    return seed;
}

std::string TemplateEngine::renderTemplateContent(const std::string& content, const Context& context) const {
    std::string result = content;

    // Обработка extends
    std::map<std::string, std::string> childBlocks;
    result = processExtends(result, context, childBlocks);

    // Обработка комментариев
    result = processComments(result);

    // include
    result = processInclude(result, context);

    // if
    {
        size_t pos = 0;
        while (true) {
            size_t ifPos = result.find("{% if ", pos);
            if (ifPos == std::string::npos) break;
            result = processIf(result, context);
            pos = 0;
        }
    }

    // for
    {
        size_t pos = 0;
        while (true) {
            size_t forPos = result.find("{% for ", pos);
            if (forPos == std::string::npos) break;
            result = processFor(result, context);
            pos = 0;
        }
    }

    // variables
    result = replaceVariables(result, context);

    return result;
}

void TemplateEngine::execAllCheckTask()
{
    for (auto& t: checkTasks)
    {
        t();
    }
}

bool TemplateEngine::evaluateCondition(const std::string& varName, const Context& context) const {
    if (varName[0] == '_' && varName[1] == '_') return false;
    auto it = context.find(varName);
    if (it == context.end()) return false;
    if (std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    } else if (std::holds_alternative<std::string>(it->second)) {
        return !std::get<std::string>(it->second).empty();
    } else if (std::holds_alternative<std::vector<std::map<std::string, std::string>>>(it->second)) {
        return !std::get<std::vector<std::map<std::string, std::string>>>(it->second).empty();
    }
    return false;
}

std::string TemplateEngine::replaceVariables(const std::string& str, const Context& context) const {
    std::string result = str;
    size_t startPos = 0;
    while (true) {
        size_t start = result.find("{{", startPos);
        if (start == std::string::npos) break;
        size_t end = result.find("}}", start);
        if (end == std::string::npos) break;

        std::string varExpr = trim(result.substr(start + 2, end - (start + 2)));
        size_t pipePos = varExpr.find('|');
        std::string varName = varExpr;
        std::string filter;
        if (pipePos != std::string::npos) {
            varName = trim(varExpr.substr(0, pipePos));
            filter = trim(varExpr.substr(pipePos + 1));
        }

        std::string replacement;
        auto it = context.find(varName);
        if (it == context.end() || (varName[0] == '_' && varName[1] == '_')) {
            // Попытка обработать вложенные переменные, например item.field
            size_t dotPos = varName.find('.');
            if (dotPos != std::string::npos) {
                std::string parent = varName.substr(0, dotPos);
                std::string child = varName.substr(dotPos + 1);
                auto parentIt = context.find(parent);
                if (parentIt != context.end() && std::holds_alternative<std::vector<std::map<std::string, std::string>>>(parentIt->second)) {
                    // В данном упрощённом варианте берем первый элемент
                    const auto& vec = std::get<std::vector<std::map<std::string, std::string>>>(parentIt->second);
                    if (!vec.empty()) {
                        auto childIt = vec[0].find(child);
                        if (childIt != vec[0].end()) {
                            replacement = childIt->second;
                        }
                    }
                }
            } else {
                replacement = "";
            }
        } else {
            if (std::holds_alternative<std::string>(it->second)) {
                replacement = std::get<std::string>(it->second);
            } else if (std::holds_alternative<bool>(it->second)) {
                replacement = std::get<bool>(it->second) ? "true" : "false";
            } else if (std::holds_alternative<std::vector<std::map<std::string, std::string>>>(it->second)) {
                replacement = "[object]";
            }
        }

        if (!filter.empty()) {
            replacement = applyFilters(replacement, filter);
        }

        result.replace(start, (end - start) + 2, replacement);
        startPos = start + replacement.size();
    }
    return result;
}

void TemplateEngine::renderLoop(const std::string& loopVar, const std::string& listName, const std::string& innerBlock, const Context& context, std::string& output) const {
    auto it = context.find(listName);
    if (it == context.end()) return;
    if (!std::holds_alternative<std::vector<std::map<std::string, std::string>>>(it->second)) return;

    const auto& vec = std::get<std::vector<std::map<std::string, std::string>>>(it->second);
    for (const auto& item : vec) {
        Context iterationContext = context;
        for (auto &kv : item) {
            iterationContext[loopVar + "." + kv.first] = kv.second;
        }
        output += renderTemplateContent(innerBlock, iterationContext);
    }
}

std::string TemplateEngine::processIf(const std::string& block, const Context& context) const {
    size_t ifPos = block.find("{% if ");
    if (ifPos == std::string::npos) return block;
    size_t ifEnd = block.find("%}", ifPos);
    if (ifEnd == std::string::npos) return block;

    std::string condVar = trim(block.substr(ifPos + 6, ifEnd - (ifPos + 6)));
    bool condition = evaluateCondition(condVar, context);

    size_t endifPos = block.find("{% endif %}", ifEnd);
    if (endifPos == std::string::npos) return block;

    std::string inside = block.substr(ifEnd + 2, endifPos - (ifEnd + 2));
    size_t elsePos = inside.find("{% else %}");

    std::string chosen;
    if (condition) {
        // часть до else или вся строка если нет else
        if (elsePos != std::string::npos) {
            chosen = inside.substr(0, elsePos);
        } else {
            chosen = inside;
        }
    } else {
        // если есть else
        if (elsePos != std::string::npos) {
            size_t elseEnd = elsePos + std::string("{% else %}").size();
            chosen = inside.substr(elseEnd);
        } else {
            chosen = "";
        }
    }

    std::string result = block;
    result.replace(ifPos, (endifPos - ifPos) + 11, chosen);
    return result;
}

std::string TemplateEngine::processFor(const std::string& block, const Context& context) const {
    // Регулярное выражение для поиска блока for
    static const std::regex forRegex(R"(\{% for\s+(\w+)\s+in\s+(\w+)\s+%\})");
    static const std::regex endForRegex(R"(\{% endfor %\})");

    std::smatch forMatch;
    std::smatch endForMatch;

    // Поиск начала блока for
    if (!std::regex_search(block, forMatch, forRegex)) {
        // Если не найдено, возвращаем исходный блок
        return block;
    }

    // Извлечение переменных из выражения for
    std::string varName = forMatch[1];
    std::string listName = forMatch[2];

    // Позиции начала и конца блока for
    size_t forStartPos = forMatch.position(0);
    size_t forEndPos = forMatch.position(0) + forMatch.length(0);

    // Поиск соответствующего {% endfor %}
    std::string remainingBlock = block.substr(forEndPos);
    if (!std::regex_search(remainingBlock, endForMatch, endForRegex)) {
        // Если не найден {% endfor %}, возвращаем исходный блок
        return block;
    }

    size_t endForPos = forEndPos + endForMatch.position(0);
    size_t endForLength = endForMatch.length(0);

    // Извлечение внутреннего содержимого между {% for %} и {% endfor %}
    size_t innerStart = forEndPos;
    size_t innerLength = endForPos - forEndPos;
    std::string innerBlock = block.substr(innerStart, innerLength);

    // Проверка наличия списка в контексте
    auto listIt = context.find(listName);
    if ((listName[0] == '_' && listName[1] == '_') || listIt == context.end() || !std::holds_alternative<std::vector<std::map<std::string, std::string>>>(listIt->second)) {
        // Если список не найден или имеет неверный тип, заменяем блок на пустую строку
        std::string result = block;
        result.erase(forStartPos, (endForPos + endForLength) - forStartPos);
        return result;
    }

    // Получение списка из контекста
    const auto& list = std::get<std::vector<std::map<std::string, std::string>>>(listIt->second);

    // Рендеринг цикла
    std::string renderedLoop;
    renderedLoop.reserve(list.size() * innerBlock.size()); // Предварительное резервирование для повышения производительности

    for (const auto& item : list) {
        Context iterationContext = context; // Копирование текущего контекста
        for (const auto& [key, value] : item) {
            iterationContext[varName + "." + key] = value;
        }
        renderedLoop += renderTemplateContent(innerBlock, iterationContext);
    }

    // Замена исходного блока на отрендеренный цикл
    std::string result = block;
    result.replace(forStartPos, (endForPos + endForLength) - forStartPos, renderedLoop);
    return result;
}

std::string TemplateEngine::processInclude(const std::string& block, const Context& context) const {
    std::string result = block;

    // Регулярное выражение для поиска директив include, например: {% include "header.html" %}
    static const std::regex includeRegex("\\{\\%\\s*include\\s*\"([^\"]+)\"\\s*\\%\\}");
    std::smatch match;
    std::string::const_iterator searchStart(result.cbegin());

    // Используем std::regex_iterator для перебора всех совпадений
    std::regex_iterator<std::string::const_iterator> rit(searchStart, result.cend(), includeRegex);
    std::regex_iterator<std::string::const_iterator> rend;

    // Смещение для корректной замены подстрок
    size_t offset = 0;

    while (rit != rend) {
        match = *rit;
        std::string includeName = match[1];  // Имя шаблона внутри кавычек

        // Вычисляем хэш контекста
        size_t contextHash = hashContext(context);

        // Создаём ключ для кэша
        TemplateEngine::IncludeCacheKey cacheKey{ includeName, contextHash };

        std::string renderedContent;

        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto cacheIt = includeCache.find(cacheKey);
            if (cacheIt != includeCache.end()) {
                // Используем кэшированное значение
                renderedContent = cacheIt->second;
            }
        }

        if (renderedContent.empty()) {
            // Получаем содержимое включаемого шаблона
            std::string incContent = getTemplateContent(includeName);
            if (incContent.empty()) {
                // Заменяем директиву на сообщение об ошибке
                std::string errorMsg = "[Error: Included template not found: " + includeName + "]";
                size_t matchPos = match.position(0) + offset;
                size_t matchLength = match.length(0);
                result.replace(matchPos, matchLength, errorMsg);
                offset += errorMsg.length() - matchLength;

                // Обновляем итератор с учётом изменений в строке
                rit = std::regex_iterator<std::string::const_iterator>(
                    result.cbegin() + matchPos + errorMsg.length(),
                    result.cend(),
                    includeRegex
                );
                continue;
            }

            // Рендерим содержимое включаемого шаблона
            try {
                renderedContent = renderTemplateContent(incContent, context);
            } catch (const std::exception& e) {
                // Заменяем директиву на сообщение об ошибке
                std::string errorMsg = "[Error rendering included template: " + includeName + "]";
                renderedContent = errorMsg;
            }

            // Сохраняем результат в кэш
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                includeCache[cacheKey] = renderedContent;
            }
        }

        // Вычисляем позицию для замены
        size_t matchPos = match.position(0) + offset;
        size_t matchLength = match.length(0);

        // Выполняем замену в результирующей строке
        result.replace(matchPos, matchLength, renderedContent);

        // Обновляем смещение
        offset += renderedContent.length() - matchLength;

        // Обновляем итератор, учитывая изменённую строку
        rit = std::regex_iterator<std::string::const_iterator>(
            result.cbegin() + matchPos + renderedContent.length(),
            result.cend(),
            includeRegex
        );
    }

    return result;
}

std::string TemplateEngine::applyFilters(const std::string& value, const std::string& filter) const {
    if (filter == "upper") {
        std::string up = value;
        std::transform(up.begin(), up.end(), up.begin(), ::toupper);
        return up;
    } else if (filter == "lower") {
        std::string low = value;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        return low;
    } else if (filter == "escape") {
        std::string esc;
        for (auto c : value) {
            if (c == '<') esc += "&lt;";
            else if (c == '>') esc += "&gt;";
            else if (c == '&') esc += "&amp;";
            else if (c == '"') esc += "&quot;";
            else esc += c;
        }
        return esc;
    }
    return value;
}

std::string TemplateEngine::trim(const std::string& s) const {
    std::string r = s;
    r.erase(r.begin(), std::find_if(r.begin(), r.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    r.erase(std::find_if(r.rbegin(), r.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), r.end());
    return r;
}

// Новый метод для извлечения блоков из шаблона
std::map<std::string, std::string> TemplateEngine::extractBlocks(const std::string& content) const {
    std::map<std::string, std::string> blocks;
    std::regex blockRegex(R"(\{% block (\w+) %\}([\s\S]*?)\{% endblock %\})");
    std::smatch match;
    std::string::const_iterator searchStart(content.cbegin());
    while (std::regex_search(searchStart, content.cend(), match, blockRegex)) {
        std::string blockName = match[1];
        std::string blockContent = match[2];
        blocks[blockName] = blockContent;
        searchStart = match.suffix().first;
    }
    return blocks;
}

// Новый метод для обработки директивы extends
std::string TemplateEngine::processExtends(const std::string& content, const Context& context, std::map<std::string, std::string>& childBlocks) const {
    std::regex extendsRegex("\\{% extends\\s+\"([^\"]+)\"\\s*%\\}");
    std::smatch match;
    if (std::regex_search(content, match, extendsRegex)) {
        std::string baseTemplateName = match[1];
        std::string baseContent = getTemplateContent(baseTemplateName);
        if (baseContent.empty()) {
            return "Base template not found: " + baseTemplateName;
        }

        // Извлекаем блоки из базового шаблона
        std::map<std::string, std::string> baseBlocks = extractBlocks(baseContent);

        // Извлекаем блоки из дочернего шаблона
        std::map<std::string, std::string> childExtractedBlocks = extractBlocks(content);
        // Перезаписываем блоки базового шаблона блоками из дочернего шаблона
        for (const auto& [name, blk] : childExtractedBlocks) {
            baseBlocks[name] = blk;
        }

        // Заменяем блоки в базовом шаблоне на соответствующие из дочернего
        std::string renderedBase = baseContent;
        for (const auto& [name, blk] : baseBlocks) {
            std::regex blockPlaceholder(R"(\{% block\s+)" + name + R"(\s*%\}[\s\S]*?\{% endblock %\})");
            renderedBase = std::regex_replace(renderedBase, blockPlaceholder, blk);
        }

        // Рендерим объединённый шаблон
        return renderTemplateContent(renderedBase, context);
    }
    return content;
}

// Реализация метода обработки комментариев
std::string TemplateEngine::processComments(const std::string& content) const {
    std::string result = content;

    // Регулярное выражение для поиска комментариев: {# комментарий #}
    static const std::regex commentRegex("\\{\\#([\\s\\S]*?)\\#\\}");

    // Удаляем все комментарии из шаблона
    result = std::regex_replace(result, commentRegex, "");

    return result;
}
