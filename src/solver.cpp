#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>

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
    return !(row & row >> 1 & row >> 2) || std::popcount(row) > m_size / 2;
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

  void place(int r, int c, bool cross) {
    if (!canPlace(r, c))
      throw std::invalid_argument("game.place: invalid position");
    board_t cMask{column(c)};
    board_t cMaskT{column(r)};
    m_mask[r] |= cMask;
    m_maskT[c] |= cMaskT;
    if (cross) {
      m_cells[r] |= cMask;
      m_cells[c] |= cMaskT;
    }
  }

  void unPlace(int r, int c) {
    if (canPlace(r, c))
      throw std::invalid_argument("game.unplace: invalid position");
    board_t cMask{!column(c)};
    board_t cMaskT{!column(r)};
    m_mask[r] &= cMask;
    m_maskT[c] &= cMaskT;
    m_cells[r] &= cMask;
    m_cells[c] &= cMaskT;
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
  std::cout << g << '\n';
  return 0;
}