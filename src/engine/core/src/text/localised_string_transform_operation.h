#pragma once
#include "halley/text/localised_string.h"

namespace Halley {
    class ILocStrOp {
    public:
        virtual ~ILocStrOp() = default;

	    virtual void eval(LocalisedString& dst) = 0;
        virtual bool checkForUpdates() = 0;
        virtual void setLanguage(const I18NLanguage& language) = 0;
        virtual std::shared_ptr<ILocStrOp> clone() const = 0;

    protected:
        static void setString(LocalisedString& dst, String value);
    };

    class LocStrOpReplaceTokens : public ILocStrOp {
    public:
        LocStrOpReplaceTokens(LocalisedString original, Vector<String> ids, Vector<LocalisedString> toks);

        void eval(LocalisedString& dst) override;
        bool checkForUpdates() override;
        void setLanguage(const I18NLanguage& language) override;
        std::shared_ptr<ILocStrOp> clone() const override;

    private:
        LocalisedString original;
		Vector<String> ids;
    	Vector<LocalisedString> toks;
    };
}
