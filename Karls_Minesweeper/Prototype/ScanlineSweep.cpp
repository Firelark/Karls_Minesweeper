
#include "ScanlineSweep.h"
#include <algorithm>
#include <cmath>
#include <functional>


namespace kms
{
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

	bool Intersect(const ScanLine& a, const ScanLine& b)
	{
		const auto ax1 = a.start_position.x;
		const auto ax2 = a.start_position.x + a.magnitude;
		const auto bx1 = b.start_position.x;
		const auto bx2 = b.start_position.x + b.magnitude;

		return a.start_position.y == b.start_position.y && ((ax1 <= bx1 && bx1 <= ax2) || (ax1 <= bx2 && bx2 <= ax1) || (ax1 <= bx1 && bx2 >= ax2) || (bx1 <= ax1 && ax2 >= bx2));
	}

	void Report(const ScanLine& scan_line, unsigned board_height, std::function<void(const ScanLine&)> report_unhandled_scanline)
	{
		if (scan_line.magnitude == 0)
			return;

		if (scan_line.feed == LineFeed::down || scan_line.feed == LineFeed::undefiend)
		{
			auto next_scan_line_down = scan_line;
			next_scan_line_down.feed = LineFeed::down;
			++next_scan_line_down.start_position.y;
			if (next_scan_line_down.start_position.y < board_height)
			{
				report_unhandled_scanline(next_scan_line_down);
			}
		}
		
		if (scan_line.feed == LineFeed::up || scan_line.feed == LineFeed::undefiend)
		{
			auto next_scan_line_up = scan_line;
			next_scan_line_up.feed = LineFeed::up;
			if (next_scan_line_up.start_position.y != 0)
			{
				--next_scan_line_up.start_position.y;
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

		auto hot = [](Tile_t value)
		{
			return value != 0;
		};

		auto cleared_range = ClearedRange{start_offset, start_offset + scan_line.magnitude};

		auto next_scan_line = scan_line;

		auto start_value = tiles.at(start_offset);
		if (scan_line.start_position.x != 0 && start_value == 0)
		{
			// scan to the left of the startposition to find new start position
			auto revit_tiles_begin = tiles.rbegin() + reverse_start_offset;
			auto revit_tiles_end = tiles.rbegin() + largest_index - begin_row_offset;
			const auto rev_offset_of_start = static_cast<unsigned>(std::find_if(revit_tiles_begin, revit_tiles_end, hot) - tiles.rbegin());
			const auto new_start_offset = largest_index - rev_offset_of_start;
			
			const auto delta = start_offset - new_start_offset;
			cleared_range.begin = new_start_offset;
			cleared_range.end = new_start_offset + delta + scan_line.magnitude;

			if (scan_line.feed != LineFeed::undefiend)
			{
				// if line feed of the current scan_line is defined
				// then create a new separate next scan line for the 
				// scan line to the left of the starting position
				auto first_next_scan_line = scan_line;
				first_next_scan_line.feed = LineFeed::undefiend;
				first_next_scan_line.start_position.x -= delta;
				first_next_scan_line.magnitude = delta;

				// report the left scan line separatly
				Report(first_next_scan_line, board_size.height, report_unhandled_scanline);
			}
			else 
			{
				// if line feed is undefined then the
				// scan line to the left of the starting position 
				// can be extended main "next_scan_line" and reported
				// later.
				next_scan_line.start_position.x -= delta;
				next_scan_line.magnitude += delta;
			}
		}

		// scan to the right of the start position until the magnitude 
		const auto it_original_start_of_scan_line = tiles.begin() + start_offset;
		auto it_curr_tile = it_original_start_of_scan_line;
		const auto it_end_of_scan_line = it_curr_tile + scan_line.magnitude;
		while (it_curr_tile != it_end_of_scan_line)
		{
			// for any pair of hot tiles (starting from next_scan_line.start_position) report a next scan line

			// adjust magnitude
			// find the first "hot" tile (e.i. not zero)
			it_curr_tile = std::find_if(++it_curr_tile, it_end_of_scan_line, hot);
			//if (it_curr_tile != it_end_of_scan_line)
			//	++it_curr_tile;
			auto delta = static_cast<unsigned>(it_end_of_scan_line - it_curr_tile);

			next_scan_line.magnitude -= delta;

			if (it_curr_tile == it_end_of_scan_line && next_scan_line.feed == LineFeed::undefiend)
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
		if (it_curr_tile != it_end_of_row)
		{

			const auto it_begin_of_current_scan_line = it_curr_tile;
			// expand the scanline to the right (until end or next hot tile)
			it_curr_tile = std::find_if(++it_curr_tile, it_end_of_row, hot);
			//if (it_curr_tile != it_end_of_row)
			//	++it_curr_tile;

			const auto added_magnitude = static_cast<unsigned>(it_curr_tile - it_begin_of_current_scan_line);

			if (next_scan_line.feed != LineFeed::undefiend)
			{
				Report(next_scan_line, board_size.height, report_unhandled_scanline);

				next_scan_line.start_position.x = static_cast<unsigned>(it_begin_of_current_scan_line - (tiles.begin() + begin_row_offset));
				next_scan_line.magnitude = added_magnitude;
			}
			else
			{
				// adjust the magnitude to include the expanded range
				next_scan_line.magnitude += added_magnitude;
			}

			next_scan_line.feed = LineFeed::undefiend;
			Report(next_scan_line, board_size.height, report_unhandled_scanline);

			cleared_range.end += added_magnitude;
		}

		return cleared_range;
	}
}


void kms::ScanlineSweep(const Size2D& board_size, const Pos2D& start_position, const kms::TilesVector_t& tiles, std::function<bool(unsigned)> fn_is_cleared, std::function<void(const ClearedRange&)> fn_report_clear_range)
{
	const auto start_offset = GetOffsetIndex(board_size, start_position);
	const auto beg_of_line_offset = GetOffsetIndex(board_size, {0, start_position.y});
	const auto end_of_line_offset = beg_of_line_offset + board_size.width;

	std::vector<ScanLine> unhandled_scanlines;
	auto fn_report_scan_line = [&](const ScanLine& reported_scan_line) {

		for (auto a : unhandled_scanlines)
			if (Intersect(a, reported_scan_line))
				return;
		unhandled_scanlines.push_back(reported_scan_line); 
	};

	// initial scan line
	auto init_scan_line = ScanLine{ start_position, 1, LineFeed::undefiend };


	auto clear_range = Scan(init_scan_line, board_size, tiles, fn_report_scan_line);

	fn_report_clear_range(clear_range);

	while (!unhandled_scanlines.empty())
	{
		const auto scan_line = unhandled_scanlines.back();
		unhandled_scanlines.pop_back();
		if(!fn_is_cleared(GetOffsetIndex(board_size, scan_line.start_position)))
		{ 
			clear_range = Scan(scan_line, board_size, tiles, fn_report_scan_line);
			fn_report_clear_range(clear_range);
		}
	}
}

