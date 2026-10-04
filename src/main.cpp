#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/TextInput.hpp>
#include <vector>

using namespace geode::prelude;

class AIPopup : public geode::Popup<> {
protected:
    TextInput* m_input;
    EditorUI* m_editorUI;

    bool setup() override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        this->setTitle("Groq AI Level Builder");

        m_input = TextInput::create(300.0f, "Contoh: 'buatkan rintangan untuk lagu ini'...");
        m_input->setPosition(winSize / 2);
        m_mainLayer->addChild(m_input);

        auto btnSpr = ButtonSprite::create("Generate");
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(AIPopup::onGenerate));
        btn->setPosition({winSize.width / 2, winSize.height / 2 - 60.0f});

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
        std::vector<std::string> officialSongs = {
            "Stereo Madness", "Back On Track", "Polargeist", "Dry Out", "Base After Base", 
            "Cant Let Go", "Jumper", "Time Machine", "Cycles", "xStep", "Clutterfunk", 
            "Theory of Everything", "Electroman Adventures", "Clubstep", "Electrodynamix", 
            "Hexagon Force", "Blast Processing", "Theory of Everything 2", "Geometrical Dominator", 
            "Deadlocked", "Fingerdash", "Dash", "Explorers"
        };

        if (track >= 0 && track < officialSongs.size()) {
            return "Official Song: " + officialSongs[track];
        }

        return "Official Track ID: " + std::to_string(track);
    }

public:
    static AIPopup* create(EditorUI* ui) {
        auto ret = new AIPopup();
        ret->m_editorUI = ui;
        if (ret && ret->initAnchored(400.f, 220.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
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

        std::string systemPrompt = "Kamu adalah bot pembuat level Geometry Dash. "
                                   "Lagu yang saat ini diputar di level adalah: " + songInfo + ". "
                                   "Berdasarkan prompt user: '" + prompt + "', balas HANYA dengan JSON array berisi objek. "
                                   "Sesuaikan vibe atau gaya rintangan dengan lagu tersebut jika relevan. "
                                   "Jangan tambahkan teks lain atau markdown (```json). "
                                   "Struktur harus: [{\"id\": 1, \"x\": 15.0, \"y\": 15.0}]. Objek GD: 1=balok, 8=duri, 10=portal.";

        auto jsonBody = matjson::makeObject({
            {"model", "llama-3.3-70b-versatile"},
            {"messages", matjson::makeArray({
                matjson::makeObject({
                    {"role", "system"},
                    {"content", systemPrompt}
                })
            })},
            {"temperature", 0.3}
        });

        web::AsyncWebRequest()
            .header("Content-Type", "application/json")
            .header("Authorization", "Bearer " + apiKey)
            .bodyRaw(jsonBody.dump(matjson::NO_INDENTATION))
            .post(url)
            .text()
            .then([this](std::string const& response) {
                try {
                    auto fullResponse = matjson::parse(response);
                    std::string aiText = fullResponse["choices"][0]["message"]["content"].as_string();
                    
                    auto levelData = matjson::parse(aiText);
                    int objectCount = 0;

                    for (auto const& item : levelData.as_array()) {
                        int id = item["id"].as_int();
                        float x = static_cast<float>(item["x"].as_double());
                        float y = static_cast<float>(item["y"].as_double());

                        GameObject* obj = GameObject::createWithKey(id);
                        if (obj) {
                            obj->setPosition({x, y});
                            m_editorUI->m_editorLayer->m_objectLayer->addChild(obj);
                            m_editorUI->m_editorLayer->m_objects->addObject(obj);
                            objectCount++;
                        }
                    }
                    Notification::create(std::to_string(objectCount) + " objek berhasil dipasang!", NotificationIcon::Success)->show();
                } catch (std::exception& e) {
                    Notification::create("Gagal membaca desain dari AI", NotificationIcon::Error)->show();
                }
            })
            .expect([](std::string const& error) {
                Notification::create("Gagal terhubung ke Groq", NotificationIcon::Error)->show();
            });
    }
};

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MyEditorUI::onAIBtn));
        
        auto menu = CCMenu::create();
        menu->setPosition({ 25.0f, 100.0f });
        menu->addChild(btn);
        
        this->addChild(menu);

        return true;
    }

    void onAIBtn(CCObject*) {
        AIPopup::create(this)->show();
    }
};
