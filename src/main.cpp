#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/GameObject.hpp>
#include <vector>

using namespace geode::prelude;

class AIPopup : public Popup {
protected:
    TextInput* m_input = nullptr;
    EditorUI* m_editorUI = nullptr;

    bool init(EditorUI* ui) {
        m_editorUI = ui;

        if (!Popup::init(400.f, 220.f)) return false;

        this->setTitle("Groq AI Level Builder");

        m_input = TextInput::create(300.0f, "Masukkan prompt level...");
        m_input->setPosition({0.f, 20.f});
        m_mainLayer->addChild(m_input);

        auto btnSpr = ButtonSprite::create("Generate");
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(AIPopup::onGenerate));
        btn->setPosition({0.f, -60.f});
        m_buttonMenu->addChild(btn);

        return true;
    }

    std::string getCurrentSongInfo() {
        if (!m_editorUI || !m_editorUI->m_editorLayer || !m_editorUI->m_editorLayer->m_level) {
            return "Lagu tidak diketahui";
        }

        auto level = m_editorUI->m_editorLayer->m_level;

        if (level->m_songID > 0) {
            return "Custom Song (Newgrounds ID: " + std::to_string(level->m_songID) + ")";
        }

        int track = level->m_audioTrack;
        static const std::vector<std::string> officialSongs = {
            "Stereo Madness", "Back On Track", "Polargeist", "Dry Out", "Base After Base",
            "Cant Let Go", "Jumper", "Time Machine", "Cycles", "xStep", "Clutterfunk",
            "Theory of Everything", "Electroman Adventures", "Clubstep", "Electrodynamix",
            "Hexagon Force", "Blast Processing", "Theory of Everything 2", "Geometrical Dominator",
            "Deadlocked", "Fingerdash", "Dash", "Explorers"
        };

        if (track >= 0 && track < static_cast<int>(officialSongs.size())) {
            return "Official Song: " + officialSongs[track];
        }

        return "Official Track ID: " + std::to_string(track);
    }

public:
    static AIPopup* create(EditorUI* ui) {
        auto ret = new AIPopup();
        if (ret->init(ui)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void onGenerate(CCObject*) {
        std::string prompt = m_input->getString();
        if (prompt.empty()) return;

        std::string songInfo = getCurrentSongInfo();
        this->onClose(nullptr);

        Notification::create("Menganalisis lagu & membuat level...", NotificationIcon::Loading)->show();

        std::string apiKey = "Gsk_Lfna4DvXz9GFGCZRm46PWGdyb3FYDHxNrXkMnFooCkbnKY3c6wRw";
        std::string url = "https://api.groq.com/openai/v1/chat/completions";

        std::string systemPrompt =
            "Kamu adalah bot pembuat level Geometry Dash. "
            "Lagu saat ini: " + songInfo + ". "
            "User prompt: '" + prompt + "'. "
            "Balas HANYA dengan JSON array: [{\"id\": 1, \"x\": 15.0, \"y\": 15.0}]. Jangan pakai markdown.";

        matjson::Value body = matjson::makeObject({
            {"model", "llama-3.3-70b-versatile"},
            {"temperature", 0.3},
            {"messages", matjson::Value(std::vector<matjson::Value>{
                matjson::makeObject({
                    {"role", "system"},
                    {"content", systemPrompt}
                })
            })}
        });

        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        req.header("Authorization", "Bearer " + apiKey);
        req.bodyJSON(body);

        auto editorUI = m_editorUI;

        async::spawn(
            req.post(url),
            [editorUI](web::WebResponse response) {
                if (!response.ok()) {
                    Notification::create("Gagal terhubung ke Groq", NotificationIcon::Error)->show();
                    return;
                }

                auto res = response.json();
                if (!res) {
                    Notification::create("Response tidak valid", NotificationIcon::Error)->show();
                    return;
                }

                try {
                    auto& json = res.unwrap();
                    std::string aiText = json["choices"][0]["message"]["content"].asString().unwrapOr("");

                    auto levelData = matjson::parse(aiText);
                    if (!levelData) {
                        Notification::create("AI tidak mengembalikan JSON valid", NotificationIcon::Error)->show();
                        return;
                    }

                    int objectCount = 0;
                    auto arr = levelData.unwrap().asArray();
                    if (!arr) return;

                    for (auto const& item : arr.unwrap()) {
                        int id = item["id"].asInt().unwrapOr(1);
                        float x = static_cast<float>(item["x"].asDouble().unwrapOr(0.0));
                        float y = static_cast<float>(item["y"].asDouble().unwrapOr(0.0));

                        if (auto obj = GameObject::createWithKey(id)) {
                            obj->setPosition({x, y});
                            if (editorUI && editorUI->m_editorLayer) {
                                editorUI->m_editorLayer->m_objectLayer->addChild(obj);
                                editorUI->m_editorLayer->m_objects->addObject(obj);
                                objectCount++;
                            }
                        }
                    }

                    Notification::create(std::to_string(objectCount) + " objek dipasang!", NotificationIcon::Success)->show();
                }
                catch (...) {
                    Notification::create("Gagal membaca hasil AI", NotificationIcon::Error)->show();
                }
            }
        );
    }
};

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MyEditorUI::onAIBtn));

        auto menu = CCMenu::create();
        menu->setPosition({25.0f, 100.0f});
        menu->addChild(btn);
        this->addChild(menu);

        return true;
    }

    void onAIBtn(CCObject*) {
        AIPopup::create(this)->show();
    }
};
