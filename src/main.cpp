#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/Slider.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/loader/SettingV3.hpp>


struct {
	struct {
		bool m_isEnabled;
		IconType m_iconType;
		bool m_affectEditorSlider;
		bool m_affectTriggerSliders;
		bool m_forceGlow;
		short m_updateId = 1; // needed for slider preview in settings
		void update() {
			m_isEnabled = Mod::get()->getSettingValue<bool>("is-enabled");
			m_forceGlow = Mod::get()->getSettingValue<bool>("force-glow");
			m_affectEditorSlider = Mod::get()->getSettingValue<bool>("affect-editor-slider");
			m_affectTriggerSliders = Mod::get()->getSettingValue<bool>("affect-trigger-sliders");

			int iconType = std::atoi(Mod::get()->getSettingValue<std::string>("icon").c_str());
			iconType = (iconType <= 0 || iconType >= 10) ? 1 : iconType; // validate
			m_iconType = static_cast<IconType>(iconType - 1); // 0 <= IconType <= 8

			m_updateId++;
		}
	} m_settings; 

	bool m_isInEditor = false;
} GLOBAL;


#include "settings.cpp"


$on_mod(Loaded) {
	GLOBAL.m_settings.update();
	// listen for settings update
	listenForAllSettingChangesV3([](std::shared_ptr<SettingV3> idk) {
		GLOBAL.m_settings.update();
	},
	Mod::get());
}


int getPlayerIconIndex(IconType type) {
    auto gm = GameManager::sharedState();
    switch(type) {
        case IconType::Cube: return gm->getPlayerFrame();
        case IconType::Ship: return gm->getPlayerShip();
        case IconType::Ball: return gm->getPlayerBall();
        case IconType::Ufo: return gm->getPlayerBird();
        case IconType::Wave: return gm->getPlayerDart();
        case IconType::Robot: return gm->getPlayerRobot();
        case IconType::Spider: return gm->getPlayerSpider();
		case IconType::Swing: return gm->getPlayerSwing(); 
		case IconType::Jetpack: return gm->getPlayerJetpack();
        default: return gm->getPlayerFrame();
    }
}

SimplePlayer* getPlayerFrame(IconType type, bool forceGlow) {
    auto gm = GameManager::sharedState();
	SimplePlayer* player = SimplePlayer::create(0);
	player->updatePlayerFrame(getPlayerIconIndex(type), type);
	player->setColor(gm->colorForIdx(gm->getPlayerColor()));
	player->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
	player->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));
	player->enableCustomGlowColor(gm->colorForIdx(gm->getPlayerGlowColor()));
	if(!gm->getPlayerGlow() && !forceGlow) player->disableGlowOutline();
	return player;
}


class $modify(LevelEditorLayer) {
	// all I need is to detect when editor is opened and closed.
	// I achieve this by using Fields constructor and destructor.
	// It requires having at least one modified function and at least 
	// one field (with at least one usage) in Fields so that 
	// nothing will be cleared during the compilation.
	struct Fields {
		char a = 1;
		Fields() {GLOBAL.m_isInEditor = true;}
		~Fields() {GLOBAL.m_isInEditor = false;}
	};
	bool init(GJGameLevel* p0, bool p1) {
		if (m_fields->a == 0){m_fields->a = 1;};
		return LevelEditorLayer::init(p0, p1);
	}
};


class $modify(Slider) {
	void upgradeSlider() {
		auto thumb = this->getThumb();

		auto player2 = getPlayerFrame(GLOBAL.m_settings.m_iconType, GLOBAL.m_settings.m_forceGlow);
		player2->setContentSize(thumb->getContentSize());
		player2->setScale(.9f);
		player2->setPosition(thumb->getContentSize()/2);

		auto player3 = getPlayerFrame(GLOBAL.m_settings.m_iconType, GLOBAL.m_settings.m_forceGlow);
		player3->setContentSize(thumb->getContentSize());
		player3->setScale(.9f);
		player3->setPosition(thumb->getContentSize()/2);

		// thumb->setDisabledImage(player); // invisible (unused)
		thumb->setNormalImage(player2); // static
		thumb->setSelectedImage(player3); // on move
	}

	// p2 - ???, p3 - groove texture, p4 - static texture, p5 - selected texture
	bool init(CCNode *p0, SEL_MenuHandler p1, const char *p2, const char *p3, const char *p4, const char *p5, float p6) {
		if (!Slider::init(p0, p1, p2, p3, p4, p5, p6)) {		
			return false;
		}
		if(!GLOBAL.m_settings.m_isEnabled) return true;

		if(GLOBAL.m_isInEditor) {
			// we are in the editor now
			if (p4 != 0) {
				if (std::strcmp(p4, "sliderthumb.png") == 0) {
					// this is trigger sliders
					if (GLOBAL.m_settings.m_affectTriggerSliders) {
						upgradeSlider();
					}
				} else if (std::strcmp(p4, "GJ_colorThumbBtn.png") == 0) {
					// this is color sliders
					// dont modify them (because this seems creepy)
				} else if (std::strcmp(p4, "GJ_moveBtn.png") == 0) {
					// this is position slider
					if (GLOBAL.m_settings.m_affectEditorSlider) {
						upgradeSlider();
					}
				} else {
					// what!?
				}
			}
		} else {
			// we are not in the editor now
			upgradeSlider();
		}

		return true;
	}
};
