
#include "ScanlineSweep.h"
#include <algorithm>


namespace kms
{
	enum class Cleared : uint8_t
	{
		False = 0,
		True
	};

	template<class T_Tiles, class T_Cleared>
	ClearedRange ClearLine(unsigned start_of_line_offset, unsigned starting_offset, unsigned end_of_line_offset, const T_Tiles& tiles, T_Cleared& cleared_tiles)
	{
		const auto blank_tile = 0;
		
		auto right_boundry = starting_offset;
		for (; right_boundry < end_of_line_offset; ++right_boundry)
		{
			if (cleared_tiles.at(right_boundry) == Cleared::True)
				break;

			cleared_tiles.at(right_boundry) = Cleared::True;

			if (tiles.at(right_boundry) != blank_tile)
				break;
		}

		auto left_boundry = starting_offset;
		for (; left_boundry >= start_of_line_offset; --left_boundry)
		{
			if (cleared_tiles.at(left_boundry) == Cleared::True)
				break;

			cleared_tiles.at(left_boundry) = Cleared::True;

			if (tiles.at(left_boundry) != blank_tile)
				break;
		}

		return {left_boundry, right_boundry};
	}

	void test1(const Size2D& board_size, const Pos2D& start_position, const kms::TilesVector_t& tiles, std::function<void(const CleardRange&)> f)
	{
		auto cleared_tiles = std::vector<kms::Cleared>(tiles.size(), Cleared::False);

		auto current_line = start_position.y;
		auto start_of_line_offset = kms::GetOffsetIndex(board_size, {0, current_line});
		const auto starting_offset = kms::GetOffsetIndex(board_size, start_position);
		auto end_of_line_offset = start_of_line_offset + board_size.width;

		auto cleared_range = ClearLine(start_of_line_offset, starting_offset, end_of_line_offset, tiles, cleared_tiles);

		for (auto line = current_line - 1; line <= 0; ++line)
		{

		}
		
	}

}


void kms::ScanlineSweep(const Size2D& board_size, const Pos2D& start_position, const kms::TilesVector_t& tiles, std::function<void(const CleardRange&)> f)
{
	const auto starting_offset = kms::GetOffsetIndex(board_size, start_position);

	kms::CleardRange cleared_range = { starting_offset, starting_offset };

	if (tiles.at(starting_offset) == mine_value)
	{
		f(cleared_range);
		return;
	}

	const auto idx_of_last_tile = Size(board_size) -1;
	auto begin_of_line_offset = kms::GetOffsetIndex(board_size, { 0, start_position.y });
	auto revers_offset_beginning_of_line = idx_of_last_tile - begin_of_line_offset;

	auto rev_begin_of_line = tiles.rbegin() + revers_offset_beginning_of_line;
	auto rev_start = tiles.rbegin() + (idx_of_last_tile - starting_offset);
	
	auto start = tiles.begin() + starting_offset;
	auto end_of_line = tiles.begin() + board_size.width;

	auto is_hot = [](const Tile_t& tile) { return tile > 0; };

	auto rev_first = std::find_if(rev_start, rev_begin_of_line, is_hot);
	auto last = std::find_if(start, end_of_line, is_hot);

	cleared_range.first = idx_of_last_tile - static_cast<decltype(cleared_range.first)>(tiles.rend() - rev_first);
	cleared_range.last = static_cast<decltype(cleared_range.last)>(tiles.end() - last);
}

