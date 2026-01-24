#include "hexsvg.h"
#include <format>

CHexSvg::CHexSvg(size_t boxes_per_row)
    :m_boxes_per_row(boxes_per_row)
{
    if (m_boxes_per_row < 4)
        m_boxes_per_row = 4;

    m_m = std::make_shared<CHexSvgMetrics>(
        m_boxes_per_row,
        m_row_header_width,
        m_colum_header_height,
        m_row_height,
        m_column_width);
}

CHexSvg::~CHexSvg()
{
}

std::string CHexSvg::add_text(size_t row, size_t column, std::string content)
{
    return std::format(R"(<text font-size="{}" font-family="{}" x="{}" y="{}" text-anchor="middle">{}</text>)",
        m_font_size,
        m_font_family,
        row * m_row_height,
        column * m_column_width,
        content);
}

void CHexSvg::set_address(size_t addr)
{
    m_init_address = addr;
}

void CHexSvg::set_bitmap(bool b_bitmap)
{
    m_show_bits = b_bitmap;
}

void CHexSvg::set_ascii(bool b_ascii)
{
    m_show_ascii = b_ascii;
}

void CHexSvg::set_auto_color(bool b_auto_color)
{
    m_auto_color = b_auto_color;
}

void CHexSvg::set_legend(bool b_legend)
{
    m_show_legend = b_legend;
}

void CHexSvg::add_byte(uint8_t value, std::string color)
{
    if (m_auto_color)
    {
        color = auto_calc_color(value);
        m_boxes.push_back(
            std::make_shared<CHexBox>(std::format("{:02X}", value),
                m_offset, 1, true, color));
        m_offset += 1;
    }
}

void CHexSvg::add_word(uint16_t value, std::string color)
{
    m_boxes.push_back(
        std::make_shared<CHexBox>(std::format("{:04X}", value),
            m_offset, 2, true, color));
    m_offset += 2;
}

void CHexSvg::add_dword(uint32_t value, std::string color)
{
    m_boxes.push_back(
        std::make_shared<CHexBox>(std::format("{:08X}", value),
            m_offset, 4, true, color));
    m_offset += 4;
}

void CHexSvg::add_qword(uint64_t value, std::string color)
{
    if (m_boxes_per_row >= 8)
    {
        m_boxes.push_back(
            std::make_shared<CHexBox>(std::format("{:016X}", value),
                m_offset, 8, true, color));
        m_offset += 8;
    }
}

void CHexSvg::add_buffer(std::string buffer, std::string color)
{
    for (auto ch : buffer)
    {
        if (m_auto_color)
        {
            auto auto_color = auto_calc_color(ch);
            m_boxes.push_back(std::make_shared<CHexBox>(
                std::format("{:02X}", (unsigned char)ch),
                m_offset, 1, true, auto_color));
        }
        else
        {
            m_boxes.push_back(std::make_shared<CHexBox>(
                std::format("{:02X}", (unsigned char)ch),
                m_offset, 1, true, color));
        }

        m_offset += 1;
    }
}

void CHexSvg::add_legend(std::string color, std::string note)
{
    m_legends[color] = note;
}

void CHexSvg::generate_row_header()
{
    size_t rows = (m_offset + m_boxes_per_row - 1) / m_boxes_per_row;
    for (size_t i = 0; i < rows; i++)
    {
        auto [x, y, width, height] = m_m->get_row_header(i);
        m_row_header_content << std::format(R"(<text font-size="{}" font-family="{}" x="{}" y="{}" text-anchor="middle">{}</text>)",
            m_font_size, 
            m_font_family, 
            x + width / 2,
            y + (height + m_font_size) / 2,
            std::format("0x{:016X}", m_init_address + i * m_boxes_per_row)
            );
    }
}

void CHexSvg::generate_column_header()
{
    for (size_t i = 0; i < m_boxes_per_row; i++)
    {
        auto [x, y, width, height] = m_m->get_column_header(i);
        m_column_header_content << std::format(R"(<text font-size="{}" font-family="{}" x="{}" y="{}" text-anchor="middle">{}</text>)",
            m_font_family,
            m_font_family,
            x + width / 2,
            y + (height + m_font_size) / 2, 
            std::format("{:X}", i));
    }
}

void CHexSvg::generate_framework()
{
    for (auto& box : m_boxes)
    {
        m_framework_content << box->gen_framework_bg(m_m);
    }

    for (auto& box : m_boxes)
    {
        m_framework_content << box->gen_framework_separator(m_m);
    }

    auto [x, y, width, height] = m_m->get_column_header(m_boxes_per_row - 1);

    size_t rows = (m_offset + m_boxes_per_row - 1) / m_boxes_per_row;

    for (size_t i = 0; i < rows; i++)
    {
        auto [x, y, width, height] = m_m->get_row_framework(i);
        m_framework_content << std::format(R"(<rect x="{}" y="{}" height="{}" width="{}" stroke="#000000" fill="none" stroke-width="1" />)",
            x,
            y,
            height,
            width);
    }
}

void CHexSvg::generate_text()
{
    for (auto& box : m_boxes)
    {
        m_content << box->gen_text(m_m);
    }
}

void CHexSvg::generate_svg_headers()
{
    auto [x1, y1, width1, height1] = m_m->get_column_header(m_boxes_per_row - 1);
    auto [x2, y2, width2, height2] = m_m->get_cell(m_offset, 1);

    size_t canvas_width = x1 + width1;
    size_t canvas_height = y2 + height2;

    if (m_show_legend)
        canvas_width += (10 * width2);

    m_svg_headers << std::format(R"(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns:svg="http://www.w3.org/2000/svg"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink" version="1.0" width="{0}" height="{1}" viewBox="0 0 {0} {1}">)",
        canvas_width,
        canvas_height);
}

void CHexSvg::generate_bitmap()
{
    for (auto& box : m_boxes)
    {
        m_bitmap_content << box->gen_bitmap(m_m);
    }
}

void CHexSvg::generate_ascii()
{
    for (auto& box : m_boxes)
    {
        m_ascii_content << box->gen_ascii(m_m);
    }
}

void CHexSvg::generate_legend()
{
    auto [_x, _y, _width, _height] = m_m->get_cell(7, 1);

    auto x = _x + 2 * _width;
    auto y = _y + _height;
    auto width = 7 * _width;
    auto height = (m_legends.size() + 2) * _height;

    m_legend_content << std::format(R"(<rect x="{}" y="{}" height="{}" width="{}" stroke="#000000" std::fill="none" stroke-width="1" />)",
        x, y, height, width);

    size_t i = 0;
    for (auto& it : m_legends)
    {
        m_legend_content << std::format(R"(<rect x="{}" y="{}" height="{}" width="{}" stroke="#000000" std::fill="{}" stroke-width="1" />)",
            x + _width,
            y + (1 + i) * _height,
            _height,
            _width,
            it.first);

        m_legend_content << std::format(R"(<text font-size="{}" font-family="{}" x="{}" y="{}" text-anchor="middle">{}</text>)",
            m_font_size,
            m_font_family,
            x + 4 * _width + _width / 2,
            y + (1 + i) * _height + (_height + m_font_size) / 2,
            it.second);

        i++;
    }
}

std::string CHexSvg::generate()
{
    generate_column_header();
    generate_row_header();
    generate_framework();
    generate_text();
    generate_svg_headers();
    if (m_show_bits)
        generate_bitmap();

    if (m_show_ascii)
        generate_ascii();

    if (m_show_legend)
        generate_legend();

    std::string svg_content;

    svg_content += m_svg_headers.str();
    svg_content += m_row_header_content.str();
    svg_content += m_column_header_content.str();
    svg_content += m_framework_content.str();
    svg_content += m_content.str();
    svg_content += m_bitmap_content.str();
    svg_content += m_ascii_content.str();
    svg_content += m_legend_content.str();
    svg_content += m_svg_trailers;

    return svg_content;
}

std::string CHexSvg::auto_calc_color(uint8_t b)
{
    auto highlights = CSvgTheme::GetInstance()->get_highlight();
    auto trivials = CSvgTheme::GetInstance()->get_trivial();

    if (b == 0)
        return highlights[0];
    else if (b >= 0x30 && b <= 0x39)
        return highlights[1];
    else if (b >= 0x41 && b <= 0x5A)
        return highlights[2];
    else if (b >= 0x61 && b <= 0x7A)
        return highlights[3];
    else if (b < 0x80)
        return highlights[4];
    else if (b < 0xF0)
        return highlights[5];
    else
        return trivials[0];
}

CHexSvgMetrics::CHexSvgMetrics(size_t bpr, size_t rhw, size_t chh, size_t rh, size_t cw)
    :m_boxes_per_row(bpr)
    ,m_row_header_width(rhw)
    ,m_column_header_height(chh)
    ,m_row_height(rh)
    ,m_column_width(cw)
{
}

CHexSvgMetrics::~CHexSvgMetrics()
{
}

std::tuple<size_t, size_t, size_t, size_t> CHexSvgMetrics::get_cell(size_t offset, size_t span)
{
    size_t row = offset / m_boxes_per_row;
    size_t column = offset % m_boxes_per_row;

    if (column + span > m_boxes_per_row)
    {
        span = m_boxes_per_row - column;
    }

    return std::make_tuple(m_row_header_width + column * m_column_width,
        m_column_header_height + row * m_row_height,
        m_column_width * span,
        m_row_height);
}

std::tuple<size_t, size_t, size_t, size_t> CHexSvgMetrics::get_row_header(size_t row)
{
    return std::make_tuple(0,
        m_column_header_height + row * m_row_height,
        m_row_header_width,
        m_row_height);
}

std::tuple<size_t, size_t, size_t, size_t> CHexSvgMetrics::get_row_framework(size_t row)
{
    return std::make_tuple(m_row_header_width,
        m_column_header_height + row * m_row_height,
        m_column_width * m_boxes_per_row,
        m_row_height);
}

std::tuple<size_t, size_t, size_t, size_t> CHexSvgMetrics::get_column_header(size_t column)
{
    return std::make_tuple(m_row_header_width + column * m_column_width,
        0,
        m_column_width,
        m_column_header_height);
}

CHexBox::CHexBox(std::string value, size_t offset, size_t span, bool solid_outline, std::string fill_color, size_t font_size, std::string font_family)
    :m_value(value)
    ,m_span(span)
    ,m_solid_outline(solid_outline)
    ,m_fill_color(fill_color)
    ,m_offset(offset)
    ,m_font_size(font_size)
    ,m_font_family(font_family)
{
}

CHexBox::~CHexBox()
{
}

std::string CHexBox::gen_framework_bg(std::shared_ptr<CHexSvgMetrics> m)
{
    auto [x, y, width, height] = m->get_cell(m_offset, m_span);

    return std::format(R"(<rect x="{}" y="{}" width="{}" height="{}" std::fill="{}" stroke="none"/>)",
        x,
        y,
        width,
        height,
        m_fill_color);
}

std::string CHexBox::gen_framework_separator(std::shared_ptr<CHexSvgMetrics> m)
{
    auto [x, y, width, height] = m->get_cell(m_offset, m_span);

    return std::format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}" stroke="#000000" stroke-width="1" {} />)",
        x+width,
        y,
        x+width,
        y+height,
        m_solid_outline? R"(stroke-dasharray="1,3")" : "");
}

std::string CHexBox::gen_text(std::shared_ptr<CHexSvgMetrics> m)
{
    auto [x, y, width, height] = m->get_cell(m_offset, m_span);

    return std::format(R"(<text font-size="{}" font-family="{}" x="{}" y="{}" text-anchor="middle" letter-spacing="{}">{}</text>)",
        m_font_size,
        m_font_family,
        x + width/2,
        y + (height + m_font_size)/2,
        m_font_size/8,
        m_value);
}

std::string CHexBox::gen_bitmap(std::shared_ptr<CHexSvgMetrics> m)
{
    auto [x, y, width, height] = m->get_cell(m_offset, m_span);

    if (m_span > 8)
        return "";

    size_t total_grids = m_span * 8;
    size_t grid_width = width / total_grids;

    size_t int_value = std::stoull(m_value, nullptr, 16);

    std::string bitmap_svg = "";
    for (size_t i = 0; i < total_grids; i++)
    {
        bitmap_svg += std::format(R"(<rect x="{}" y="{}" height="{}" width="{}" std::fill="{}" stroke="#000000" stroke-width="1"/>)",
            x + (total_grids - i - 1) * grid_width, 
            y, 
            grid_width, 
            grid_width,
            (((1 << i) & int_value) != 0)? "#e0e0e0":"#606060");
    }

    return bitmap_svg;
}

std::string CHexBox::gen_ascii(std::shared_ptr<CHexSvgMetrics> m)
{
    auto [x, y, width, height] = m->get_cell(m_offset, m_span);

    std::string ascii_text = "";
    size_t int_value = stoull(m_value, nullptr, 16);
    for (size_t i = 0; i < m_span; i++)
    {
        char ch = (int_value & 0xFF);
        if (ch == '<')
            ascii_text += "&lt;";
        else if (ch == '>')
            ascii_text += "&gt;";
        else if (ch == '&')
            ascii_text += "&amp;";
        else if (ch >= ' ' && ch <= '~')
            ascii_text += ch;
        else
            ascii_text += '.';

        int_value = int_value >> 8;
    }

    //reverse(ascii_text.begin(), ascii_text.end());

    return std::format(R"(<text font-size="{}" font-family="{}" x="{}" y="{}" text-anchor="end">{}</text>)",
        m_font_size / 2, 
        m_font_family,
        x + width - m_font_size / 8, 
        y + height - m_font_size / 8, ascii_text);
}
