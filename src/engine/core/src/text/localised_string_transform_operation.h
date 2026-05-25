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
        virtual bool isEquivalentTo(const ILocStrOp& other) const = 0;

    protected:
        static void setString(LocalisedString& dst, String value, Vector<LocalisedString::TokenInfo> tokenInfos);
    };

    class LocStrOpReplaceTokens : public ILocStrOp {
    public:
        LocStrOpReplaceTokens(LocalisedString original, Vector<String> ids, Vector<LocalisedString> toks);

        void eval(LocalisedString& dst) override;
        bool checkForUpdates() override;
        void setLanguage(const I18NLanguage& language) override;
        std::shared_ptr<ILocStrOp> clone() const override;
        bool isEquivalentTo(const ILocStrOp& other) const override;

    private:
        LocalisedString original;
		Vector<String> ids;
    	Vector<LocalisedString> toks;

    	bool isEquivalentToReplaceTokens(const LocStrOpReplaceTokens& other) const;
    };

    class LocStrOpSubstr : public ILocStrOp {
    public:
        LocStrOpSubstr(LocalisedString original, size_t offset, size_t count);

        void eval(LocalisedString& dst) override;
        bool checkForUpdates() override;
        void setLanguage(const I18NLanguage& language) override;
        std::shared_ptr<ILocStrOp> clone() const override;
        bool isEquivalentTo(const ILocStrOp& other) const override;

    private:
        LocalisedString original;
        size_t offset;
        size_t count;

    	bool isEquivalentToSubstr(const LocStrOpSubstr& other) const;
    };
}
