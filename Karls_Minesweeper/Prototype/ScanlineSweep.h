#pragma once
#ifndef SCANLINESWEEP_H_
#define SCANLINESWEEP_H_

#include <cstdint>
#include <functional>
#include "Minesweep_Basics.h"

namespace kms
{
	// Starting from a start position that has a zero value, sweep all the connected tiles that have a value of zero, and stop at either a border or a number (greater than zero)
	// For each cleared line call the provieded function object and pass the cleared range to it
	void ScanlineSweep(const Size2D& board_size, const Pos2D& start_position, const TilesVector_t& tiles, std::function<bool(unsigned)> fn_is_cleared, std::function<void(const ClearedRange&)> fn_report_clear_range);
}

#endif // !SCANLINEFILL_H_

