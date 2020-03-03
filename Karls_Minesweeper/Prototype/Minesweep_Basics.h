#pragma once
#ifndef MINESWEEP_BASICS
#define MINESWEEP_BASICS

#include <stdexcept>
#include <vector>

namespace kms
{
	extern const int mine_value;

	struct Size2D
	{
		unsigned width = 0;
		unsigned height = 0;
	};

	inline unsigned Size(const Size2D& board_size)
	{
		return board_size.width * board_size.height;
	}

	struct Pos2D
	{
		unsigned x = 0;
		unsigned y = 0;
	};
    
	inline unsigned GetOffsetIndex(const Size2D& board_size, const Pos2D& position)
    {
        auto offset = position.y * board_size.width + position.x;

        if (offset >= Size(board_size))
            throw(std::out_of_range("Not on board!"));

        return offset;
    }
	
	struct CleardRange
	{
		unsigned first = 0;
		unsigned last = 0;
	};

	using Tile_t = int;
	using TilesVector_t = std::vector<Tile_t>;
}

#endif // !MINESWEEP_BASICS

