#include <Geode/Geode.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>

using namespace geode::prelude;

static std::unordered_map<uintptr_t, std::vector<uint8_t>> g_patchedBytes;



static std::vector<uint8_t> intToBytes(int paramInt) {
    std::vector<uint8_t> arrayOfByte(4);
    for (int i = 0; i < 4; i++)
        arrayOfByte[3 - i] = (paramInt >> (i * 8));

    std::reverse(arrayOfByte.begin(), arrayOfByte.end());
    return arrayOfByte;
}

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

enum fType {
    ftNumeric = 0b0001,
    ftSigned = 0b0010,
    ftFloat = 0b0100,
    ftNot = 0b1000,
};

static constexpr const int MIN_SIGNED = -999;
static constexpr const int MIN_UNSIGNED = 0;
static constexpr const int MAX_SIGNED = 999;
static constexpr const int INC_NORMAL = 1;
static constexpr const int INC_BIG = 5;
static constexpr const float INC_SMALL = .1f;

fType isNumericChar(char c) {
    const std::string validChars = "0123456789-+. ";

    for (char cf : validChars) {
        if (cf == c) {
            switch (c) {
            case '-': return ftSigned;
            case '.': return ftFloat;
            default: return ftNumeric;
            }
        }
    }

    return ftNot;
}


// this exists entirely because dynamic_cast<gd::GameObject*> doesnt work
CCTextInputNode* castToInput(CCObject* obj) {
    if (obj != nullptr) {
          return dynamic_cast<CCTextInputNode*>(obj);
    }
    return nullptr;
}

std::vector<CCTextInputNode*> findTextInputNodes(CCNode* parent) {
    std::vector<CCTextInputNode*> res;

    for (int i = 0; i < parent->getChildrenCount(); ++i) {
        auto child = dynamic_cast<CCNode*>(parent->getChildren()->objectAtIndex(i));

        auto input = castToInput(child);

        if (input)
            res.push_back(input);
        else {
            if (child->getChildrenCount() > 100)
                continue;
            if (!child->isVisible())
                continue;

            auto cres = findTextInputNodes(child);

            res.insert(res.end(), cres.begin(), cres.end());
        }
    }

    return res;
}


CCTextInputNode* getInputUnderMouse() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto winSizePx = CCDirector::sharedDirector()->getOpenGLView()->getViewPortRect();
    auto ratio_w = winSize.width / winSizePx.size.width;
    auto ratio_h = winSize.height / winSizePx.size.height;

    auto mpos = CCDirector::sharedDirector()->getOpenGLView()->getMousePosition();
    // the mouse position is stored from the top-left while cocos
    // coordinates are from the bottom-left
    mpos.y = winSizePx.size.height - mpos.y;

    mpos.x *= ratio_w;  // scale mouse position to be in
    mpos.y *= ratio_h;  // cocos2d coordinate space

    auto inputs = findTextInputNodes(scene);

    for (auto input : inputs) {
        auto pos = input->getPosition();
        auto size = input->getScaledContentSize();
        auto rect = CCRect{ pos.x, pos.y, size.width, size.height };

        rect.size = rect.size * 1.2f;
        rect.origin = rect.origin - rect.size / 2;

        auto mposn = input->getParent()->convertToNodeSpace(mpos);

        if (rect.containsPoint(mposn))
            return input;
    }

    return nullptr;
}

class $modify(CCMouseDispatcher) {

    bool dispatchScrollMSG(float x, float y) {
        auto self = getInputUnderMouse();

        if (!self) return CCMouseDispatcher::dispatchScrollMSG(x, y);
        if (!self->isVisible()) return CCMouseDispatcher::dispatchScrollMSG(x, y);


        int type = 0;
        for (auto const& cf : std::string(self->m_allowedChars)) {
            type |= isNumericChar(cf);

            if (type & ftNot)
                return true;
        }

        auto kb = CCDirector::sharedDirector()->getKeyboardDispatcher();

        auto val = 0.0f;

        if (std::string(self->getString()).c_str() && strlen(self->getString().c_str()))
            try { val = std::stof(self->getString()); }
        catch (...) {}

        float inc = INC_NORMAL;

        if (kb->getControlKeyPressed()) inc = INC_BIG;
        if ((type & ftFloat) && kb->getShiftKeyPressed()) inc = INC_SMALL;

        val += -(x / 12.0f) * inc;

        if (type & ftSigned) {
            if (val < MIN_SIGNED) val = MIN_SIGNED;
        }
        else
            if (val < MIN_UNSIGNED) val = MIN_UNSIGNED;

        if (val > MAX_SIGNED)
            val = MAX_SIGNED;

        if (!(type & ftFloat))
            val = std::roundf(val);

        self->setString(formatToString(val).c_str());

        if (self->m_delegate)
            self->m_delegate->textChanged(self);

        return true;
    }
};
