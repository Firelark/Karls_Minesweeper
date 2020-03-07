// Console_TestApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <limits>
#include <exception>
#include <algorithm>
#include <cstdlib>
#include <ctime>

#include "Minesweep_Basics.h"
#include "ScanlineSweep.h"

namespace kms
{
    int ReportNeighbouringMine(int value)
    {
        if (value < 0)
            return value;

        return value + 1;
    }

    template<class T_Itr>
    void ReportMineOnNearbyRow(T_Itr nearest, bool left, bool right)
    {
        *nearest = ReportNeighbouringMine(*nearest);

        if (left)
            *(nearest - 1) = ReportNeighbouringMine(*(nearest - 1));

        if (right)
            *(nearest + 1) = ReportNeighbouringMine(*(nearest + 1));
    }

    template<class T_Itr>
    void ReportMineOnCurrentRow(T_Itr curr, bool left, bool right)
    {
        *curr = mine_value;

        if (left)
            *(curr - 1) = ReportNeighbouringMine(*(curr - 1));

        if (right)
            *(curr + 1) = ReportNeighbouringMine(*(curr + 1));
    }

    std::vector<int> PlaceMines(const Size2D& board_size, unsigned int coverage)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        const float percent_coef = 100.f / static_cast<float>(RAND_MAX);

        std::vector<int> tiles(Size(board_size), 0);

        for (auto y = 0u; y < board_size.height; ++y)
        {
            const bool top_neighbour = y > 0;
            const bool bottom_neighbour = y < (board_size.height - 1);

            for (auto x = 0u; x < board_size.width; ++x)
            {
                const bool left_neighbour = x > 0;
                const bool right_neighbour = x < (board_size.width - 1);

                const auto curr_tile_offset = y * board_size.width + x;

                // determine whether to place a mine
                auto rand = std::rand();
                auto percent = rand * percent_coef;
                bool place_mine = percent < coverage;

                if (place_mine)
                {
                    auto itr_curr_tile = tiles.begin() + curr_tile_offset;

                    ReportMineOnCurrentRow(itr_curr_tile, left_neighbour, right_neighbour);

                    if (top_neighbour)
                        ReportMineOnNearbyRow(itr_curr_tile - board_size.width, left_neighbour, right_neighbour);

                    if (bottom_neighbour)
                        ReportMineOnNearbyRow(itr_curr_tile + board_size.width, left_neighbour, right_neighbour);
                }
            }
        }

        return tiles;
    }


    void PrintTile(int value, bool visited)
    {
        if (value == 0 && visited)
            std::cout << "[x]";
        else if (value == 0)
            std::cout << "[ ]";
        else if (value == mine_value)
            std::cout << "{*}";
        else if (value < 9)
            std::cout << '[' << value << ']';
        else
            std::cout << "[E]";
    }

    template <class T_itr>
    void PrintBoard(const Size2D& board_size, T_itr begin, T_itr end)
    {
        int row = 0;
        int column = 0;
        std::for_each(begin, end, [&](auto const& v) {
            PrintTile(v, false);

            if (++column == board_size.width)
            {
                std::cout << '\n';
                ++row;
                column = 0;
            }
            });
    }

    template <class T_tiles_itr, class T_visited_idx_itr>
    void PrintBoard_VisitedTiles(const Size2D& board_size, T_tiles_itr tiles_begin, T_tiles_itr tiles_end, T_visited_idx_itr visidx_begin, T_visited_idx_itr visidx_end)
    {
        int row = 0;
        int col = 0;
        uint16_t idx = 0;
        std::for_each(tiles_begin, tiles_end, [&](int val) {
            if (*(visidx_begin + idx) == 1)
                PrintTile(val, true);
            else
                std::cout << "[ ]";

            ++idx;

            if (++col == board_size.width)
            {
                std::cout << '\n';
                ++row;
                col = 0;
            }
            });
    }

    int StepOnTile(const Size2D& board_size, const Pos2D& pos, const TilesVector_t& tiles)
    {
        auto offset = GetOffsetIndex(board_size, pos);

        return tiles.at(offset);
    }

    Pos2D GetPosition(Size2D limits)
    {
        Pos2D pos;

        bool valid = true;

        do
        {
            std::cout << "Enter x position (0-15): ";
            std::cin >> pos.x;
            std::cout << "Enter y position (0-15): ";
            std::cin >> pos.y;

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<int>::max(), '\n');

            valid = pos.x <= limits.width - 1 && pos.y <= limits.height - 1;
            if (!valid)
                std::cout << "Invalid input\n";

        } while (!valid);

        return pos;
    }

    void Play(const Size2D& board_size, const kms::TilesVector_t& tiles_data)
    {
        using tile_state_t = uint16_t;

        if (tiles_data.size() > std::numeric_limits<tile_state_t>::max())
            throw(std::domain_error("Too many tiles!"));

		auto tiles_status = std::vector <int>(tiles_data.size(), 0);
		auto fn_clear_tile = [&](const Pos2D& position) {
			auto offset = GetOffsetIndex(board_size, position);
			if (tiles_status.at(offset) == 0)
			{
				tiles_status.at(offset) = 1;
				return true;
			}
			return false;
		};

        auto fn_get_tile_data = [&](const Pos2D& position) {
            auto offset = GetOffsetIndex(board_size, position);
            return tiles_data.at(offset);
        };


        system("cls");
        std::cout << "Player Board:\n";
        PrintBoard_VisitedTiles(board_size, tiles_data.begin(), tiles_data.end(), tiles_status.begin(), tiles_status.end());
        std::cout << "\n------------------------------------\n";
        std::cout << "Board behind the tiles (for debuggning!)\n";
        PrintBoard(board_size, tiles_data.begin(), tiles_data.end());

        bool game_over = false;

        while (!game_over)
        {
            auto position = GetPosition(board_size);

            ScanlineSweep(board_size, position, fn_get_tile_data, fn_clear_tile);

            auto tile_value = StepOnTile(board_size, position, tiles_data);

            if (tile_value == mine_value)
            {
                game_over = true;
                std::cout << "Game Over!\n";
                PrintBoard(board_size, tiles_data.begin(), tiles_data.end());
                return;
            }

            system("cls");
            std::cout << "Player Board:\n";
            PrintBoard_VisitedTiles(board_size, tiles_data.begin(), tiles_data.end(), tiles_status.begin(), tiles_status.end());
            std::cout << "\n------------------------------------\n";
            std::cout << "Board behind the tiles (for debuggning!)\n";
            PrintBoard(board_size, tiles_data.begin(), tiles_data.end());
        }
    }
}

int main()
{
    kms::Size2D board_size = {16, 16};

    const auto tiles = PlaceMines(board_size, 10);

    Play(board_size, tiles);
}


