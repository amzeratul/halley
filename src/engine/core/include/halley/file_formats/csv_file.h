#pragma once

namespace Halley {
    class CSVFile {
    public:
        void setColumns(Vector<String> columns);
        const Vector<String>& getColumns() const;
        std::optional<size_t> getColumnIndex(const String& column) const;

        size_t addRow();
        size_t addRow(gsl::span<const String> data);
        gsl::span<const String> getRow(size_t idx) const;
        gsl::span<String> getRow(size_t idx);
        size_t getNumRows() const;

    	void setCell(size_t row, std::optional<size_t> column, String value);
    	const String& getCell(size_t row, std::optional<size_t> column) const;

        void clear();

        void load(const String& str);
        String save() const;

    private:
        Vector<String> columns;
        Vector<String> data;
    };
}
