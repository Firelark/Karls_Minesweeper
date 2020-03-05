
#include "ScanlineSweep.h"
#include <algorithm>
#include <cmath>
#include <functional>


namespace kms
{
	enum class Cleared : uint8_t
	{
		False = 0,
		True
	};

	enum class LineFeed
	{
		undefiend,
		up,
		down
	};

	enum class ScanDirection
	{
		left,
		right
	};

	struct ScanLine
	{
		Pos2D start_position;
		unsigned magnitude = 0;
		LineFeed feed = LineFeed::undefiend;
	};

	void Report(const ScanLine& scan_line, unsigned board_height, std::function<void(const ScanLine&)> report_unhandled_scanline)
	{
		if (scan_line.feed == LineFeed::down || scan_line.feed == LineFeed::undefiend)
		{
			auto next_scan_line_down = scan_line;
			++next_scan_line_down.start_position.y;
			if (next_scan_line_down.start_position.y < board_height)
			{
				report_unhandled_scanline(next_scan_line_down);
			}
		}
		
		if (scan_line.feed == LineFeed::up || scan_line.feed == LineFeed::undefiend)
		{
			auto next_scan_line_up = scan_line;
			--next_scan_line_up.start_position.y;
			if (next_scan_line_up.start_position.y > 0)
			{
				report_unhandled_scanline(next_scan_line_up);
			}
		}
	}

	kms::ClearedRange Scan(const ScanLine& scan_line, const Size2D& board_size, const TilesVector_t& tiles, std::function<void(const ScanLine&)> report_unhandled_scanline)
	{
		if (Size(board_size) == 0 || tiles.size() > std::numeric_limits<unsigned>::max())
			throw(std::domain_error("Error: Board size is zero, or number of tiles are too big!"));

		const auto start_offset = GetOffsetIndex(board_size, scan_line.start_position);
		const auto begin_row_offset = GetOffsetIndex(board_size, {0, scan_line.start_position.y});
		const auto end_of_row_offset = begin_row_offset + board_size.width;
		const auto largest_index = Size(board_size) - 1;
		const auto reverse_start_offset = largest_index - start_offset;
		auto hot_to_cold = [](Tile_t first_tile_value, Tile_t second_tile_value) 
		{
			return first_tile_value != 0 && second_tile_value == 0;
		};

		auto cleared_range = ClearedRange{};

		auto start_value = tiles.at(start_offset);
		if (start_value == 0)
		{
			// scan to the left to find new start
			auto tiles_revit_begin = tiles.rbegin() + reverse_start_offset;
			auto tiles_revit_end = tiles_revit_begin + scan_line.magnitude;
			const auto new_start_offset = largest_index - static_cast<unsigned>(std::find_if_not(tiles_revit_begin, tiles_revit_end, 0) - tiles.rbegin());
			
			const auto delta = start_offset - new_start_offset;
			cleared_range.begin = new_start_offset;
			cleared_range.end = new_start_offset + delta + scan_line.magnitude;

			auto first_next_scan_line = scan_line;
			first_next_scan_line.feed == LineFeed::undefiend;
			first_next_scan_line.start_position.x -= delta;
			first_next_scan_line.magnitude = delta;
			Report(first_next_scan_line, board_size.height, report_unhandled_scanline);
		}

		auto next_scan_line = scan_line;
		// scan to the right over original magnitude 
		const auto it_original_start_of_scan_line = tiles.begin() + start_offset;
		auto it_curr_tile = it_original_start_of_scan_line;
		const auto it_end_of_scan_line = it_curr_tile + scan_line.magnitude;
		while (it_curr_tile != it_end_of_scan_line)
		{
			// for any pair of hot tiles (starting from next_scan_line.start_position) report a next scan line

			// adjust magnitude
			it_curr_tile = std::find_if_not(it_curr_tile, it_end_of_scan_line, 0);
			next_scan_line.magnitude -= static_cast<unsigned>(it_end_of_scan_line - it_curr_tile);

			if (it_curr_tile == it_end_of_scan_line)
				break;

			// report the next scan line
			Report(next_scan_line, board_size.height, report_unhandled_scanline);
			
			// NEW SCAN LINE: new start postion and initial magnitude of the next scan line 
			it_curr_tile = std::adjacent_find(it_curr_tile, it_end_of_scan_line, hot_to_cold);
			next_scan_line.start_position.x = static_cast<unsigned>(it_curr_tile - (tiles.begin() + begin_row_offset));
			next_scan_line.magnitude = static_cast<unsigned>(it_end_of_scan_line - it_curr_tile);
		}


		// find the next hot tile to the right 
		const auto it_end_of_row = tiles.begin() + end_of_row_offset;

		// if not at the end, and the last tile on the right of the scan line is not hot (not zero)
		if (it_curr_tile != it_end_of_row && *it_curr_tile == 0)
		{
			next_scan_line.feed == LineFeed::undefiend;

			const auto it_begin_of_last_scan_line = it_curr_tile;
			// expand the scanline to the right (until end or next hot tile)
			it_curr_tile = std::find_if_not(it_curr_tile, it_end_of_row, 0);

			const auto added_magnitude = static_cast<unsigned>(it_curr_tile - it_begin_of_last_scan_line); 
			// adjust the magnitude to include the expanded range
			next_scan_line.magnitude += added_magnitude;

			Report(next_scan_line, board_size.height, report_unhandled_scanline);

			cleared_range.end += added_magnitude;
		}

		return cleared_range;
	}


	void test2()
	{
		Size2D board_size;
		unsigned starting_offset;
		unsigned beg_of_line_offset;
		unsigned end_of_line_offset;

		std::vector<ScanLine> unhandled_scanlines;

		ScanLine right = ScanRight(starting_offset, board_size, tiles);
		Scanline left = ScanLeft(starting_offset, board_size, tiles);
	
		Scanline curr_line = Concatenate(left, right);

		if (curr_line.feed == LineFeed::undefiend)
		{
			auto tmp = curr_line;
			tmp.feed = LineFeed::up;
			unhandled_scanlines.push_back(tmp);
			tmp.feed = LineFeed::down;
			unhandled_scanlines.push_back(tmp);
		}
		

		

	}











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

