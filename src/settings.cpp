// my own setting for previewing slider

class MySliderSetting : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
        auto res = std::make_shared<MySliderSetting>();
        auto root = checkJson(json, "MySliderSetting");
        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const& json) override {return true;}
    bool save(matjson::Value& json) const override {return true;}
    bool isDefaultValue() const override {return true;}
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};


class MySliderSettingNode : public SettingNodeV3 {
protected:
    Slider* m_slider;
    CCMenuItemSpriteExtra* m_button;
    CCMenu* m_menu;
    short m_lastUpdateId;

    bool init(std::shared_ptr<MySliderSetting> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        // node setup
        this->setContentHeight(50.f);
        m_lastUpdateId = GLOBAL.m_settings.m_updateId;

        // menu setup
        m_menu = this->getButtonMenu();
        m_menu->setContentWidth(210.f);
    
        // slider
        m_slider = Slider::create(this, nullptr, .77f);
        m_slider->setValue(.5f);
        m_menu->addChildAtPosition(m_slider, Anchor::Center);

        // button
        auto m_buttonSprite = ButtonSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
        m_buttonSprite->setScale(.5f);
        m_button = CCMenuItemSpriteExtra::create(m_buttonSprite, this, 
            menu_selector(MySliderSettingNode::onRefreshButton));
        m_menu->addChildAtPosition(m_button, Anchor::Left);

        m_menu->updateLayout();
        this->updateState(nullptr);
        return true;
    }

    void onRefreshButton(CCObject*) {
        if (m_lastUpdateId == GLOBAL.m_settings.m_updateId) {
            FLAlertLayer::create(
                "Info",
                "<co>Apply</c> the settings to update the preview",
                "OK"
            )->show();
        } else {
            float value = m_slider->getValue();
            m_slider->removeFromParent(); // remove old slider
            // and replace it with the new one
            m_slider = Slider::create(this, nullptr, .77f);
            m_slider->setValue(value);
            m_menu->addChildAtPosition(m_slider, Anchor::Center);

            m_lastUpdateId = GLOBAL.m_settings.m_updateId;
        }
    }
    
    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static MySliderSettingNode* create(std::shared_ptr<MySliderSetting> setting, float width) {
        auto ret = new MySliderSettingNode();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool hasUncommittedChanges() const override {return false;}
    bool hasNonDefaultValue() const override {return false;}

    std::shared_ptr<MySliderSetting> getSetting() const {
        return std::static_pointer_cast<MySliderSetting>(SettingNodeV3::getSetting());
    }
};

SettingNodeV3* MySliderSetting::createNode(float width) {
    return MySliderSettingNode::create(
        std::static_pointer_cast<MySliderSetting>(shared_from_this()),
        width
    );
}

// Register the setting
$execute {
    (void)Mod::get()->registerCustomSettingType("my-slider", &MySliderSetting::parse);
}
