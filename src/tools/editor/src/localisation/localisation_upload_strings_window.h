#pragma once
#include "localisation_string_upload_data.h"

namespace Halley {
	class LocalisationClient;

    class LocUploadStringsGrid : public UIGrid {
    public:
        LocUploadStringsGrid(UIFactory& factory, LocStringUploadData& data, HashMap<String, Vector<String>>& keysLocalisedIn);

        const String& getKeyAt(int idx) const override;
        size_t getSrcRowCount() const override;
        
        std::pair<Vector<float>, Vector<String>> getColumns() const override;
        void getLineDrawData(int idx, Vector<String>& strs, Vector<Colour4f>& colours, Vector<Sprite>& sprites) const override;
        String getCellToolTip(int row, int col, const String& columnName) const override;

        void onRightClick(std::optional<int> line) override;
        void copySelection() override;

        const LocStringUploadChunkData& getChunk(int idx) const;
        LocStringUploadChunkData::Entry& getEntry(int idx) const;

    private:
        LocStringUploadData& uploadData;
        HashMap<String, Vector<String>>& keysLocalisedIn;

        Vector<std::pair<int, int>> mapping;

        Sprite tickSprite;
        Sprite locSprite;
        
    	void generateMapping();
        String getTypeDesc(LocStringUploadEntryType type) const;
        std::optional<Colour4f> getRowColour(int row) const;
    };

	class LocUploadStringsWindow : public UIWidget {
    public:
        LocUploadStringsWindow(UIFactory& factory, LocalisationClient& client, LocStringUploadData uploadData, HashMap<String, Vector<String>> keysLocalisedIn);

        void onMakeUI() override;
        void onAddedToRoot(UIRoot& root) override;
        void update(Time t, bool moved) override;

    private:
        UIFactory& factory;
        LocalisationClient& client;
        LocStringUploadData uploadData;
        HashMap<String, Vector<String>> keysLocalisedIn;

		AliveFlag aliveFlag;
        std::shared_ptr<LocUploadStringsGrid> grid;

        bool onlyShowSend = false;

        enum class Status {
	        Idle,
            Uploading,
            Success,
            Error
        };

        int sendCount = 0;
		Status curStatus = Status::Idle;

        void upload();
        void doUpload();
        void setStatus(const String& message, Status status);
        void updateSummary();
        void updateButtons();
        void markSend(bool toSend);
        void markAllSend(bool toSend);
        void markSend(const HashSet<int>& lines, bool toSend);
        void selectGroup();
        void selectGroup(const String& id);

        void saveReport();
        String generateReport() const;
    };
}
