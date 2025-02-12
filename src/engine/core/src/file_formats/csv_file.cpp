#include "halley/file_formats/csv_file.h"

using namespace Halley;

void CSVFile::setColumns(Vector<String> columns)
{
	if (!this->columns.empty()) {
		throw Exception("Setting CSV columns more than once is not implemented.", HalleyExceptions::Utils);
	}

	clear();
	this->columns = std::move(columns);
}

const Vector<String>& CSVFile::getColumns() const
{
	return columns;
}

std::optional<size_t> CSVFile::getColumnIndex(const String& column) const
{
	const auto iter = columns.find(column);
	if (iter == columns.end()) {
		return {};
	} else {
		return iter - columns.begin();
	}
}

size_t CSVFile::addRow()
{
	const auto idx = getNumRows();
	data.resize(data.size() + columns.size(), String());
	return idx;
}

size_t CSVFile::addRow(gsl::span<const String> newData)
{
	if (columns.size() != newData.size()) {
		throw Exception("Number of entries in data doesn't match number of columns.", HalleyExceptions::Utils);
	}

	const auto idx = getNumRows();
	data.insert(data.end(), newData.begin(), newData.end());
	return idx;
}

void CSVFile::setCell(size_t row, std::optional<size_t> column, String value)
{
	if (column && *column < columns.size()) {
		getRow(row)[*column] = std::move(value);
	}
}

const String& CSVFile::getCell(size_t row, std::optional<size_t> column) const
{
	if (column && *column < columns.size()) {
		return getRow(row)[*column];
	} else {
		return String::emptyString();
	}
}

size_t CSVFile::getNumRows() const
{
	return data.size() / columns.size();
}

gsl::span<const String> CSVFile::getRow(size_t idx) const
{
	return data.const_span().subspan(idx * columns.size(), columns.size());
}

gsl::span<String> CSVFile::getRow(size_t idx)
{
	return data.span().subspan(idx * columns.size(), columns.size());
}

void CSVFile::clear()
{
	data.clear();
	data.shrink_to_fit();
}

void CSVFile::load(const String& str)
{
	// TODO
}

String CSVFile::save() const
{
	auto toCSVFormat = [](const String& str) -> String
	{
		if (str.startsWith(" ") || str.endsWith(" ") || str.contains(',') || str.contains('\"') || str.contains('\n')) {
			return "\"" + str.replaceAll("\"", "\"\"") + "\"";
		}

		return str;
	};

	std::stringstream str;

	// Header
	bool first = true;
	for (const auto& col: columns) {
		if (first) {
			first = false;
		} else {
			str << ',';
		}
		str << toCSVFormat(col);
	}
	str << '\n';

	// Rows
	const auto nRows = getNumRows();
	for (size_t i = 0; i < nRows; ++i) {
		const auto& row = getRow(i);

		first = true;
		for (const auto& elem: row) {
			if (first) {
				first = false;
			} else {
				str << ',';
			}
			str << toCSVFormat(elem);
		}
		str << '\n';
	}

	return str.str();
}
