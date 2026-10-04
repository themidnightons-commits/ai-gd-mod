#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <random>
#include <algorithm>

using namespace geode::prelude;

namespace {

struct Section { std::string mode = "cube"; float speed = 1.f; float weight = 1.f; int diff = 3; };
struct Spd { int portal; float ups; };

Spd speedInfo(float s) {
    if (s <= .75f) return {200, 251.16f};
    if (s <= 1.5f) return {201, 311.58f};
    if (s <= 2.5f) return {202, 387.42f};
    if (s <= 3.5f) return {203, 468.f};
    return {1334, 576.f};
}
int modePortal(std::string const& m) {
    if (m == "ship") return 13;
    if (m == "ball") return 47;
    if (m == "ufo")  return 111;
    if (m == "wave") return 660;
    return 12;
}

struct Builder {
    LevelEditorLayer* lel;
    std::mt19937 rng;
    int count = 0;

    float rnd() { return std::uniform_real_distribution<float>(0.f, 1.f)(rng); }

    void add(int id, float x, float y, float rot = 0.f) {
        if (auto o = lel->createObject(id, ccp(x, y), true)) {
            if (rot != 0.f) o->setRotation(rot);
            count++;
        }
    }

    void cube(float& x, float end, int d, float f) {
        while (x < end) {
            float k = rnd();
            if (k < .4f) add(8, x, 15);
            else if (k < .65f) { add(8, x, 15); add(8, x + 30, 15); }
            else if (k < .8f) { add(1, x, 15); add(1, x + 30, 15); add(8, x + 120, 15); }
            else if (d >= 3) { add(35, x, 6); add(8, x + 60, 15); add(8, x + 90, 15); add(8, x + 120, 15); }
            else add(8, x, 15);
            x += 150 + (9 - d + rnd() * 3) * 30 * f;
        }
    }

    void ball(float& x, float end, int d, float f) {
        float nx = x;
        for (; x < end; x += 30) {
            add(1, x, 165);
            if (x >= nx) {
                if (rnd() < .5f) add(8, x, 15); else add(8, x, 135, 180.f);
                nx = x + (9 - d + rnd() * 3) * 30 * f;
            }
        }
    }

    void corridor(std::string const& m, float& x, float end, int d) {
        int fl = 0, gb = 8 - d;
        float ch = m == "wave" ? .1f : .14f;
        for (; x < end; x += 30) {
            if (rnd() < ch) fl = std::clamp(fl + (rnd() < .5f ? -1 : 1), 0, 4);
            for (int j = 0; j <= fl; j++) add(1, x, 15 + j * 30);
            for (int j = fl + gb; j <= 10; j++) add(1, x, 15 + j * 30);
            if (m == "ufo" && rnd() < .05f * d) add(8, x, 15 + (fl + 1) * 30);
        }
    }

    void run(std::vector<Section> const& secs, float totalSec) {
        float wsum = 0.f;
        for (auto& s : secs) wsum += s.weight;
        if (wsum <= 0.f) wsum = 1.f;
        float x = 300.f;
        for (auto& s : secs) {
            auto sp = speedInfo(s.speed);
            float f = sp.ups / 311.58f;
            float end = x + totalSec * (s.weight / wsum) * sp.ups;
            int d = std::clamp(s.diff, 1, 5);
            add(sp.portal, x, 105);
            add(modePortal(s.mode), x + 30, s.mode == "cube" ? 45 : 105);
            x += 180;
            if (s.mode == "cube") cube(x, end, d, f);
            else if (s.mode == "ball") ball(x, end, d, f);
            else corridor(s.mode, x, end, d);
        }
    }
};

constexpr auto SYSTEM_PROMPT = R"(You design the structure of Geometry Dash levels. Reply with ONLY valid JSON, no prose and no markdown, in this exact shape: {"sections":[{"mode":"cube|ship|ball|ufo|wave","speed":0.5|1|2|3|4,"weight":relative length as a number,"diff":1-5}]}. Use 2 to 8 sections with a progression that fits the user's request, mood and song.)";

class AIPopup : public Popup {
protected:
    LevelEditorLayer* m_lel = nullptr;
    TextInput* m_prompt = nullptr;
    TextInput* m_secs = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCMenuItemSpriteExtra* m_btn = nullptr;
    async::TaskHolder<web::WebResponse> m_task;
    float m_total = 60.f;
    std::string m_seed;

    bool init(LevelEditorLayer* lel) {
        if (!Popup::init(340.f, 220.f)) return false;
        m_lel = lel;
        this->setTitle("Groq AI Level Builder");

        m_prompt = TextInput::create(300.f, "Masukkan prompt level...", "chatFont.fnt");
        m_prompt->setMaxCharCount(400);
        m_mainLayer->addChildAtPosition(m_prompt, Anchor::Center, {0, 40});

        m_secs = TextInput::create(160.f, "Detik (default 60)", "chatFont.fnt");
        m_secs->setFilter("0123456789");
        m_mainLayer->addChildAtPosition(m_secs, Anchor::Center, {0, -5});

        m_status = CCLabelBMFont::create(songInfo().c_str(), "chatFont.fnt");
        m_status->setScale(.7f);
        m_mainLayer->addChildAtPosition(m_status, Anchor::Center, {0, -40});

        m_btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Generate"), this, menu_selector(AIPopup::onGo));
        m_buttonMenu->addChildAtPosition(m_btn, Anchor::Bottom, {0, 30});
        return true;
    }

    std::string songInfo() {
        if (!m_lel || !m_lel->m_level) return "Lagu tidak diketahui";
        auto lvl = m_lel->m_level;
        if (lvl->m_songID > 0) return fmt::format("Custom song (NG ID {})", lvl->m_songID);
        static const std::vector<std::string> names = {
            "Stereo Madness", "Back On Track", "Polargeist", "Dry Out", "Base After Base",
            "Cant Let Go", "Jumper", "Time Machine", "Cycles", "xStep", "Clutterfunk",
            "Theory of Everything", "Electroman Adventures", "Clubstep", "Electrodynamix",
            "Hexagon Force", "Blast Processing", "Theory of Everything 2", "Geometrical Dominator",
            "Deadlocked", "Fingerdash", "Dash", "Explorers"
        };
        int t = lvl->m_audioTrack;
        if (t >= 0 && t < (int)names.size()) return fmt::format("Official: {}", names[t]);
        return "Lagu tidak diketahui";
    }

    void setStatus(std::string const& s) { m_status->setString(s.c_str()); }

    void onGo(CCObject*) {
        auto key = Mod::get()->getSettingValue<std::string>("api-key");
        if (key.empty()) {
            FLAlertLayer::create("API key kosong", "Isi Groq API key di Settings mod ini (Geode > mod > Settings).", "OK")->show();
            return;
        }
        auto prompt = m_prompt->getString();
        if (prompt.empty()) { setStatus("Isi prompt dulu"); return; }

        auto secs = m_secs->getString();
        m_total = secs.empty() ? 60.f : std::clamp<float>((float)std::atof(secs.c_str()), 10.f, 300.f);
        m_seed = prompt;

        auto userMsg = fmt::format("Level idea: {}\nSong: {}\nTarget length: {} seconds",
            prompt, songInfo(), (int)m_total);
        auto sys = matjson::makeObject({{"role", "system"}, {"content", std::string(SYSTEM_PROMPT)}});
        auto usr = matjson::makeObject({{"role", "user"}, {"content", userMsg}});
        auto body = matjson::makeObject({
            {"model", Mod::get()->getSettingValue<std::string>("model")},
            {"messages", std::vector<matjson::Value>{sys, usr}},
            {"temperature", 0.5},
            {"response_format", matjson::makeObject({{"type", "json_object"}})}
        });

        web::WebRequest req;
        req.header("Authorization", fmt::format("Bearer {}", key));
        req.header("Content-Type", "application/json");
        req.timeout(std::chrono::seconds(60));
        req.bodyJSON(body);

        m_btn->setEnabled(false);
        setStatus("Menghubungi Groq...");
        m_task.spawn(
            req.post("https://api.groq.com/openai/v1/chat/completions"),
            [this](web::WebResponse res) { this->onResponse(std::move(res)); }
        );
    }

    void onResponse(web::WebResponse res) {
        m_btn->setEnabled(true);
        if (!res.ok()) { setStatus(fmt::format("Gagal: HTTP {}", res.code())); return; }

        auto json = res.json();
        if (json.isErr()) { setStatus("Response tidak valid"); return; }
        auto root = json.unwrap();
        auto content = root["choices"][0]["message"]["content"].asString().unwrapOr("");

        auto a = content.find('{');
        auto b = content.rfind('}');
        if (a == std::string::npos || b == std::string::npos || b < a) { setStatus("AI tidak membalas JSON"); return; }
        auto plan = matjson::parse(content.substr(a, b - a + 1));
        if (plan.isErr()) { setStatus("JSON dari AI rusak"); return; }
        auto pj = plan.unwrap();

        std::vector<Section> secs;
        auto arr = pj["sections"].asArray();
        if (arr.isOk()) {
            auto v = arr.unwrap();
            for (auto& s : v) {
                Section sec;
                sec.mode = s["mode"].asString().unwrapOr("cube");
                sec.speed = (float)s["speed"].asDouble().unwrapOr(1.0);
                sec.weight = (float)s["weight"].asDouble().unwrapOr(1.0);
                sec.diff = (int)s["diff"].asInt().unwrapOr(3);
                secs.push_back(sec);
                if (secs.size() >= 8) break;
            }
        }
        if (secs.empty()) { setStatus("Rencana AI kosong"); return; }

        Builder bld{m_lel, std::mt19937((unsigned)std::hash<std::string>{}(m_seed))};
        bld.run(secs, m_total);
        setStatus(fmt::format("{} objek dipasang ({} section)", bld.count, secs.size()));
    }

public:
    static AIPopup* create(LevelEditorLayer* lel) {
        auto ret = new AIPopup();
        if (ret->init(lel)) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

} // namespace

class $modify(AIEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("AI"), this, menu_selector(AIEditorUI::onAI));
        auto win = CCDirector::get()->getWinSize();
        btn->setPosition({45.f, win.height - 70.f});
        menu->addChild(btn);
        this->addChild(menu, 100);
        return true;
    }

    void onAI(CCObject*) {
        if (auto p = AIPopup::create(m_editorLayer)) p->show();
    }
};
