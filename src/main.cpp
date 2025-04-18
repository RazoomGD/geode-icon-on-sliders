#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/Slider.hpp>
#include <Geode/modify/SliderTouchLogic.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Dispatch.hpp>

// common user object id for this mod (IOS = icon on sliders)
#define USER_OBJ_ID "IOS_obj"

// more icons support
using MoreIconsSimplePlayerEvent = geode::DispatchEvent<SimplePlayer*, std::string, IconType>;

// is player using icon from "more icons"
static std::string moreIconsGetActiveIcon(IconType type) {
	if (!Loader::get()->isModLoaded("hiimjustin000.more_icons")) return "";
	std::string savedType;
	switch (type) {
		case IconType::Cube: savedType = "icon"; break;
		case IconType::Ship: savedType = "ship"; break;
		case IconType::Ball: savedType = "ball"; break;
		case IconType::Ufo: savedType = "ufo"; break;
		case IconType::Wave: savedType = "wave"; break;
		case IconType::Robot: savedType = "robot"; break;
		case IconType::Spider: savedType = "spider"; break;
		case IconType::Swing: savedType = "swing"; break;
		case IconType::Jetpack: savedType = "jetpack"; break;
		default: return "";
	}
	return Loader::get()->getLoadedMod("hiimjustin000.more_icons")->getSavedValue<std::string>(savedType, "");
}

struct {
	struct {
		bool m_isEnabled;
		IconType m_iconType;
		bool m_affectEditorSlider;
		bool m_affectTriggerSliders;
		bool m_animate;
		int m_glowMode; // 1 - default, 2 - on touch, 3 - force
		bool m_lightenOnTouch;
		float m_animStrength;

		short m_updateId = 1; // needed for slider preview in settings

		bool isAnimated() {
			return m_animate && (m_iconType != IconType::Wave);
		}

		void update() {
			m_isEnabled = Mod::get()->getSettingValue<bool>("is-enabled");
			m_animate = Mod::get()->getSettingValue<bool>("animate");
			m_lightenOnTouch = Mod::get()->getSettingValue<bool>("lighten-touch");
			m_affectEditorSlider = Mod::get()->getSettingValue<bool>("affect-editor-slider");
			m_affectTriggerSliders = Mod::get()->getSettingValue<bool>("affect-trigger-sliders");
			m_animStrength = Mod::get()->getSettingValue<double>("anim-strength");

			int iconType = std::atoi(Mod::get()->getSettingValue<std::string>("icon").c_str());
			iconType = (iconType <= 0 || iconType >= 10) ? 1 : iconType; // validate
			m_iconType = static_cast<IconType>(iconType - 1); // 0 <= IconType <= 8

			m_glowMode = std::atoi(Mod::get()->getSettingValue<std::string>("glow-mode").c_str());
			m_glowMode = (m_glowMode <= 0 || m_glowMode >= 4) ? 1 : m_glowMode; // validate

			m_updateId++;
		}
	} m_settings; 

} GLOBAL;


#include "settings.cpp"

enum class MoveState {None=0, Begin, End, Middle};

struct MySlider;

// default parameters for sliders
struct SliderInfo : CCObject {
	SEL_MenuHandler m_defaultSelector;
	MySlider* m_slider;
	MoveState m_moveState;
	SliderInfo(SEL_MenuHandler defaultSelector, MySlider* slider) {
		m_defaultSelector = defaultSelector;
		m_slider = slider;
		m_moveState = MoveState::None;
		this->autorelease();
	}
};


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

void lightenColor(ccColor3B* col) {
	const float add = 105, mul = 0.3;
	// lower saturation, increase brightness
	col->r = std::min(255, (int)col->r - (int)(col->r * mul) + (int)add);
	col->g = std::min(255, (int)col->g - (int)(col->g * mul) + (int)add);
	col->b = std::min(255, (int)col->b - (int)(col->b * mul) + (int)add);
}

// create player icon and put it on base node
SimplePlayer* getPlayerFrame(IconType type, bool forceGlow, bool forceNoGlow, bool lighterCol, CCNode* base) {
    auto gm = GameManager::sharedState();
	auto col1 = gm->colorForIdx(gm->getPlayerColor());
	auto col2 = gm->colorForIdx(gm->getPlayerColor2());
	auto col3 = gm->colorForIdx(gm->getPlayerGlowColor());

	if (lighterCol) {
		lightenColor(&col1);
		lightenColor(&col2);
	}

	SimplePlayer* player = SimplePlayer::create(0);
	base->addChild(player);

	std::string moreIconsName = moreIconsGetActiveIcon(type);
	if (!moreIconsName.empty()) {
		MoreIconsSimplePlayerEvent("hiimjustin000.more_icons/simple-player", player, moreIconsName, type).post();
		if (!player->getUserObject("hiimjustin000.more_icons/name")) {
			player->updatePlayerFrame(getPlayerIconIndex(type), type);
		}
	} else {
		player->updatePlayerFrame(getPlayerIconIndex(type), type);
	}

	player->setColor(col1);
	player->setSecondColor(col2);
	player->setGlowOutline(col3);
	player->enableCustomGlowColor(col3);
	if(!gm->getPlayerGlow() && !forceGlow || forceNoGlow) player->disableGlowOutline();
	
	if (type == IconType::Ship || type == IconType::Ufo || type == IconType::Jetpack) {
		// put the cube into the vehicle
		SimplePlayer* cube = SimplePlayer::create(0);
		base->addChild(cube, -1);

		moreIconsName = moreIconsGetActiveIcon(IconType::Cube);
		if (!moreIconsName.empty()) {
			MoreIconsSimplePlayerEvent("hiimjustin000.more_icons/simple-player", cube, moreIconsName, IconType::Cube).post();
			if (!cube->getUserObject("hiimjustin000.more_icons/name")) {
				cube->updatePlayerFrame(gm->getPlayerFrame(), IconType::Cube);
			}
		} else {
			cube->updatePlayerFrame(gm->getPlayerFrame(), IconType::Cube);
		}

		cube->setColor(col1);
		cube->setSecondColor(col2);
		cube->setGlowOutline(col3);
		cube->enableCustomGlowColor(col3);
		if(!gm->getPlayerGlow() && !forceGlow || forceNoGlow) cube->disableGlowOutline();

		// fix glow layering and position all sprites correctly
		CCPoint posCube, posVehicle;
		float scaleCube;
		switch (type) {
			case IconType::Ship: {posCube = ccp(0,5); posVehicle = ccp(0,-5); scaleCube = 0.55; break;}
			case IconType::Ufo: {posCube = ccp(0,5); posVehicle = ccp(0,0); scaleCube = 0.55; break;}
			case IconType::Jetpack: {posCube = ccp(6,4); posVehicle = ccp(0,0); scaleCube = 0.6; break;}
		}
		if (auto glowSpr = player->m_outlineSprite) {
			CCPoint textureOffset = glowSpr->getParent()->getPosition();
			glowSpr->removeFromParent();
			cube->addChild(glowSpr, -1);
			glowSpr->setScale(1 / scaleCube);
			glowSpr->setPosition((posVehicle - posCube + textureOffset) / scaleCube);
		}
		cube->setScale(scaleCube);
		cube->setPosition(posCube);
		player->setPosition(posVehicle);
	}
	
	return player;
}


class $modify(MySlider, Slider) {
	struct Fields {
		bool m_isAffected = false;
		float m_oldValue; // value before move
		Ref<SimplePlayer> m_staticImage;
		Ref<SimplePlayer> m_onMoveImage;
		Ref<CCNode> m_onMoveBaseNode;
		IconType m_icon;
	};

	void upgradeSlider() {
		m_fields->m_isAffected = true;

		auto thumb = this->getThumb();

		// SliderThumb -> Base -> Node -> SimplePlayer

		auto base = CCSprite::create();
		auto node = CCSprite::create();
		auto player = getPlayerFrame(GLOBAL.m_settings.m_iconType, 
			GLOBAL.m_settings.m_glowMode == 3, GLOBAL.m_settings.m_glowMode == 2, false, node);
		base->setContentSize(thumb->getContentSize());
		node->setScale(.9f);
		base->addChild(node);
		node->setPosition(thumb->getContentSize()/2);

		auto base2 = CCSprite::create();
		auto node2 = CCSprite::create();
		auto player2 = getPlayerFrame(GLOBAL.m_settings.m_iconType, 
			GLOBAL.m_settings.m_glowMode == 3 || GLOBAL.m_settings.m_glowMode == 2, 
			false, GLOBAL.m_settings.m_lightenOnTouch, node2);
		base2->setContentSize(thumb->getContentSize());
		node2->setScale(.9f);
		base2->addChild(node2);
		node2->setPosition(thumb->getContentSize()/2);

		// thumb->setDisabledImage(player); // invisible (unused)
		thumb->setNormalImage(base); // static
		thumb->setSelectedImage(base2); // on move

		m_fields->m_staticImage = player;
		m_fields->m_onMoveImage = player2;
		m_fields->m_icon = GLOBAL.m_settings.m_iconType;
		m_fields->m_onMoveBaseNode = node2;
	}

	// p2 - ???, p3 - groove texture, p4 - static texture, p5 - selected texture
	$override
	bool init(CCNode *p0, SEL_MenuHandler p1, const char *p2, const char *p3, const char *p4, const char *p5, float p6) {
		if (!Slider::init(p0, p1, p2, p3, p4, p5, p6)) return false;

		if (!GLOBAL.m_settings.m_isEnabled) return true;

		if(GameManager::sharedState()->m_levelEditorLayer != nullptr) {
			// we are in the editor now
			if (p4 != 0) {
				if (std::strcmp(p4, "sliderthumb.png") == 0) {
					// this is trigger slider
					if (GLOBAL.m_settings.m_affectTriggerSliders) {
						upgradeSlider();
					}
				} else if (std::strcmp(p4, "GJ_colorThumbBtn.png") == 0) {
					// this is color slider
					// dont modify them (because this looks creepy)
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

		// replace selector if this slider needs to be animated
		if (GLOBAL.m_settings.isAnimated() && m_fields->m_isAffected) {
			auto thumb = getThumb();
			auto sliderInfo = new SliderInfo(thumb->m_pfnSelector, this);
			thumb->setUserObject(USER_OBJ_ID, sliderInfo);
			thumb->m_pfnSelector = menu_selector(MySlider::onMoveAnimate);
		}
		
		return true;
	}

	// this is called on initial slider setup or when something changes slider value
	$override
	void setValue(float val) {
		Slider::setValue(val);
		if (GLOBAL.m_settings.isAnimated() && m_fields->m_isAffected) {
			setAnimation(this, getThumb(), MoveState::None);
		}
	}

	// this is called on moving animated sliders
	void onMoveAnimate(CCObject* sender) {
		const auto thumb = static_cast<SliderThumb*>(sender);
		const auto sliderInfo = static_cast<SliderInfo*>(thumb->getUserObject(USER_OBJ_ID));
		if (!sliderInfo) return;
		if (auto selector = sliderInfo->m_defaultSelector) {
			(this->*selector)(sender); // call original selector
		}
		setAnimation(sliderInfo->m_slider, thumb, sliderInfo->m_moveState);
		sliderInfo->m_moveState = MoveState::Middle;
	}

	// control animation of a slider (action = MoveState::None - ignore the anim)
	void setAnimation(MySlider* slider, SliderThumb* thumb, MoveState action) {
		const float val = thumb->getValue(); // 0 <= val <= 1
		const float delay = 0.08;
		float maxAngle = 22; 

		switch (slider->m_fields->m_icon) {
			case IconType::Ship:
			case IconType::Swing: {maxAngle *= -1;}
			case IconType::Cube:
			case IconType::Ufo:
			case IconType::Jetpack: {
				if (action == MoveState::Middle) {
					float speed = slider->getValue() - slider->m_fields->m_oldValue;
					int sign = speed >= 0 ? 1 : -1;
					float angle = sign * maxAngle * std::min(1.f, speed * sign * 50.f * GLOBAL.m_settings.m_animStrength);

					slider->m_fields->m_onMoveBaseNode->runAction(CCSequence::create(
						CCRotateTo::create(delay, angle), nullptr)); // smooth rotation

					// log::debug("count {}", slider->m_fields->m_onMoveBaseNode->numberOfRunningActions());

					// special action that resets rotation when slider is not moved
					slider->m_fields->m_onMoveBaseNode->stopActionByTag(42);
					auto special = CCSequence::create(
						CCDelayTime::create(delay), CCRotateTo::create(delay*2, 0), nullptr);
					special->setTag(42);
					slider->m_fields->m_onMoveBaseNode->runAction(special);
					
				} else if (action == MoveState::Begin || action == MoveState::End) {
					slider->m_fields->m_onMoveBaseNode->stopAllActions();
					slider->m_fields->m_onMoveBaseNode->setRotation(0);
				}
				break;
			}
			case IconType::Ball: {
				auto groove = slider->m_groove;
				float scaleRatio = (groove) ? groove->getScaleX() / groove->getScaleY() : 1.f;
				slider->m_fields->m_onMoveImage->setRotation(720.f * val * scaleRatio);
				slider->m_fields->m_staticImage->setRotation(720.f * val * scaleRatio);
				break;
			}
			case IconType::Robot: {
				if (action == MoveState::Begin) { // start
					slider->m_fields->m_onMoveImage->m_robotSprite->tweenToAnimation("fall_loop", delay);
					slider->m_fields->m_staticImage->m_robotSprite->tweenToAnimation("fall_loop", delay);
				} else if (action == MoveState::End) { // finish
					slider->m_fields->m_onMoveImage->m_robotSprite->tweenToAnimation("idle", delay);
					slider->m_fields->m_staticImage->m_robotSprite->tweenToAnimation("idle", delay);
				}
				break;
			}
			case IconType::Spider: {
				if (action == MoveState::Begin) { // start
					// todo: ask RobTop to fix spider leg animation transition bug
					slider->m_fields->m_onMoveImage->m_spiderSprite->tweenToAnimation("fall_loop", delay);
					slider->m_fields->m_staticImage->m_spiderSprite->tweenToAnimation("fall_loop", delay);
				} else if (action == MoveState::End) { // finish
					slider->m_fields->m_onMoveImage->m_spiderSprite->tweenToAnimation("idle", delay);
					slider->m_fields->m_staticImage->m_spiderSprite->tweenToAnimation("idle", delay);
				}
				break;
			}
			default: break;
		}
	}
};


// set these flags to know when the current slider state
class $modify(SliderTouchLogic) {
	$override
	bool ccTouchBegan(CCTouch* p0, CCEvent* p1) {
		if (auto obj = static_cast<SliderInfo*>(m_thumb->getUserObject(USER_OBJ_ID))) {
			obj->m_moveState = MoveState::Begin;
		}
		return SliderTouchLogic::ccTouchBegan(p0, p1);
	}

	$override
	void ccTouchMoved(CCTouch* p0, CCEvent* p1) {
		if (auto obj = static_cast<SliderInfo*>(m_thumb->getUserObject(USER_OBJ_ID))) {
			obj->m_slider->m_fields->m_oldValue = obj->m_slider->getValue();
		}
		SliderTouchLogic::ccTouchMoved(p0, p1);
	}

	$override
	void ccTouchEnded(CCTouch* p0, CCEvent* p1) {
		if (auto obj = static_cast<SliderInfo*>(m_thumb->getUserObject(USER_OBJ_ID))) {
			obj->m_moveState = MoveState::End;
		}
		SliderTouchLogic::ccTouchEnded(p0, p1);
	}
};
