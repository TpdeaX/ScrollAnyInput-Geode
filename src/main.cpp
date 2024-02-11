#include <Geode/Geode.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>

using namespace geode::prelude;

template <class R, class T>
R& from(T base, intptr_t offset) {
    return *reinterpret_cast<R*>(reinterpret_cast<uintptr_t>(base) + offset);
}

static constexpr const int VOLUME_OVERLAY_TAG = 345893;

template<typename T>
static std::string formatToString(T num, unsigned int precision = 60u) {
    std::string res = std::to_string(num);

    if (std::is_same<T, float>::value && res.find('.') != std::string::npos) {
        while (res.at(res.length() - 1) == '0')
            res = res.substr(0, res.length() - 1);

        if (res.at(res.length() - 1) == '.')
            res = res.substr(0, res.length() - 1);

        if (res.find('.') != std::string::npos) {
            auto pos = res.find('.');

            if (precision)
                res = res.substr(0, pos + 1 + precision);
            else
                res = res.substr(0, pos);
        }
    }

    return res;
}


template <typename T>
static void HIDE_SPRITE(
    T* sprite,
    const GLubyte opacity)
{
    sprite->setOpacity(opacity);
    sprite->stopAllActions();
    sprite->runAction(cocos2d::CCSequence::create(
        cocos2d::CCDelayTime::create(1.0f),
        cocos2d::CCFadeTo::create(.6f, 0),
        nullptr));
}

class VolumeControlOverlay : public CCNode {
protected:
    CCScale9Sprite* m_pBG;
    Slider* m_pSliderMusic;
    Slider* m_pSliderSFX;
    CCLabelBMFont* m_pLabelMusic;
    CCLabelBMFont* m_pLabelSFX;
    CCLabelBMFont* m_pPercentMusic;
    CCLabelBMFont* m_pPercentSFX;
    CCLabelBMFont* m_pShiftLabel;
    CCLabelBMFont* m_pControlLabel;

    bool init(const int tag) {
        if (!CCNode::init())
            return false;

        m_pBG = cocos2d::extension::CCScale9Sprite::create(
            "square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f }
        );

        m_pBG->setScale(.5f);
        m_pBG->setColor({ 0, 0, 0 });
        m_pBG->setContentSize({ 520.0f, 130.0f });
        m_pBG->setOpacity(0);
        m_pBG->setPosition({ -32.5f, 0.0f });

        this->addChild(m_pBG);


        m_pLabelMusic = CCLabelBMFont::create("Music", "bigFont.fnt");
        m_pLabelMusic->setScale(.4f);
        m_pLabelMusic->setPosition(
            -150.0f + m_pLabelMusic->getScaledContentSize().width / 2,
            -20.0f
        );

        this->addChild(m_pLabelMusic);

        m_pPercentMusic = CCLabelBMFont::create("%", "bigFont.fnt");
        m_pPercentMusic->setScale(.35f);
        m_pPercentMusic->setPosition(
            80.0f, -20.0f
        );

        this->addChild(m_pPercentMusic);

        m_pSliderMusic = Slider::create(this, nullptr, .8f);
        m_pSliderMusic->m_touchLogic->m_thumb->setVisible(false);
        m_pSliderMusic->setValue(
            from<float>(FMODAudioEngine::sharedEngine(), 0x168)
        );
        m_pSliderMusic->m_sliderBar->setOpacity(0);
        m_pSliderMusic->m_groove->setOpacity(0);
        m_pSliderMusic->setPosition(-20.0f, -20.0f);

        this->addChild(m_pSliderMusic);


        m_pLabelSFX = CCLabelBMFont::create("SFX", "bigFont.fnt");
        m_pLabelSFX->setScale(.5f);
        m_pLabelSFX->setPosition(
            -150.0f + m_pLabelSFX->getScaledContentSize().width / 2,
            20.0f
        );

        this->addChild(m_pLabelSFX);

        m_pPercentSFX = CCLabelBMFont::create("%", "bigFont.fnt");
        m_pPercentSFX->setScale(.35f);
        m_pPercentSFX->setPosition(
            80.0f, 20.0f
        );

        this->addChild(m_pPercentSFX);

        m_pSliderSFX = Slider::create(this, nullptr, .8f);
        m_pSliderSFX->m_touchLogic->m_thumb->setVisible(false);
        m_pSliderSFX->setValue(
            from<float>(FMODAudioEngine::sharedEngine(), 0x16C)
        );
        m_pSliderSFX->m_sliderBar->setOpacity(0);
        m_pSliderSFX->m_groove->setOpacity(0);
        m_pSliderSFX->setPosition(-20.0f, 20.0f);

        this->addChild(m_pSliderSFX);


        m_pShiftLabel = CCLabelBMFont::create("(Shift)", "goldFont.fnt");
        m_pShiftLabel->setScale(.3f);
        m_pShiftLabel->setPosition(
            -150.0f + m_pLabelSFX->getScaledContentSize().width / 2,
            10.0f
        );

        this->addChild(m_pShiftLabel);


        m_pControlLabel = CCLabelBMFont::create("Hold Control to Fine-tune", "bigFont.fnt");
        m_pControlLabel->setScale(.3f);
        m_pControlLabel->setPosition(0, 0);

        this->addChild(m_pControlLabel);


        this->setTag(tag);

        return true;
    }

public:
    static VolumeControlOverlay* create(const int tag) {
        auto pRet = new VolumeControlOverlay();

        if (pRet && pRet->init(tag)) {
            pRet->autorelease();
            return pRet;
        }

        CC_SAFE_DELETE(pRet);
        return nullptr;
    }

    void show() {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        HIDE_SPRITE(m_pSliderMusic->m_sliderBar, 255);
        HIDE_SPRITE(m_pSliderMusic->m_groove, 255);
        HIDE_SPRITE(m_pSliderSFX->m_sliderBar, 255);
        HIDE_SPRITE(m_pSliderSFX->m_groove, 255);
        HIDE_SPRITE(m_pLabelMusic, 255);
        HIDE_SPRITE(m_pLabelSFX, 255);
        HIDE_SPRITE(m_pPercentMusic, 255);
        HIDE_SPRITE(m_pPercentSFX, 255);
        HIDE_SPRITE(m_pShiftLabel, 255);
        HIDE_SPRITE(m_pControlLabel, 100);
        HIDE_SPRITE(m_pBG, 205);

        this->setPosition({
            winSize.width - 120.0f,
            60.0f
            });
        this->setScale(1.0f);
    }

    void updateValue(float offset, bool sfx) {
        auto fe = FMODAudioEngine::sharedEngine();

        auto ovol = sfx ? from<float>(fe, 0x16C) : from<float>(fe, 0x168);
        auto vol = ovol + offset / 240.f;

        if (vol < 0.0f) vol = 0.0f;
        if (vol > 1.0f) vol = 1.0f;
        if (sfx)
            m_pSliderSFX->setValue(vol);
        else
            m_pSliderMusic->setValue(vol);

        m_pSliderMusic->updateBar();
        m_pSliderSFX->updateBar();

        if (sfx) {
            from<float>(fe, 0x16C) = vol;
            from<FMOD::Channel*>(fe, 0x19C)->setVolume(vol);
        }
        else {
            from<float>(fe, 0x168) = vol;
            from<bool>(fe, 0x174) = 0;
            from<FMOD::Channel*>(fe, 0x18C)->setVolume(vol);

            if (!GameManager::sharedState()->getEditorLayer() && !GameManager::sharedState()->getPlayLayer())
                if (ovol == 0.0f && vol > 0.0f)
                    GameManager::sharedState()->fadeInMenuMusic();
        }

        m_pPercentMusic->setString(
            (formatToString(round(from<float>(fe, 0x168) * 100.0f), 0u) + "%").c_str()
        );
        m_pPercentMusic->limitLabelWidth(20.0f, .35f, .2f);
        m_pPercentSFX->setString(
            (formatToString(round(from<float>(fe, 0x16C) * 100.0f), 0u) + "%").c_str()
        );
        m_pPercentSFX->limitLabelWidth(20.0f, .35f, .2f);
    }
};


class $modify(CCMouseDispatcher) {

    bool dispatchScrollMSG(float x, float y) {

        auto kb = CCDirector::sharedDirector()->getKeyboardDispatcher();

        if (kb->getAltKeyPressed()) {

            auto scene = CCDirector::sharedDirector()->getRunningScene();

            auto overlay = as<VolumeControlOverlay*>(scene->getChildByTag(VOLUME_OVERLAY_TAG));
            if (!overlay) {
                overlay = VolumeControlOverlay::create(VOLUME_OVERLAY_TAG);
                scene->addChild(overlay, 9999);
            }

            auto offset = -x;

            if (kb->getControlKeyPressed())
                offset /= 6.0f;

            //std::cout << offset << "\n";

            overlay->updateValue(offset, kb->getShiftKeyPressed());
            overlay->show();

            return true;

        }
        else {
            return  CCMouseDispatcher::dispatchScrollMSG(x, y);
        }

    }

};

