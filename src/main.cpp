#include <Geode/Geode.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>

using namespace geode::prelude;

namespace matdash {
    struct Console {
        std::ofstream out, in;
        Console() {
            AllocConsole();
            out = decltype(out)("CONOUT$", std::ios::out);
            in = decltype(in)("CONIN$", std::ios::in);
            std::cout.rdbuf(out.rdbuf());
            std::cin.rdbuf(in.rdbuf());

            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
        }
        ~Console() {
            out.close();
            in.close();
        }
    };

    inline void create_console() {
        static Console console;
    }
}

enum class FieldType {
    Numeric = 0b0001,
    Signed = 0b0010,
    Float = 0b0100,
    NotNumeric = 0b1000,
};

constexpr int MIN_SIGNED = -999;
constexpr int MIN_UNSIGNED = 0;
constexpr int MAX_SIGNED = 999;
constexpr int INCREMENT_NORMAL = 1;
constexpr int INCREMENT_BIG = 5;
constexpr float INCREMENT_SMALL = .1f;

template<typename T>
static std::string formatNumericToString(T num, unsigned int precision = 60u) {
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

FieldType getCharType(char c) {
    const std::string validChars = "0123456789-+. ";

    for (char validChar : validChars) {
        if (validChar == c) {
            switch (c) {
            case '-': return FieldType::Signed;
            case '.': return FieldType::Float;
            default: return FieldType::Numeric;
            }
        }
    }

    return FieldType::NotNumeric;
}

std::vector<CCTextInputNode*> findTextInputNodesRecursively(CCNode* parent) {
    std::vector<CCTextInputNode*> result;

    for (int i = 0; i < parent->getChildrenCount(); ++i) {
        auto child = typeinfo_cast<CCNode*>(parent->getChildren()->objectAtIndex(i));

        auto inputNode = typeinfo_cast<CCTextInputNode*>(child);

        if (inputNode)
            result.push_back(inputNode);
        else {
            if (child->getChildrenCount() > 100 || !child->isVisible())
                continue;

            auto childResults = findTextInputNodesRecursively(child);
            result.insert(result.end(), childResults.begin(), childResults.end());
        }
    }

    return result;
}

CCTextInputNode* getInputNodeUnderMouse() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto winSizePx = CCDirector::sharedDirector()->getOpenGLView()->getViewPortRect();
    auto widthRatio = winSize.width / winSizePx.size.width;
    auto heightRatio = winSize.height / winSizePx.size.height;

    auto mousePosition = CCDirector::sharedDirector()->getOpenGLView()->getMousePosition();
    mousePosition.y = winSizePx.size.height - mousePosition.y;
    mousePosition.x *= widthRatio;
    mousePosition.y *= heightRatio;

    auto inputNodes = findTextInputNodesRecursively(scene);

    for (auto inputNode : inputNodes) {
        auto position = inputNode->getPosition();
        auto size = inputNode->getScaledContentSize();
        auto boundingBox = CCRect{ position.x, position.y, size.width, size.height };

        boundingBox.size = boundingBox.size * 1.2f;
        boundingBox.origin = boundingBox.origin - boundingBox.size / 2;

        auto mousePositionInNodeSpace = inputNode->getParent()->convertToNodeSpace(mousePosition);

        if (boundingBox.containsPoint(mousePositionInNodeSpace))
            return inputNode;
    }

    return nullptr;
}

class $modify(CCMouseDispatcher) {
public:
    bool dispatchScrollMSG(float x, float y) {
        auto self = getInputNodeUnderMouse();

        if (!self) return CCMouseDispatcher::dispatchScrollMSG(x, y);
        if (!self->isVisible()) return CCMouseDispatcher::dispatchScrollMSG(x, y);

        int type = 0;
        for (auto const& cf : std::string(self->m_allowedChars)) {
            type |= static_cast<int>(getCharType(cf));

            if (type & static_cast<int>(FieldType::NotNumeric))
                return true;
        }

        auto kb = CCDirector::sharedDirector()->getKeyboardDispatcher();

        auto val = 0.0f;

        if (strlen(self->getString().c_str()) > 0)
            try { val = std::stof(self->getString().c_str()); }
        catch (...) {}

        float inc = kb->getControlKeyPressed() ? INCREMENT_BIG : INCREMENT_NORMAL;

        if ((type & static_cast<int>(FieldType::Float)) && kb->getShiftKeyPressed())
            inc = INCREMENT_SMALL;

        float scrollSpeed = Mod::get()->getSettingValue<double>("scroll-speed");
        inc *= scrollSpeed;

        float smoothness = Mod::get()->getSettingValue<double>("scroll-smoothness");
        float stepSize = Mod::get()->getSettingValue<double>("scroll-step-size");
        float acceleration = Mod::get()->getSettingValue<double>("scroll-acceleration");

        float delta = -(x / 12.0f) * inc;
        delta *= acceleration;
        delta *= smoothness;
        delta *= stepSize;
        val += (Mod::get()->getSettingValue<bool>("scroll-reverse") ? -delta : delta);

        if (Mod::get()->getSettingValue<bool>("scroll-boundaries")) {
            if (type & static_cast<int>(FieldType::Signed)) {
                if (val < MIN_SIGNED)
                    val = MIN_SIGNED;
                else if (val > MAX_SIGNED)
                    val = MAX_SIGNED;
            }
            else {
                if (val < MIN_UNSIGNED)
                    val = MIN_UNSIGNED;
                else if (val > MAX_SIGNED)
                    val = MIN_UNSIGNED;
            }
        }

        if (!(type & static_cast<int>(FieldType::Float)))
            val = std::roundf(val);

        self->setString(formatNumericToString(val).c_str());

        if (self->m_delegate)
            self->m_delegate->textChanged(self);

        return true;
    }
};


$on_mod(Loaded) {
#ifdef GEODE_IS_WINDOWS
    //matdash::create_console();
#endif
}
