
#include "ScanlineSweep.h"
#include <algorithm>
#include <cmath>
#include <functional>


namespace kms
{
	enum class ELineFeed
	{
		undefiend,
		up,
		down
	};

	ELineFeed OppositeFeedDirection(ELineFeed feed)
	{
		switch (feed)
		{
		case kms::ELineFeed::up:
			return ELineFeed::down;
		case kms::ELineFeed::down:
			return ELineFeed::up;
		}
		return ELineFeed::undefiend;
	}

	enum class ETileStatus
	{
		not_cleared,
		cleared
	};

	struct ScanLine
	{
		Pos2D start_position;
		unsigned magnitude = 0;
		ELineFeed feed = ELineFeed::undefiend;
	};

	ScanLine CreateScanLine(const Pos2D& start_position, unsigned magnitude, ELineFeed feed, const Size2D& board_size)
	{
		if (start_position.x >= board_size.width || start_position.y >= board_size.height || start_position.x + magnitude > board_size.width)
			throw(std::logic_error("Not on board"));

		ScanLine scan_line;
		scan_line.start_position = start_position;
		scan_line.magnitude = magnitude;
		scan_line.feed = feed;
		return scan_line;
	}

	bool Intersect(const ScanLine& a, const ScanLine& b)
	{
		const auto ax1 = a.start_position.x;
		const auto ax2 = a.start_position.x + a.magnitude;
		const auto bx1 = b.start_position.x;
		const auto bx2 = b.start_position.x + b.magnitude;

		return a.start_position.y == b.start_position.y &&
			((ax1 <= bx1 && bx1 <= ax2) || (ax1 <= bx2 && bx2 <= ax1) || (ax1 <= bx1 && bx2 >= ax2) || (bx1 <= ax1 && ax2 >= bx2));
	}

	void CacheScanline(const ScanLine& scan_line, const Size2D& board_size, std::function<void(const ScanLine&)> fn_cache_scanline)
	{
		if (scan_line.start_position.x >= board_size.width || scan_line.start_position.y >= board_size.height || scan_line.start_position.x + scan_line.magnitude > board_size.width)
			throw(std::logic_error("Invalid Position!"));

		fn_cache_scanline(scan_line);
	}
	
	void CacheScanLine_NextRow(const Pos2D& curr_position, unsigned magnitude, ELineFeed curr_feed, const Size2D& board_size, std::function<void(const ScanLine&)> fn_cache)
	{
		auto create_scanline = [&](const Pos2D& start_position, unsigned magnitude, ELineFeed line_feed) { return CreateScanLine(start_position, magnitude, line_feed, board_size); };

		if ((curr_feed == ELineFeed::up || curr_feed == ELineFeed::undefiend) && curr_position.y > 0)
		{
			auto next_start_position = Pos2D{ curr_position.x, curr_position.y - 1 };
			auto next_scanline = create_scanline(next_start_position, magnitude, ELineFeed::up);
			fn_cache(next_scanline);
		}
		
		if ((curr_feed == ELineFeed::down || curr_feed == ELineFeed::undefiend) && curr_position.y < board_size.height -1)
		{
			auto next_start_position = Pos2D{curr_position.x, curr_position.y + 1 };
			auto next_scanline = create_scanline(next_start_position, magnitude, ELineFeed::down);
			fn_cache(next_scanline);
		}	
	}

	void CacheScanLine_NextRow_ReverseFeed(const Pos2D& curr_position, unsigned magnitude, ELineFeed curr_feed, const Size2D& board_size, std::function<void(const ScanLine&)> fn_cache)
	{
		CacheScanLine_NextRow(curr_position, magnitude, OppositeFeedDirection(curr_feed), board_size, fn_cache);
	}

	void CacheScanLine_NextRow(const ScanLine& curr_scanline, const Size2D& board_size, std::function<void(const ScanLine&)> fn_cache)
	{
		CacheScanLine_NextRow(curr_scanline.start_position, curr_scanline.magnitude, curr_scanline.feed, board_size, fn_cache);
	}


	// Adjust the start of the scanline to hit the next hot tile, or first tile of the row to the right of the start_position 
	// cache any scanline that results from a possible expansion to the left
	ScanLine AdjustScanlineStart(const ScanLine& scanline, const Size2D& board_size, 
		std::function<int(const Pos2D&)> fn_get_tile_data, 
		std::function<void(const ScanLine&)> fn_cache)
	{
		auto is_tile_hot_at = [&](const Pos2D& position) { return fn_get_tile_data(position); };

		// start position is not at the start of the row and the tile value is cold tile (zero value)
		// expand scanline to the left until it hits (and includes) the next hot tile (non-zero value) 
		// or until the beginning of the row is hit.

		// also record the next_scanline starting at the new start position and have a magnitude equal
		// to the old_start_position - new_start_position.


		if (scanline.magnitude > 1 && scanline.feed != ELineFeed::undefiend)
		{
			bool shrink = false;
			shrink = is_tile_hot_at(Position2D(scanline.start_position.x + 1, scanline.start_position.y));
			if (scanline.start_position.x != 0 && scanline.start_position.x < board_size.width)
			{
				if (scanline.feed == ELineFeed::up)
					shrink = shrink && is_tile_hot_at(Position2D(scanline.start_position.x, scanline.start_position.y -1));
				else if (scanline.feed == ELineFeed::up)
					shrink = shrink && is_tile_hot_at(Position2D(scanline.start_position.x, scanline.start_position.y +1));
			}

			if (shrink)
			{
				auto shrunk_scanline = scanline;
				shrunk_scanline.start_position.x += 1;
				shrunk_scanline.magnitude - 1;
				return shrunk_scanline;
			}
		}


		auto adjusted_scanline = scanline;
		auto extension_magnitude = decltype(scanline.magnitude){0};
		for (; adjusted_scanline.start_position.x > 0; --adjusted_scanline.start_position.x)
		{
			// if tile value is hot
			if (is_tile_hot_at(adjusted_scanline.start_position))
				break;
		}

		extension_magnitude = scanline.start_position.x - adjusted_scanline.start_position.x;
		
		if (extension_magnitude && adjusted_scanline.feed != ELineFeed::undefiend)
			CacheScanLine_NextRow_ReverseFeed(adjusted_scanline.start_position, extension_magnitude, adjusted_scanline.feed, board_size, fn_cache);

		adjusted_scanline.magnitude += extension_magnitude;
		return adjusted_scanline;
	}

	// Adjust the scanlines magnitude possibly extending it to the right cache any scanlines that may result from this
	ScanLine AdjustScanlineMagnitude(const ScanLine& scanline, const Size2D& board_size,
			std::function<int(const Pos2D&)> fn_get_tile_data,
			std::function<void(const ScanLine&)> fn_cache)
	{
		auto is_tile_hot_at = [&](const Pos2D& position) { return fn_get_tile_data(position); };

		auto pos_last_tile_of_scanline = Position2D(scanline.start_position.x + scanline.magnitude - 1, scanline.start_position.y);

		// check if the scanline magnitude should shrink
		if (scanline.magnitude > 1 && scanline.feed != ELineFeed::undefiend)
		{
			bool shrink = false;
			const auto xpos_at_last_tile_of_row = board_size.width -1;
			const auto xpos_last_tile_of_scanline = scanline.start_position.x + scanline.magnitude - 1;
			
			//if (is_tile_hot_at(Position2D(xpos_last_tile_of_scanline, scanline.start_position.y)) == false)
			//{
				shrink = is_tile_hot_at(Position2D(xpos_last_tile_of_scanline -1, scanline.start_position.y));

				// if the last tile of the scanline is not the last tile of the row
				if (xpos_last_tile_of_scanline == xpos_at_last_tile_of_row)
				{
					// also check the last tile on the previous row
					if (scanline.feed == ELineFeed::up)
						shrink = shrink && is_tile_hot_at(Position2D(xpos_last_tile_of_scanline, scanline.start_position.y - 1));
					else if (scanline.feed == ELineFeed::down)
						shrink = shrink && is_tile_hot_at(Position2D(xpos_last_tile_of_scanline, scanline.start_position.y + 1));
				}
			//}
			
			if (shrink)
			{
				auto shrunk_scanline = scanline;
				shrunk_scanline.magnitude -= 1; // shrink by one
				return shrunk_scanline;
			}
		}

		auto adjusted_scanline = scanline;
		auto extension_magnitude = decltype(scanline.magnitude){0};
		const auto beginning_of_extension = adjusted_scanline.start_position.x + adjusted_scanline.magnitude;
		auto curr_position = adjusted_scanline.start_position;
		curr_position.x = beginning_of_extension;
		for (; curr_position.x < board_size.width; ++extension_magnitude, ++curr_position.x)
		{
			if (is_tile_hot_at(curr_position))
			{
				++extension_magnitude;
				break;
			}
		}

		if (extension_magnitude && adjusted_scanline.feed != ELineFeed::undefiend)
		{
			auto position_first_tile_of_extension = adjusted_scanline.start_position;
			position_first_tile_of_extension.x = beginning_of_extension;
			CacheScanLine_NextRow_ReverseFeed(position_first_tile_of_extension, extension_magnitude, adjusted_scanline.feed, board_size, fn_cache);
		}

		adjusted_scanline.magnitude += extension_magnitude;
		return adjusted_scanline;
	}

	bool IsMagnitudeAdjustible(const ScanLine& scanline, const Size2D& board_size, std::function<bool(Pos2D)> fn_get_tile_data)
	{
		auto is_not_last_tile_in_row = [&]() { return scanline.start_position.x + scanline.magnitude - 1 < board_size.width; };
		auto is_last_tile_blank = [&]() { return fn_get_tile_data({ scanline.start_position.x + scanline.magnitude - 1, scanline.start_position.y }) == 0;  };

		return scanline.magnitude != 0 && is_not_last_tile_in_row() && is_last_tile_blank();
	}

	void SweepOneScanLine(const ScanLine& scanline,
		const Size2D& board_size,
		std::function<int(Pos2D)> fn_get_tile_data,
		std::function<bool(const Pos2D&)> fn_clear_tile_at,
		std::function<void(const ScanLine&)> fn_cache_scanline)
	{
		// Simplifying function calls for better readability
		auto cache = [&](const ScanLine& scan_line) { CacheScanline(scan_line, board_size, fn_cache_scanline); };
		auto create_scanline = [&](const Pos2D& start_position, unsigned magnitude, ELineFeed line_feed) { return CreateScanLine(start_position, magnitude, line_feed, board_size); };
		auto is_start_adjustible = [&](const ScanLine& scanline) { return fn_get_tile_data(scanline.start_position) == 0 && scanline.start_position.x != 0; };
		auto is_magnitude_adjustible = [&](const ScanLine& scanline) { return IsMagnitudeAdjustible(scanline, board_size, fn_get_tile_data); };
		auto is_hot = [&](const Pos2D& position) { return fn_get_tile_data(position) != 0; };
	

		// ADJUST THE START OF THE SCANLINE
		// check the value of the starting tile,

		// if the value is zero (not hot) then
		// scan left until the first hot tile
		// put the new start at the scanline at
		// that tile.

		// also put cleared_range to start at the
		// same tile

		auto adjusted_scanline = scanline;
	
		// Adjust start
		if (is_start_adjustible(scanline))
			adjusted_scanline = AdjustScanlineStart(adjusted_scanline, board_size, fn_get_tile_data, cache);

		// Adjust magnitude
		if (is_magnitude_adjustible(adjusted_scanline))
			adjusted_scanline = AdjustScanlineMagnitude(adjusted_scanline, board_size, fn_get_tile_data, cache);

		bool recording = true;
		bool reset = false;
		auto next_scanline = adjusted_scanline;
		next_scanline.magnitude = 0;
		const auto xend_of_scanline = adjusted_scanline.start_position.x + adjusted_scanline.magnitude;

		// Scan the the scanline
		for (auto curr_position = adjusted_scanline.start_position; curr_position.x < xend_of_scanline; ++curr_position.x, ++next_scanline.magnitude)
		{
			// clear the tile at the curren position, if this function return false the tile has already been cleared
			// so the next scanline should be aborted
			if (fn_clear_tile_at(curr_position))
			{
				if (reset)
				{
					next_scanline.start_position = curr_position;
					next_scanline.magnitude = 1;
					reset = false;
				}

				if (is_hot(curr_position) || curr_position.x == board_size.width -1)
				{ 
					if (recording)
					{
						if(next_scanline.magnitude) // not allowing
						{ 
							++next_scanline.magnitude; // one beyond the hot tile
							CacheScanLine_NextRow(next_scanline, board_size, cache);
						}
						reset = false;
						recording = false;
					}

					next_scanline.start_position = curr_position;
					next_scanline.magnitude = 0;
				}
				else if(reset == false && recording == false)
				{
					recording = true;
				}
			}
			else
			{
				recording = false;
				reset = true;
			}
		}
	}

	void ScanlineSweep(const Size2D& board_size, const Pos2D& start_position,
		std::function<int(Pos2D)> fn_get_tile_data, std::function<bool(Pos2D)> fn_clear_tile)
	{
		auto unhandled_scanlines = std::vector <ScanLine>{};
		auto fn_cache_scanline = [&](const ScanLine& scanline) { unhandled_scanlines.push_back(scanline); };

		// create the first scanline to start with
		auto starting_scanline = ScanLine{};
		starting_scanline.start_position = start_position;
		starting_scanline.magnitude = 1; // one tile

		// simplify calling to sweep
		auto sweep = [&](const ScanLine& scanline) { SweepOneScanLine(scanline, board_size, fn_get_tile_data, fn_clear_tile, fn_cache_scanline); };

		// sweep the first line
		sweep(starting_scanline);

		while (!unhandled_scanlines.empty())
		{
			const auto curr_scanline = unhandled_scanlines.back();
			unhandled_scanlines.pop_back();
			sweep(curr_scanline);
		}
	}
}

