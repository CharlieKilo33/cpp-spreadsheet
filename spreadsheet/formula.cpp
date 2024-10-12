#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

#include "FormulaAST.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
  return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
 public:
  explicit Formula(std::string expression) try
      : ast_(ParseFormulaAST(std::move(expression))) {
  } catch (std::exception& exc) {
    throw FormulaException("");
  }

  Value Evaluate(const SheetInterface& sheet) const override {
    const std::function<double(Position)> args = [&sheet](const Position p)->double {
            if (!p.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }

            const auto* cell = sheet.GetCell(p);
            if (!cell) {
                return 0;
            }
            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }
            if (std::holds_alternative<std::string>(cell->GetValue())) {
                auto value = std::get<std::string>(cell->GetValue());
                double result = 0;
                if (!value.empty()) {
                    std::istringstream in(value);
                    if (!(in >> result) || !in.eof()) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }
                return result;
            }
            throw FormulaError(std::get<FormulaError>(cell->GetValue()));
        };

        try {
            return ast_.Execute(args);
        }
        catch (FormulaError& e) {
            return e;
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        for (auto cell : ast_.GetCells()) {
            if (cell.IsValid()) {
                cells.push_back(cell);
            }
        }
        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
        return cells;
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

 private:
  FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
  return std::make_unique<Formula>(std::move(expression));
}