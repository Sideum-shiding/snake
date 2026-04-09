#include "../include/config.h"
#include "../include/settings.h"

#include <fstream>
#include <sstream>
#include <string>
#include <map>

// Глобальный объект настроек
Settings g_Settings;

// ============================================================================
// Минимальный JSON-сериализатор/десериализатор (без внешних зависимостей)
// ============================================================================

namespace {

// Простая запись значений в JSON-формат
class JsonWriter {
public:
    void BeginObject() { m_ss << "{\n"; m_indent++; }
    void EndObject()   { m_indent--; Indent(); m_ss << "}"; }

    void Key(const std::string& key) {
        if (m_needComma) m_ss << ",\n";
        Indent();
        m_ss << "\"" << key << "\": ";
        m_needComma = true;
    }

    void Value(bool v)              { m_ss << (v ? "true" : "false"); }
    void Value(int v)               { m_ss << v; }
    void Value(float v)             { m_ss << v; }
    void Value(const std::string& v){ m_ss << "\"" << v << "\""; }

    void BeginNested(const std::string& key) {
        Key(key);
        m_ss << "{\n";
        m_indent++;
        m_needComma = false;
    }
    void EndNested() {
        m_ss << "\n";
        m_indent--;
        Indent();
        m_ss << "}";
        m_needComma = true;
    }

    std::string ToString() const { return m_ss.str(); }

private:
    void Indent() { for (int i = 0; i < m_indent; ++i) m_ss << "  "; }
    std::stringstream m_ss;
    int m_indent = 0;
    bool m_needComma = false;
};

// Простой парсер JSON: извлекаем пары ключ-значение в плоскую карту
// Формат ключей: "section.field" (например, "aimbot.enabled")
void ParseJsonFlat(const std::string& json, std::map<std::string, std::string>& out) {
    std::string currentSection;
    std::string buffer = json;

    size_t pos = 0;
    auto skipWs = [&]() {
        while (pos < buffer.size() && (buffer[pos] == ' ' || buffer[pos] == '\t' ||
               buffer[pos] == '\n' || buffer[pos] == '\r' || buffer[pos] == ','))
            pos++;
    };

    auto readString = [&]() -> std::string {
        if (buffer[pos] != '"') return "";
        pos++; // skip opening quote
        std::string result;
        while (pos < buffer.size() && buffer[pos] != '"') {
            result += buffer[pos++];
        }
        pos++; // skip closing quote
        return result;
    };

    auto readValue = [&]() -> std::string {
        skipWs();
        if (pos >= buffer.size()) return "";
        if (buffer[pos] == '"') return readString();
        // Read until comma, newline, or closing brace
        std::string result;
        while (pos < buffer.size() && buffer[pos] != ',' && buffer[pos] != '\n' &&
               buffer[pos] != '}' && buffer[pos] != ' ' && buffer[pos] != '\r') {
            result += buffer[pos++];
        }
        return result;
    };

    while (pos < buffer.size()) {
        skipWs();
        if (pos >= buffer.size()) break;

        if (buffer[pos] == '{') { pos++; continue; }
        if (buffer[pos] == '}') {
            pos++;
            if (!currentSection.empty()) currentSection.clear();
            continue;
        }

        if (buffer[pos] == '"') {
            std::string key = readString();
            skipWs();
            if (pos < buffer.size() && buffer[pos] == ':') pos++; // skip colon
            skipWs();

            if (pos < buffer.size() && buffer[pos] == '{') {
                currentSection = key;
                pos++;
                continue;
            }

            std::string value = readValue();
            std::string fullKey = currentSection.empty() ? key : (currentSection + "." + key);
            out[fullKey] = value;
        } else {
            pos++;
        }
    }
}

// Вспомогательные функции для парсинга значений
bool ToBool(const std::string& v) { return v == "true"; }
int ToInt(const std::string& v) { try { return std::stoi(v); } catch (...) { return 0; } }
float ToFloat(const std::string& v) { try { return std::stof(v); } catch (...) { return 0.0f; } }

} // anonymous namespace

namespace Config {

bool Save(const std::string& path) {
    JsonWriter w;
    w.BeginObject();

    // Aimbot
    w.BeginNested("aimbot");
    w.Key("enabled");      w.Value(g_Settings.aimbot.enabled);
    w.Key("fov");          w.Value(g_Settings.aimbot.fov);
    w.Key("smooth");       w.Value(g_Settings.aimbot.smooth);
    w.Key("visibleOnly");  w.Value(g_Settings.aimbot.visibleOnly);
    w.Key("hitbox");       w.Value(g_Settings.aimbot.hitbox);
    w.EndNested();

    // Triggerbot
    w.BeginNested("triggerbot");
    w.Key("enabled");      w.Value(g_Settings.triggerbot.enabled);
    w.Key("delay_ms");     w.Value(g_Settings.triggerbot.delay_ms);
    w.Key("friendlyFire"); w.Value(g_Settings.triggerbot.friendlyFire);
    w.EndNested();

    // Visuals
    w.BeginNested("visuals");
    w.Key("enabled");      w.Value(g_Settings.visuals.enabled);
    w.Key("box");          w.Value(g_Settings.visuals.box);
    w.Key("healthBar");    w.Value(g_Settings.visuals.healthBar);
    w.Key("name");         w.Value(g_Settings.visuals.name);
    w.Key("snapLines");    w.Value(g_Settings.visuals.snapLines);
    w.Key("boxColorR");    w.Value(g_Settings.visuals.boxColor[0]);
    w.Key("boxColorG");    w.Value(g_Settings.visuals.boxColor[1]);
    w.Key("boxColorB");    w.Value(g_Settings.visuals.boxColor[2]);
    w.Key("boxColorA");    w.Value(g_Settings.visuals.boxColor[3]);
    w.Key("visColorR");    w.Value(g_Settings.visuals.visibleColor[0]);
    w.Key("visColorG");    w.Value(g_Settings.visuals.visibleColor[1]);
    w.Key("visColorB");    w.Value(g_Settings.visuals.visibleColor[2]);
    w.Key("visColorA");    w.Value(g_Settings.visuals.visibleColor[3]);
    w.EndNested();

    // Misc
    w.BeginNested("misc");
    w.Key("bhop");         w.Value(g_Settings.misc.bhop);
    w.Key("thirdPerson");  w.Value(g_Settings.misc.thirdPerson);
    w.Key("aspectRatio");  w.Value(g_Settings.misc.aspectRatio);
    w.EndNested();

    w.EndObject();

    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << w.ToString();
    file.close();
    return true;
}

bool Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    std::map<std::string, std::string> values;
    ParseJsonFlat(ss.str(), values);

    auto get = [&](const std::string& key) -> std::string {
        auto it = values.find(key);
        return (it != values.end()) ? it->second : "";
    };

    // Aimbot
    if (!get("aimbot.enabled").empty())     g_Settings.aimbot.enabled     = ToBool(get("aimbot.enabled"));
    if (!get("aimbot.fov").empty())         g_Settings.aimbot.fov         = ToFloat(get("aimbot.fov"));
    if (!get("aimbot.smooth").empty())      g_Settings.aimbot.smooth      = ToFloat(get("aimbot.smooth"));
    if (!get("aimbot.visibleOnly").empty()) g_Settings.aimbot.visibleOnly = ToBool(get("aimbot.visibleOnly"));
    if (!get("aimbot.hitbox").empty())      g_Settings.aimbot.hitbox      = ToInt(get("aimbot.hitbox"));

    // Triggerbot
    if (!get("triggerbot.enabled").empty())      g_Settings.triggerbot.enabled      = ToBool(get("triggerbot.enabled"));
    if (!get("triggerbot.delay_ms").empty())     g_Settings.triggerbot.delay_ms     = ToInt(get("triggerbot.delay_ms"));
    if (!get("triggerbot.friendlyFire").empty()) g_Settings.triggerbot.friendlyFire = ToBool(get("triggerbot.friendlyFire"));

    // Visuals
    if (!get("visuals.enabled").empty())   g_Settings.visuals.enabled   = ToBool(get("visuals.enabled"));
    if (!get("visuals.box").empty())       g_Settings.visuals.box       = ToBool(get("visuals.box"));
    if (!get("visuals.healthBar").empty()) g_Settings.visuals.healthBar = ToBool(get("visuals.healthBar"));
    if (!get("visuals.name").empty())      g_Settings.visuals.name      = ToBool(get("visuals.name"));
    if (!get("visuals.snapLines").empty()) g_Settings.visuals.snapLines = ToBool(get("visuals.snapLines"));
    if (!get("visuals.boxColorR").empty()) g_Settings.visuals.boxColor[0] = ToFloat(get("visuals.boxColorR"));
    if (!get("visuals.boxColorG").empty()) g_Settings.visuals.boxColor[1] = ToFloat(get("visuals.boxColorG"));
    if (!get("visuals.boxColorB").empty()) g_Settings.visuals.boxColor[2] = ToFloat(get("visuals.boxColorB"));
    if (!get("visuals.boxColorA").empty()) g_Settings.visuals.boxColor[3] = ToFloat(get("visuals.boxColorA"));
    if (!get("visuals.visColorR").empty()) g_Settings.visuals.visibleColor[0] = ToFloat(get("visuals.visColorR"));
    if (!get("visuals.visColorG").empty()) g_Settings.visuals.visibleColor[1] = ToFloat(get("visuals.visColorG"));
    if (!get("visuals.visColorB").empty()) g_Settings.visuals.visibleColor[2] = ToFloat(get("visuals.visColorB"));
    if (!get("visuals.visColorA").empty()) g_Settings.visuals.visibleColor[3] = ToFloat(get("visuals.visColorA"));

    // Misc
    if (!get("misc.bhop").empty())        g_Settings.misc.bhop        = ToBool(get("misc.bhop"));
    if (!get("misc.thirdPerson").empty()) g_Settings.misc.thirdPerson = ToBool(get("misc.thirdPerson"));
    if (!get("misc.aspectRatio").empty()) g_Settings.misc.aspectRatio = ToFloat(get("misc.aspectRatio"));

    return true;
}

} // namespace Config
