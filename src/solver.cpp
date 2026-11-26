#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

using board_t = std::uint32_t;

struct RGB {
  int r{};
  int g{};
  int b{};
  std::ostream &backgroundColour(std::ostream &out) const {
    return out << "\x1b[48;2;" << r << ";" << g << ";" << b << "m";
  }
  static std::ostream &endBackgroundColour(std::ostream &out) {
    return out << "\x1b[49m";
  }
  std::ostream &textColour(std::ostream &out) const {
    return out << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
  }
  static std::ostream &endTextColour(std::ostream &out) {
    return out << "\x1b[39m";
  }
};

class Game {
public:
  static constexpr int maxSize{32};
  static constexpr RGB blue{18, 50, 147};
  static constexpr RGB white{255, 255, 255};
  static constexpr RGB grey{170, 170, 170};

  static constexpr board_t column(int c) {
    return static_cast<board_t>(1) << c;
  }

  explicit Game(std::istream &in) {
    char ch{};
    in >> m_size;
    for (std::size_t r{0}; r < static_cast<size_t>(m_size); ++r) {
      for (std::size_t c{0}; c < static_cast<size_t>(m_size); ++c) {
        in >> ch;
        switch (ch) {
        case 'X':
          place(r, c, true);
          break;
        case 'O':
          place(r, c, false);
          break;
        default:
          continue;
        }
      }
    }
  }

  bool isRowValid(board_t row) const {
    return !((row & row >> 1 & row >> 2) || std::popcount(row) > m_size / 2);
  }

  bool isRowComplete(board_t row, board_t rowm_mask) const {
    return std::popcount(rowm_mask) == m_size &&
           std::popcount(row) == m_size / 2;
  }

  bool canPlace(int r, int c) const {
    if (r >= m_size || c >= m_size)
      return false;
    return !(m_mask[r] & column(c));
  }

  void place(int r, int c, bool crosses) {
    if (!canPlace(r, c))
      throw std::invalid_argument("game.place: invalid position");
    board_t cMask{column(c)};
    board_t cMaskT{column(r)};
    m_mask[r] |= cMask;
    m_maskT[c] |= cMaskT;
    if (crosses) {
      m_cells[r] |= cMask;
      m_cellsT[c] |= cMaskT;
    }
  }

  void unPlace(int r, int c) {
    if (canPlace(r, c))
      throw std::invalid_argument("game.unplace: invalid position");
    board_t cMask{~column(c)};
    board_t cMaskT{~column(r)};
    m_mask[r] &= cMask;
    m_maskT[c] &= cMaskT;
    m_cells[r] &= cMask;
    m_cellsT[c] &= cMaskT;
  }

  bool isValid() const {
    for (std::size_t i{0}; i < static_cast<size_t>(m_size); ++i) {
      if (!isRowValid(m_cells[i]))
        return false;
      if (!isRowValid(~m_cells[i] & m_mask[i]))
        return false;
      if (!isRowValid(m_cellsT[i]))
        return false;
      if (!isRowValid(~m_cellsT[i] & m_maskT[i]))
        return false;
    }
    return true;
  }

  bool isWon() const {
    for (std::size_t i{0}; i < static_cast<size_t>(m_size); ++i) {
      if (!isRowComplete(m_cells[i], m_mask[i]))
        return false;
      if (!isRowComplete(m_cellsT[i], m_maskT[i]))
        return false;
    }
    return true;
  }

  int size() const { return m_size; }
  board_t rowCells(std::size_t r) { return m_cells[r]; }
  board_t rowMask(std::size_t r) { return m_mask[r]; }
  board_t columnCells(std::size_t c) { return m_cellsT[c]; }
  board_t columnMask(std::size_t c) { return m_maskT[c]; }
  board_t boardMask() { return (static_cast<board_t>(1) << m_size) - 1; }

  friend std::ostream &operator<<(std::ostream &out, Game g);

private:
  int m_size{};
  std::array<board_t, maxSize> m_cells{};
  std::array<board_t, maxSize> m_mask{};
  std::array<board_t, maxSize> m_cellsT{};
  std::array<board_t, maxSize> m_maskT{};
};

std::ostream &operator<<(std::ostream &out, Game g) {
  for (std::size_t r{0}; r < static_cast<size_t>(g.m_size); ++r) {
    for (std::size_t c{0}; c < static_cast<size_t>(g.m_size); ++c) {
      if ((g.m_mask[r] >> c) & 1)
        if ((g.m_cells[r] >> c) & 1)
          Game::blue.backgroundColour(out);
        else
          Game::white.backgroundColour(out);
      else
        Game::grey.backgroundColour(out);
      std::cout << "  ";
    }
    RGB::endBackgroundColour(out);
    std::cout << '\n';
  }
  return out;
}

class BacktrackSolver {
public:
  BacktrackSolver() = default;

  bool solve(Game &g) {
    m_calls = 1;
    return backtrack(g, 0, 0);
  }
  int calls() const { return m_calls; }

private:
  int m_calls{};

  bool backtrack(Game &g, int r, int c) {
    if (c >= g.size()) {
      c = 0;
      ++r;
    }
    if (r >= g.size())
      return true;
    if (!g.isValid())
      return false;
    for (; r < g.size(); ++r) {
      for (; c < g.size(); ++c) {
        if (!g.canPlace(r, c))
          continue;
        g.place(r, c, true);
        ++m_calls;
        if (backtrack(g, r, c + 1))
          return true;
        g.unPlace(r, c);
        g.place(r, c, false);
        ++m_calls;
        if (backtrack(g, r, c + 1))
          return true;
        g.unPlace(r, c);
        return false;
      }
      c = 0;
    }
    return false;
  }
};

using placed_t = std::vector<std::pair<int, int>>;

class DeductiveSolver {
public:
  DeductiveSolver() = default;

  static placed_t deduce(Game &g) {
    placed_t placed{};
    bool progress{true};
    while (progress) {
      progress = false;
      for (std::size_t i{0}; i < rules.size(); ++i) {
        if (rules[i](g, placed)) {
          progress = true;
          break;
        }
      }
    }
    return placed;
  }

private:
  static void markPairs(Game &g, placed_t &placed, int i, board_t cells,
                        bool crosses, bool transposed) {
    while (cells) {
      int j = std::countr_zero(cells);
      std::pair<int, int> coords{(transposed ? i : j), (transposed ? j : i)};
      g.place(coords.first, coords.second, crosses);
      placed.push_back(coords);
      cells &= cells - 1;
    }
  }

  static bool adjacentPairs(Game &g, placed_t &placed) {
    bool progress{false};
    const board_t boardMask{g.boardMask()};
    const auto freeLeft = [boardMask](const board_t cells, const board_t mask) {
      return (cells & (cells << 1) << 1) & ~mask & boardMask;
    };
    const auto freeRight = [boardMask](const board_t cells,
                                       const board_t mask) {
      return (cells & (cells >> 1) >> 1) & ~mask & boardMask;
    };
    const auto free = [&freeLeft, &freeRight](const board_t cells,
                                              const board_t mask) {
      return freeLeft(cells, mask) | freeRight(cells, mask);
    };
    const auto check = [&g, &placed, &free](const int i, const board_t cells,
                                            const board_t mask,
                                            const bool transposed) {
      board_t crosses{free(cells, mask)};
      board_t naughts{free(~cells & mask, mask)};
      if (crosses)
        markPairs(g, placed, i, crosses, true, transposed);
      if (naughts)
        markPairs(g, placed, i, naughts, false, transposed);
      return static_cast<bool>(crosses | naughts);
    };

    for (int i{0}; i < g.size(); ++i) {
      const bool row{check(i, g.rowCells(i), g.rowMask(i), false)};
      const bool column{check(i, g.columnCells(i), g.columnMask(i), true)};
      if (row || column)
        progress = true;
    }
    return progress;
  }

  static constexpr std::array rules{&adjacentPairs};
};

class HybridSolver {
public:
  HybridSolver() = default;

  bool solve(Game &g) {
    m_calls = 1;
    return backtrack(g);
  }

  int calls() const { return m_calls; }

private:
  DeductiveSolver m_ds{};
  int m_calls{};

  bool backtrack(Game &g) {
    if (g.isWon())
      return true;
    placed_t placed{m_ds.deduce(g)};
    if (g.isWon())
      return true;
    const board_t boardMask{g.boardMask()};
    std::pair<int, int> rMin{-1, g.size() + 1};
    for (int r{0}; r < g.size(); ++r) {
      int count{std::popcount(g.rowMask(r))};
      if (rMin.second < count)
        rMin = {r, count};
    }
    board_t freeColumns{~g.rowMask(rMin.first) & boardMask};
    std::pair<int, int> cMin{-1, g.size() + 1};
    while (freeColumns) {
      int c{std::countr_zero(freeColumns)};
      int count{std::popcount(g.rowMask(c))};
      if (cMin.second > count)
        cMin = {c, count};
      freeColumns &= freeColumns - 1;
    }

    int nCrosses{std::popcount(g.rowCells(rMin.first)) +
                 std::popcount(g.columnCells(cMin.first))};
    bool crosses{nCrosses >= rMin.second + cMin.second};

    g.place(rMin.first, cMin.first, crosses);
    ++m_calls;
    if (backtrack(g))
      return true;
    g.unPlace(rMin.first, cMin.first);
    g.place(rMin.first, cMin.first, !crosses);
    ++m_calls;
    if (backtrack(g))
      return true;
    g.unPlace(rMin.first, cMin.first);

    for (auto p : placed) {
      g.unPlace(p.first, p.second);
    }

    return false;
  }
};

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    std::cerr << "No file provided\n";
    return 1;
  }
  std::ifstream file{argv[1]};
  if (!file) {
    std::cerr << "File " << argv[1] << " does not exist\n";
    return 1;
  }
  Game g{file};
  BacktrackSolver bs{};
  std::cout << g << '\n';
  std::cout << std::boolalpha << bs.solve(g) << "\nCalls: " << bs.calls()
            << '\n'
            << g << '\n';
  return 0;
}