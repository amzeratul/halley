#pragma once
#include "localisation_string_upload_data.h"

namespace Halley {
	class LocUploadStringsWindow;
	class ProjectWindow;
	class LocalisationClient;

    class LocUploadStringsGrid : public UIGrid {
    public:
        LocUploadStringsGrid(LocUploadStringsWindow& window, UIFactory& factory, LocStringUploadData& data, HashMap<String, Vector<String>>& keysLocalisedIn);

        const String& getKeyAt(int idx) const override;
        size_t getSrcRowCount() const override;
        
        std::pair<Vector<float>, Vector<String>> getColumns() const override;
        void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites, Vector<Vector<ColourOverride>>& colourOverrides) const override;
        String getCellToolTip(int row, int col, const String& columnName) const override;

        void onRightClick(std::optional<int> line) override;
        void copySelection() override;

        const LocStringUploadChunkData& getChunk(int idx) const;
        LocStringUploadChunkData::Entry& getEntry(int idx) const;
        size_t getNumEntries() const;

        bool onKeyPress(KeyboardKeyPress key) override;

    private:
        LocUploadStringsWindow& window;
        LocStringUploadData& uploadData;
        HashMap<String, Vector<String>>& keysLocalisedIn;

        Vector<std::pair<int, int>> mapping;

        Sprite tickSprite;
        Sprite locSprite;
        Sprite minorRevSprite;
        
    	void generateMapping();
        String getTypeDesc(LocStringUploadEntryType type, bool minor) const;
        std::optional<Colour4f> getRowColour(int row) const;
        Vector<ColourOverride> getColourOverrides(const Vector<std::pair<StringDiffType, size_t>>& values) const;
        Colour4f getColourOverride(StringDiffType diffType) const;
    };

    class LocUploadStringsState {
    public:
        struct Entry {
            bool send = false;
	        bool minor = false;

            Entry() = default;
            Entry(bool send, bool minor);
            Entry(const ConfigNode& node);

            ConfigNode toConfigNode() const;
        };

        LocUploadStringsState() = default;
        LocUploadStringsState(const ConfigNode& node);

        ConfigNode toConfigNode() const;

        HashMap<String, Entry>& getEntries();
        const HashMap<String, Entry>& getEntries() const;
        
    	Entry& get(const String& key);
    	const Entry* tryGet(const String& key) const;
        void remove(const String& key);

    private:
        HashMap<String, Entry> entries;
    };

	class LocUploadStringsWindow : public UIWidget {
    public:
        LocUploadStringsWindow(UIFactory& factory, ProjectWindow& projectWindow, LocalisationClient& client, LocStringUploadData uploadData, HashMap<String, Vector<String>> keysLocalisedIn);

        void onMakeUI() override;
        void onAddedToRoot(UIRoot& root) override;
        void update(Time t, bool moved) override;

        void toggleMinor();
        void toggleSend();

    private:
        UIFactory& factory;
        ProjectWindow& projectWindow;
        LocalisationClient& client;
        LocStringUploadData uploadData;
        HashMap<String, Vector<String>> keysLocalisedIn;

		AliveFlag aliveFlag;
        std::shared_ptr<LocUploadStringsGrid> grid;

        bool onlyShowSend = false;
        bool onlyShowModified = false;

        enum class Status {
	        Idle,
            Uploading,
            Success,
            Error
        };

        int sendCount = 0;
		Status curStatus = Status::Idle;
        bool testMode = false;

		LocUploadStringsState state;

        void upload();
        void doUpload();
        void setStatus(const String& message, Status status);
        void updateSummary();
        void updateButtons();
        void markSend(bool toSend);
        void markMinor(bool minor);
        void markAllSend(bool toSend);
        void markSend(const HashSet<int>& lines, bool toSend);
        void markMinor(const HashSet<int>& lines, bool minor);
        void selectGroup();
        void selectGroup(const String& id);

        void saveReport();
        String generateReport() const;

        void saveState();
        void loadState();
    };
}
