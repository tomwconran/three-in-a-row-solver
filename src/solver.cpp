#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
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

  explicit Game(int size) : m_size{size} {}

  bool isRowValid(board_t row, board_t rowMask) const {
    return !((row & (row >> 1) & (row >> 2)) ||
             std::popcount(row) > m_size / 2 ||
             std::popcount(rowMask) - std::popcount(row) > m_size / 2);
  }

  bool isRowComplete(board_t row, board_t rowMask) const {
    return isRowValid(row, rowMask) && std::popcount(rowMask) == m_size &&
           std::popcount(row) == m_size / 2;
  }

  bool canPlace(int r, int c) const {
    if (r < 0 || m_size <= r || c < 0 || m_size <= c)
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
      if (!isRowValid(m_cells[i], m_mask[i]))
        return false;
      if (!isRowValid(~m_cells[i] & m_mask[i], m_mask[i]))
        return false;
      if (!isRowValid(m_cellsT[i], m_maskT[i]))
        return false;
      if (!isRowValid(~m_cellsT[i] & m_maskT[i], m_maskT[i]))
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
    if (m_calls >= 1'000'000)
      return false;
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
  enum class Status {
    Stuck,
    Success,
    Contradiction,
  };

  DeductiveSolver() = default;

  static std::pair<Status, placed_t> deduce(Game &g) {
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
      if (!g.isValid()) {
        return {Status::Contradiction, placed};
      }
    }
    return {g.isWon() ? Status::Success : Status::Stuck, placed};
  }

private:
  static void mark(Game &g, placed_t &placed, int i, board_t cells,
                   bool crosses, bool transposed) {
    while (cells) {
      int j = std::countr_zero(cells);
      std::pair<int, int> coords{(transposed ? j : i), (transposed ? i : j)};
      g.place(coords.first, coords.second, crosses);
      placed.push_back(coords);
      cells &= cells - 1;
    }
  }

  static bool adjacentPairs(Game &g, placed_t &placed) {
    bool progress{false};
    const board_t boardMask{g.boardMask()};
    const auto freeLeft = [boardMask](const board_t cells, const board_t mask) {
      return ((cells & (cells << 1)) << 1) & ~mask & boardMask;
    };
    const auto freeRight = [boardMask](const board_t cells,
                                       const board_t mask) {
      return ((cells & (cells >> 1)) >> 1) & ~mask & boardMask;
    };
    const auto free = [&freeLeft, &freeRight](const board_t cells,
                                              const board_t mask) {
      return freeLeft(cells, mask) | freeRight(cells, mask);
    };
    const auto check = [&g, &placed, &free](const int i, const board_t cells,
                                            const board_t mask,
                                            const bool transposed) {
      board_t crosses{free(~cells & mask, mask)};
      board_t naughts{free(cells, mask)};
      crosses &= ~(naughts & crosses);
      if (crosses)
        mark(g, placed, i, crosses, true, transposed);
      if (naughts)
        mark(g, placed, i, naughts, false, transposed);
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

  static bool seperatedPairs(Game &g, placed_t &placed) {
    bool progress{false};
    const board_t boardMask{g.boardMask()};
    const auto freeLeft = [boardMask](const board_t cells, const board_t mask) {
      return ((cells & (cells << 2)) >> 1) & ~mask & boardMask;
    };
    const auto freeRight = [boardMask](const board_t cells,
                                       const board_t mask) {
      return ((cells & (cells << 2)) >> 1) & ~mask & boardMask;
    };
    const auto free = [&freeLeft, &freeRight](const board_t cells,
                                              const board_t mask) {
      return freeLeft(cells, mask) | freeRight(cells, mask);
    };
    const auto check = [&g, &placed, &free](const int i, const board_t cells,
                                            const board_t mask,
                                            const bool transposed) {
      board_t crosses{free(~cells & mask, mask)};
      board_t naughts{free(cells, mask)};
      if (crosses)
        mark(g, placed, i, crosses, true, transposed);
      if (naughts)
        mark(g, placed, i, naughts, false, transposed);
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

  static bool fullLine(Game &g, placed_t &placed) {
    bool progress{false};
    const board_t boardMask{g.boardMask()};
    const auto check = [&g, &placed,
                        boardMask](const int i, const board_t cells,
                                   const board_t mask, const bool transposed) {
      int half = {static_cast<int>(std::ceil(g.size() / 2.0))};
      int nCrosses{std::popcount(cells)};
      int nNaughts{std::popcount(mask) - nCrosses};
      board_t crosses{(nNaughts == half) ? ~mask & boardMask : 0};
      board_t naughts{(nCrosses == half) ? ~mask & boardMask : 0};
      if (crosses)
        mark(g, placed, i, crosses, true, transposed);
      if (naughts)
        mark(g, placed, i, naughts, false, transposed);
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

  struct CellBlock {
    int n{};
    int lCell{};
    int rCell{};
    int crossEnds{};
    int naughtEnds{};
    int crosses{};
    int naughts{};

    explicit CellBlock(int pn, int plCell, int prCell)
        : n{pn}, lCell{plCell}, rCell{prCell} {
      crossEnds = (lCell == 1 ? 1 : 0) + (rCell == 1 ? 1 : 0);
      naughtEnds = (lCell == 0 ? 1 : 0) + (rCell == 0 ? 1 : 0);
      crosses = (n + naughtEnds) / 3;
      naughts = (n + crossEnds) / 3;
    }
  };

  static board_t perfectMask(int n) {
    board_t mask{};
    for (int i{0}; i < n; ++i) {
      mask <<= 1;
      mask |= 1;
      mask <<= 2;
    }
    return mask;
  }

  static bool requiredCells(Game &g, placed_t &placed) {
    bool progress{false};
    const board_t boardMask{g.boardMask()};

    const auto check = [&g, &placed,
                        boardMask](const int i, const board_t cells,
                                   const board_t mask, const bool transposed) {
      if (g.isRowComplete(cells, mask))
        return false;
      int half = {static_cast<int>(std::ceil(g.size() / 2.0))};
      std::pair<int, int> count{std::popcount(cells),
                                std::popcount(~cells & mask)};
      board_t gaps{mask | ~boardMask};
      std::vector<std::pair<int, CellBlock>> blocks{};
      while (gaps) {
        int skip{std::countr_one(gaps)};
        if (skip > 0) {
          gaps >>= skip;
          continue;
        }
        int n{std::countr_zero(gaps)};
        int p{std::countl_zero(gaps)};
        CellBlock b{
            n, static_cast<int>(p == 0 ? -1 : (cells >> (p - 1)) & 1),
            static_cast<int>(p + 1 == g.size() ? -1 : (cells >> (p + n)) & 1)};
        count.first += b.crosses;
        count.second += b.naughts;

        blocks.push_back({p, b});
        gaps >>= n;
      }
      board_t crosses{};
      board_t naughts{};

      if (count.first == half) {
        bool placed{false};
        for (auto b : blocks) {
          if (b.second.crosses == 0) {
            placed = true;
            naughts |= ((board_t{1} << b.second.n) - 1) << b.first;
          }
        }
        if (!placed) {
          for (auto b : blocks) {
            board_t m{perfectMask(b.second.crosses)};
            if (b.second.crossEnds == 2 && (b.second.n + 1) % 3 == 0) {
              crosses |= m << b.first;
            } else if (b.second.crossEnds == 1 && (b.second.n + 2) % 3 == 0) {
              crosses |= (m >> (b.second.rCell == 1 ? 1 : 0)) << b.first;
            } else if (b.second.crossEnds == 1 && b.second.naughtEnds == 1 &&
                       b.second.n % 3 == 0) {
              crosses |= (m >> (b.second.lCell == 1 ? 0 : 2)) << b.first;
            }
          }
        }
      } else if (count.second == half) {
        bool placed{false};
        for (auto b : blocks) {
          if (b.second.naughts == 0) {
            placed = true;
            crosses |= ((board_t{1} << b.second.n) - 1) << b.first;
          }
        }
        if (!placed) {
          for (auto b : blocks) {
            board_t m{perfectMask(b.second.naughts)};
            if (b.second.naughtEnds == 2 && (b.second.n + 1) % 3 == 0) {
              naughts |= m << b.first;
            } else if (b.second.naughtEnds == 1 && (b.second.n + 2) % 3 == 0) {
              naughts |= (m >> (b.second.rCell == 0 ? 1 : 0)) << b.first;
            } else if (b.second.naughtEnds == 1 && b.second.crossEnds == 1 &&
                       b.second.n % 3 == 0) {
              naughts |= (m >> (b.second.lCell == 0 ? 0 : 2)) << b.first;
            }
          }
        }
      } else
        return false;

      if (crosses)
        mark(g, placed, i, crosses, true, transposed);
      if (naughts)
        mark(g, placed, i, naughts, false, transposed);
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

  static constexpr std::array rules{&adjacentPairs, &seperatedPairs, &fullLine,
                                    &requiredCells};
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
    if (!g.isValid())
      return false;
    if (g.isWon())
      return true;

    auto result{m_ds.deduce(g)};

    if (result.first == DeductiveSolver::Status::Success)
      return true;
    if (result.first == DeductiveSolver::Status::Stuck) {
      const board_t boardMask{g.boardMask()};
      std::pair<int, int> rMin{-1, g.size() + 1};
      for (int r{0}; r < g.size(); ++r) {
        int count{std::popcount(g.rowMask(r))};
        if (count == 0)
          continue;
        if (rMin.second > count)
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
      bool crosses{nCrosses <= rMin.second + cMin.second};
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
    }

    for (auto p : result.second) {
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

  Game g1{file};
  Game g2{g1};

  std::cout << g1 << '\n';

  // BacktrackSolver bs{};
  // std::cout << "BacktrackSolver: " << std::boolalpha << bs.solve(g1)
  //           << "\nCalls: " << bs.calls() << '\n'
  //           << g1 << '\n';

  DeductiveSolver ds{};
  ds.deduce(g1);
  std::cout << g1 << '\n';
  auto start = std::chrono::steady_clock::now();
  HybridSolver hs{};
  std::cout << "HybridSolver: " << std::boolalpha << hs.solve(g2)
            << "\nCalls: " << hs.calls() << '\n'
            << g2 << '\n';

  auto end = std::chrono::steady_clock::now();
  std::cout << "Time: " << std::fixed << std::setprecision(6)
            << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                       .count() /
                   1'000'000.0
            << "ms\n";

  return 0;
}