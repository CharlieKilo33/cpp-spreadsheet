#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <queue>
#include <stack>
#include <string>

#include "sheet.h"

class Cell::Impl {
 public:
  virtual CellInterface::Value GetValue() const = 0;
  virtual std::string GetText() const = 0;
  virtual std::vector<Position> GetReferencedCells() const { return {}; }
  virtual bool IsCacheValid() const { return true; }
  virtual void InvalidateCache() {}
  virtual ~Impl() = default;
};

class Cell::EmptyImpl : public Impl {
 public:
  CellInterface::Value GetValue() const override { return ""; }

  std::string GetText() const override { return ""; }
};

class Cell::TextImpl : public Impl {
 public:
  explicit TextImpl(std::string text) : text_(std::move(text)) {
    if (text_.empty()) {
      throw std::logic_error("");
    }
  }

  CellInterface::Value GetValue() const override {
    if (!text_.empty() && text_.front() == ESCAPE_SIGN) {
      return text_.substr(1);
    }
    return text_;
  }

  std::string GetText() const override { return text_; }

 private:
  std::string text_;
};

class Cell::FormulaImpl : public Impl {
 public:
  explicit FormulaImpl(std::string expression, const SheetInterface& sheet)
      : sheet_(sheet) {
    if (expression.empty() || expression[0] != FORMULA_SIGN) {
      throw std::logic_error("");
    }

    formula_ptr_ = ParseFormula(expression.substr(1));
  }

  Value GetValue() const override {
    if (!cache_) {
      cache_ = formula_ptr_->Evaluate(sheet_);
    }

    auto value = formula_ptr_->Evaluate(sheet_);
    if (std::holds_alternative<double>(value)) {
      return std::get<double>(value);
    }

    return std::get<FormulaError>(value);
  }

  std::string GetText() const override {
    return FORMULA_SIGN + formula_ptr_->GetExpression();
  }

  bool IsCacheValid() const override { return cache_.has_value(); }

  void InvalidateCache() override { cache_.reset(); }

  std::vector<Position> GetReferencedCells() const {
    return formula_ptr_->GetReferencedCells();
  }

 private:
  std::unique_ptr<FormulaInterface> formula_ptr_;
  const SheetInterface& sheet_;
  mutable std::optional<FormulaInterface::Value> cache_;
};

Cell::~Cell() = default;

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

void Cell::Set(std::string text) {
  std::unique_ptr<Impl> impl;

  if (text.empty()) {
    impl = std::make_unique<EmptyImpl>();
  } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
    impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
  } else {
    impl = std::make_unique<TextImpl>(std::move(text));
  }

  if (CheckForCyclicity(*impl)) {
    throw CircularDependencyException("");
  }

  RemoveDependenciesFrom(this);

  impl_ = std::move(impl);

  for (const auto& pos : impl_->GetReferencedCells()) {
    Cell* outgoing = sheet_.GetCellPtr(pos);
    if (!outgoing) {
      sheet_.SetCell(pos, "");
      outgoing = sheet_.GetCellPtr(pos);
    }
    right_nods_.insert(outgoing);
    outgoing->AddDependency(this);
  }

  DoInvalidCache(true);
}

void Cell::Clear() { Set(""); }

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }

std::string Cell::GetText() const { return impl_->GetText(); }

std::vector<Position> Cell::GetReferencedCells() const {
  return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const { return !left_nods_.empty(); }

bool Cell::CheckForCyclicity(const Impl& impl) const {
  if (impl.GetReferencedCells().empty()) {
    return false;
  }

  std::unordered_set<const Cell*> referenced;
  for (const auto& pos : impl.GetReferencedCells()) {
    referenced.insert(sheet_.GetCellPtr(pos));
  }

  std::unordered_set<const Cell*> visited;
  std::stack<const Cell*> to_visit;
  to_visit.push(this);
  while (!to_visit.empty()) {
    const Cell* current = to_visit.top();
    to_visit.pop();
    visited.insert(current);

    if (referenced.find(current) != referenced.end()) {
      return true;
    }

    for (const Cell* incoming : current->left_nods_) {
      if (visited.find(incoming) == visited.end()) {
        to_visit.push(incoming);
      }
    }
  }

  return false;
}

void Cell::DoInvalidCache(bool check_changes) {
  if (impl_->IsCacheValid() || check_changes) {
    impl_->InvalidateCache();
    for (Cell* incoming : left_nods_) {
      incoming->DoInvalidCache();
    }
  }
}

void Cell::RemoveDependenciesFrom(Cell* cell) {
  for (Cell* outgoing : right_nods_) {
    outgoing->RemoveDependency(cell);
  }
}

void Cell::RemoveDependency(Cell* cell) { left_nods_.erase(cell); }

void Cell::AddDependency(Cell* cell) { left_nods_.insert(cell); }
