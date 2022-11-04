#pragma once

#include <cstdint>
#include <tuple>
using namespace std;

class CoordinatesManager
{
public:
    CoordinatesManager(uint64_t grid_width, uint64_t grid_height, uint64_t addr_width)
        :m_grid_width(grid_width)
        , m_grid_height(grid_height)
        , m_addr_width(addr_width)
        ,m_addr_height(grid_height)
    {
        m_canvas_width = 3000;
        m_canvas_height = m_grid_height * 512 + 200;
    }

    tuple<uint64_t, uint64_t> GetCanvasSize()
    {
        return make_tuple(m_canvas_width, m_canvas_height);
    }

    tuple<uint64_t, uint64_t> GetLAddrPivot(uint64_t i)
    {
        return make_tuple(100, 100 + m_grid_height * i);
    }

    tuple<uint64_t, uint64_t> GetLGridPivot(uint64_t i, uint64_t j)
    {
        return make_tuple(100 + m_addr_width + i * m_grid_width, 100 + m_grid_height * j);
    }

    tuple<uint64_t, uint64_t, uint64_t, uint64_t> GetLAddrParameters()
    {
        return make_tuple(m_addr_width, m_addr_height, m_l_addr_columns, m_l_addr_rows);
    }

    tuple<uint64_t, uint64_t, uint64_t, uint64_t> GetLGridParameters()
    {
        return make_tuple(m_grid_width, m_grid_height, m_l_grid_columns, m_l_grid_rows);
    }

    uint64_t GetLRowHeight(uint64_t i)
    {
        return 100 + i * m_grid_height;
    }

    tuple<uint64_t, uint64_t> GetMRowPivot(uint64_t i)
    {
        return make_tuple(1000, 100 + m_addr_height * i);
    }

    tuple<uint64_t, uint64_t> GetRRowPivot(uint64_t i)
    {
        return make_tuple(2000, 100 + m_addr_height * i);
    }

private:
    uint64_t m_grid_width{ 0 };
    uint64_t m_grid_height{ 0 };

    uint64_t m_addr_width{ 0 };
    uint64_t m_addr_height{ 0 };

    uint64_t m_canvas_width{ 0 };
    uint64_t m_canvas_height{ 0 };

    uint64_t m_l_addr_rows{ 512 };
    uint64_t m_l_addr_columns{ 1 };

    uint64_t m_l_grid_rows{ 512 };
    uint64_t m_l_grid_columns{ 8 };
};
