#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <variant>

using namespace std::literals;


Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    
    const auto& cell = sheet_.find(pos);

    if (cell == sheet_.end()) {
        sheet_.emplace(pos, std::make_unique<Cell>(*this));
    }
    sheet_.at(pos)->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPtr(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    const auto& cell = sheet_.find(pos);
    if (cell != sheet_.end() && cell->second != nullptr) {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) {
            cell->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size result{ 0, 0 };
    
    for (auto it = sheet_.begin(); it != sheet_.end(); ++it) {
        if (it->second != nullptr) {
            const int col = it->first.col;
            const int row = it->first.row;
            result.rows = std::max(result.rows, row + 1);
            result.cols = std::max(result.cols, col + 1);
        }
    }

    return { result.rows, result.cols };
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            const auto& it = sheet_.find({ row, col });
            if (it != sheet_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                std::visit([&](const auto value) { output << value; }, it->second->GetValue());
            }
        }
        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            const auto& it = sheet_.find({ row, col });
            if (it != sheet_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                output << it->second->GetText();
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    const auto cell = sheet_.find(pos);
    if (cell == sheet_.end()) {
        return nullptr;
    }

    return sheet_.at(pos).get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}