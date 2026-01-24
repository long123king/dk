#pragma once

#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include <map>



class CSvgTheme
{
public:
    static CSvgTheme* GetInstance()
    {
        static CSvgTheme s_instance;
        return &s_instance;
    }

    CSvgTheme()
    {
        initialize();
    }

    ~CSvgTheme()
    {
    }

    void initialize()
    {
        for (size_t i = 1; i < 7; i++)
        {
            std::stringstream ss;
            ss << "#";
            for (size_t j = 0; j < 3; j++)
            {
                if ((i & (1 << j)) != 0)
                    ss << "f0";
                else
                    ss << "a0";
            }
            m_highlight_colors.push_back(ss.str());
        }


        m_trivial_colors.push_back("#f0f0f0");
        m_trivial_colors.push_back("#a0a0a0");
        m_trivial_colors.push_back("#808080");
        m_trivial_colors.push_back("#404040");
    }

    std::vector<std::string> get_highlight()
    {
        return m_highlight_colors;
    }

    std::vector<std::string> get_trivial()
    {
        return m_trivial_colors;
    }

private:
    std::vector<std::string> m_highlight_colors;
    std::vector<std::string> m_trivial_colors;
};

class CHexSvgMetrics
{
public:
    CHexSvgMetrics(size_t bpr, size_t rhw, size_t chh, size_t rh, size_t cw);
    ~CHexSvgMetrics();

    std::tuple<size_t, size_t, size_t, size_t> get_cell(size_t offset, size_t span);

    std::tuple<size_t, size_t, size_t, size_t> get_row_header(size_t row);
    std::tuple<size_t, size_t, size_t, size_t> get_row_framework(size_t row);
    std::tuple<size_t, size_t, size_t, size_t> get_column_header(size_t column);
private:
    size_t m_boxes_per_row;
    size_t m_row_header_width;
    size_t m_column_header_height;
    size_t m_row_height;
    size_t m_column_width;
};

class CHexBox
{
public:
    CHexBox(std::string value, size_t offset, size_t span=1, bool solid_outline = true,
        std::string fill_color="#a0ffa0", size_t font_size = 16, 
        std::string font_family = "monospace"
        );
    ~CHexBox();

    std::string gen_framework_bg(std::shared_ptr<CHexSvgMetrics> m);
    std::string gen_framework_separator(std::shared_ptr<CHexSvgMetrics> m);
    std::string gen_text(std::shared_ptr<CHexSvgMetrics> m);
    std::string gen_bitmap(std::shared_ptr<CHexSvgMetrics> m);
    std::string gen_ascii(std::shared_ptr<CHexSvgMetrics> m);

private:
    std::string m_value;
    size_t m_span;
    size_t m_offset;
    size_t m_font_size;

    bool m_solid_outline;
    std::string m_fill_color;

    std::string m_font_family;
};

class CHexSvg
{
public:
    CHexSvg(size_t boxes_per_row = 16);
    ~CHexSvg();

    std::string add_text(size_t row, size_t column, std::string content);
    void set_address(size_t addr);
    void set_bitmap(bool b_bitmap);
    void set_ascii(bool b_ascii);
    void set_auto_color(bool b_auto_color);
    void set_legend(bool b_legend);

    void add_byte(uint8_t value, std::string color = "#a0ffa0");
    void add_word(uint16_t value, std::string color = "#a0ffa0");
    void add_dword(uint32_t value, std::string color = "#a0ffa0");
    void add_qword(uint64_t value, std::string color = "#a0ffa0");

    void add_buffer(std::string buffer, std::string color = "#a0ffa0");

    void add_legend(std::string color, std::string note);

    void generate_row_header();
    void generate_column_header();
    void generate_framework();
    void generate_text();
    void generate_svg_headers();
    void generate_bitmap();
    void generate_ascii();
    void generate_legend();

    std::string generate();

    std::string auto_calc_color(uint8_t b);


private:
    size_t m_boxes_per_row;

    bool m_buffer_as_bytes{ true };
    bool m_show_bits{ true };
    bool m_show_ascii{ true };
    bool m_auto_color{ false };
    bool m_show_legend{ true };

    size_t m_address{ 0 };

    size_t m_font_size{ 16 };
    size_t m_init_address{ 0 };
    size_t m_offset{ 0 };

    std::vector<std::shared_ptr<CHexBox>> m_boxes;

    std::shared_ptr<CHexSvgMetrics> m_m;

    size_t m_row_header_width{ 200 };
    size_t m_colum_header_height{ 40 };
    size_t m_row_height{ 30 };
    size_t m_column_width{ 40 };

    std::string m_font_family{ "monospace" };
    std::string m_svg_trailers{R"(</svg>)"};

    std::stringstream m_svg_headers;
    std::stringstream m_row_header_content;
    std::stringstream m_column_header_content;
    std::stringstream m_content;
    std::stringstream m_framework_content;
    std::stringstream m_bitmap_content;
    std::stringstream m_ascii_content;
    std::stringstream m_svg_text;
    std::stringstream m_legend_content;

    std::map<std::string, std::string> m_legends;

};