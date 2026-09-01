#include "halley/file_formats/csv_file.h"

#include "halley/text/encode.h"
#include "halley/text/string_converter.h"
#include "halley/utils/utils.h"

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
	const auto newSize = data.size() + columns.size();
	data.reserve(nextPowerOf2(newSize)); // Important, otherwise we get O(n^2) performance
	data.resize(newSize, String());
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

void CSVFile::reserveRows(size_t nRows)
{
	data.reserve(nRows * columns.size());
}

void CSVFile::setCell(size_t row, std::optional<size_t> column, String value)
{
	if (column && *column < columns.size()) {
		getRow(row)[*column] = std::move(value);
	}
}

const String& CSVFile::getCell(size_t row, std::optional<size_t> column) const
{
	if (const auto* str = tryGetCell(row, column)) {
		return *str;
	} else {
		return String::emptyString();
	}
}

const String* CSVFile::tryGetCell(size_t row, std::optional<size_t> column) const
{
	if (column && *column < columns.size()) {
		return &getRow(row)[*column];
	} else {
		return nullptr;
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
	columns.clear();
	data.clear();
	data.shrink_to_fit();
}

void CSVFile::load(gsl::span<const std::byte> bytes)
{
	load(Encode::readBytesAsUTF8String(bytes));
}

void CSVFile::load(std::string_view origStr)
{
	clear();

	bool inHeader = true;
	size_t tokensInLine = 0;
	size_t lineNumber = 0;

	auto outputToken = [&](std::string_view token, bool quoted)
	{
		auto& dst = inHeader ? columns : data;
		if (quoted) {
			dst.push_back(String(token.substr(1, token.size() - 2)).replaceAll("\"\"", "\""));
		} else {
			dst.push_back(token);
		}
		++tokensInLine;
	};

	auto lineBreak = [&]()
	{
		if (tokensInLine == 0) {
			return;
		}

		if (inHeader) {
			inHeader = false;
		} else {
			if (tokensInLine != columns.size()) {
				clear();
				throw Exception("Inconsistent CSV: has " + toString(columns.size()) + " columns, but line " + toString(lineNumber + 1) + " has " + toString(tokensInLine) + " cells.", HalleyExceptions::Utils);
			}
		}
		tokensInLine = 0;
		++lineNumber;
	};

	std::optional<size_t> curTokenStart;
	bool quoting = false;
	bool lastTokenWasQuoted = false;

	auto strUTF32 = String(origStr).getUTF32();
	const auto nChars = strUTF32.size();
	int doubleQuotesInARow = 0; // CSV is bad

	for (size_t i = 0; i < nChars; ++i) {
		const char32_t curChar = strUTF32[i];
		const char32_t nextChar = i < nChars - 1 ? strUTF32[i + 1] : 0;

		// Start token
		if (!curTokenStart && curChar != '\r') {
			curTokenStart = i;
		}

		// Start/end quote block
		if (curChar == '\"') {
			++doubleQuotesInARow;
		} else {
			doubleQuotesInARow = 0;
		}
		if (!quoting && curChar == '\"') {
			quoting = true;
			doubleQuotesInARow = 0;
			lastTokenWasQuoted = true;
		} else if (quoting && curChar == '\"' && doubleQuotesInARow % 2 == 1 && nextChar != '\"') {
			quoting = false;
		}

		// End token
		if (!quoting && (curChar == ',' || curChar == '\r' || curChar == '\n')) {
			if (curTokenStart) {
				outputToken(String(strUTF32.substr(*curTokenStart, i - *curTokenStart)), lastTokenWasQuoted);
			}
			lastTokenWasQuoted = false;
			curTokenStart = std::nullopt;
		}

		// End line
		if (!quoting && curChar == '\n') {
			lineBreak();
		}
	}
}

String CSVFile::save(bool withBOM, bool withHeader) const
{
	auto toCSVFormat = [](const String& str) -> String
	{
		if (str.startsWith(" ") || str.endsWith(" ") || str.contains(',') || str.contains('\"') || str.contains('\n') || str.contains('\r')) {
			return "\"" + str.replaceAll("\"", "\"\"").replaceAll("\r\n", "\n").replaceAll("\r", "\n") + "\"";
		}

		return str;
	};

	std::stringstream str;

	if (withBOM) {
		str << static_cast<char>(0xEF);
		str << static_cast<char>(0xBB);
		str	<< static_cast<char>(0xBF);
	}

	// Header
	bool first = true;
	if (withHeader) {
		for (const auto& col: columns) {
			if (first) {
				first = false;
			} else {
				str << ',';
			}
			str << toCSVFormat(col);
		}
		str << '\n';
	}

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
