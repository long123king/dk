#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <fstream>
#include <map>

using namespace std;

class CSvgElement
{
public:
    virtual string Draw() = 0;
};

class CSvgPoint
    :public CSvgElement
{
public:
    CSvgPoint(uint64_t x, uint64_t y)
        :m_x(x)
        , m_y(y)
    {
    }

    
    virtual string Draw() override
    {
        return "";
    }

    uint64_t m_x;
    uint64_t m_y;
};

class CSvgDefsArrowHead :
    public CSvgElement
{
public:
    CSvgDefsArrowHead(string id, uint64_t width, uint64_t height)
        :m_id(id)
        , m_width(width)
        , m_height(height)
    {
    }

    
    /*
    <defs>
        <marker id = "arrowhead" markerWidth = "10" markerHeight = "7"
        refX = "0" refY = "3.5" orient = "auto">
        <polygon points = "0 0, 10 3.5, 0 7" / >
        < / marker>
        < / defs>
        */
    virtual string Draw() override
    {
        stringstream ss;

        ss << "\n<defs>"
            << "\n\t<g style='" << m_style << "'>"
            << "\n\t\t<marker id = '" << m_id
            << "' markerWidth = '" << m_width
            << "' markerHeight = '" << m_height
            << "' refX = '0' refY = '" << (double)m_height / 2
            << "' orient = 'auto'>"
            << "\n\t\t\t<polygon points = '0 0, "
            << m_width << " " << (double)m_height / 2 << ", 0 "
            << m_height
            << "' />"
            << "\n\t\t</marker>"
            << "\n\t</g>"
            << "\n</defs>\n";

        return ss.str();
    }

    void addStyle(string style)
    {
        m_style += style;
    }

private:
    string m_id;
    uint64_t m_width;
    uint64_t m_height;
    string m_style;
};

class CSvgArrow :
    public CSvgElement
{
public:
    CSvgArrow(CSvgPoint start, CSvgPoint end, string arrow_head_id)
        :m_start(start)
        , m_end(end)
        , m_arrow_head_id(arrow_head_id)
    {
    }


    
    virtual string Draw() override
    {
        stringstream ss;
        ss << "<line "
            << " x1='" << m_start.m_x << "' "
            << " y1='" << m_start.m_y << "' "
            << " x2='" << m_end.m_x << "' "
            << " y2='" << m_end.m_y << "' "
            << " marker-end='url(#" << m_arrow_head_id << ")'"
            << " />";

        return ss.str();
    }

private:
    CSvgPoint m_start;
    CSvgPoint m_end;
    string m_arrow_head_id;
};

class CSvgBesier :
    public CSvgElement
{
public:
    CSvgBesier(CSvgPoint start, CSvgPoint start_ctrl, CSvgPoint end_ctrl, CSvgPoint end, string arrow_head_id)
        :m_start(start)
        ,m_start_ctrl(start_ctrl)
        ,m_end(end)
        ,m_end_ctrl(end_ctrl)
        ,m_arrow_head_id(arrow_head_id)
    {

    }

    
    virtual string Draw() override
    {
        stringstream ss;
        ss << "<path d='"
            << " M" << m_start.m_x << " " << m_start.m_y << " "
            << " Q " << m_start_ctrl.m_x << " " << m_start_ctrl.m_y << ", "
            << " " << m_end_ctrl.m_x << " " << m_end_ctrl.m_y << " "
            << " T " << m_end.m_x << " " << m_end.m_y << "' "            
            << " marker-end='url(#" << m_arrow_head_id << ")'"
            << " />";

        return ss.str();
    }

private:
    CSvgPoint m_start;
    CSvgPoint m_start_ctrl;
    CSvgPoint m_end;
    CSvgPoint m_end_ctrl;
    string m_arrow_head_id;
};

class CSvgLine :
    public CSvgElement
{
public:
    CSvgLine(CSvgPoint start, CSvgPoint end, string style="")
        :m_start(start)
        , m_end(end)
        ,m_style(style)
    {
    }

    
    virtual string Draw() override
    {
        stringstream ss;
        ss << "<line "
            << " x1='" << m_start.m_x << "' "
            << " y1='" << m_start.m_y << "' "
            << " x2='" << m_end.m_x << "' "
            << " y2='" << m_end.m_y << "' ";

        if (!m_style.empty())
            ss << " style='" << m_style << "' ";
        
        ss << " />";

        return ss.str();
    }

private:
    CSvgPoint m_start;
    CSvgPoint m_end;
    string m_style;
};

class CSvgGroup :
    public CSvgElement
{
public:
    
    virtual string Draw() override
    {
        stringstream ss;

        ss << "\n<g ";
        
        if (!m_id.empty())
            ss << " id='" << m_id << "' ";

        ss <<" style='" << m_style << "' >\n";

        for (auto element : m_elements)
        {
            ss << "\t" << element->Draw() << endl;
        }

        ss << "</g>\n";

        return ss.str();
    }

    void addStyle(string style)
    {
        m_style += style;
    }

    void setId(string id)
    {
        m_id = id;
    }

    void appendElement(shared_ptr<CSvgElement> element)
    {
        m_elements.push_back(element);
    }

private:
    vector<shared_ptr<CSvgElement>> m_elements;
    string m_style;
    string m_id;
};

class CSvgLink :
    public CSvgElement
{
public:
    CSvgLink(string target)
        :m_target(target)
    {
    }

    virtual string Draw() override
    {
        stringstream ss;

        ss << "\n<a xlink:href='" << m_target << "'>";

        for (auto element : m_elements)
        {
            ss << "\t" << element->Draw() << endl;
        }

        ss << "</a>\n";

        return ss.str();
    }

    void appendElement(shared_ptr<CSvgElement> element)
    {
        m_elements.push_back(element);
    }

private:
    string m_target;
    vector<shared_ptr<CSvgElement>> m_elements;
};

class CSvgRect :
    public CSvgElement
{
public:

    CSvgRect(CSvgPoint pivot, uint64_t width, uint64_t height, string style = "", string style_class = "")
        :m_pivot(pivot)
        ,m_width(width)
        ,m_height(height)
        ,m_style(style)
        ,m_style_class(style_class)
    {
    }

    
    virtual string Draw() override
    {
        stringstream ss;

        ss << "<rect x='" << m_pivot.m_x
            << "' y='" << m_pivot.m_y
            << "' width='" << m_width
            << "' height='" << m_height
            << "' ";

        if (!m_style.empty())
            ss << " style = '" << m_style << "' ";

        if (!m_style_class.empty())
            ss << " class = '" << m_style_class << "' ";

        ss << " />";

        return ss.str();
    }

private:
    CSvgPoint m_pivot;
    uint64_t m_width;
    uint64_t m_height;
    string m_style;
    string m_style_class;
};

class CSvgText :
    public CSvgElement
{
public:
    CSvgText(CSvgPoint pivot, string text, string style="")
        :m_pivot(pivot)
        ,m_text(text)
        ,m_style(style)
    {
    }

    
    virtual string Draw() override
    {
        stringstream ss;

        ss << "<text x='" << m_pivot.m_x
            << "' y='" << m_pivot.m_y
            << "'"
            ;

        if (!m_style.empty())
            ss << " style = '" << m_style << "' ";

        ss <<">" << m_text << "</text>";

        return ss.str();
    }

private:
    CSvgPoint m_pivot;
    string m_text;
    string m_style;
};

class CSvgInnerStyle :
    public CSvgElement
{
public:
    CSvgInnerStyle()
    {
    }

    
    virtual string Draw() override
    {
        stringstream ss;

        ss << "<style type=\"text/css\">" << endl;

        for (auto& style_entry : m_styles)
        {
            ss << "\t" << style_entry.first
                << " {" << endl;

            for (auto& style : style_entry.second)
                ss << "\t\t" << style << endl;

            ss << "\t}" << endl;
        }
        ss << "</style>" << endl;

        return ss.str();
    }

    void addClassStyle(string element_class, string style)
    {
        if (m_styles.find(element_class) == m_styles.end())
        {
            m_styles[element_class] = vector<string>();
        }

        m_styles[element_class].push_back(style);
    };

private:
    map<string, vector<string>> m_styles;
};

class CSvgInnerScript :
    public CSvgElement
{
public:
    CSvgInnerScript()
    {
    }

    
    virtual string Draw() override
    {
        stringstream ss;

        ss << "<script type=\"application/ecmascript\"> <![CDATA[" << endl;

        for (auto& script : m_scripts)
        {
            ss << script << endl;
        }
        ss << "]]>" << endl << "</script>" << endl;

        return ss.str();
    }

    void addScript(string script)
    {
        m_scripts.push_back(script);
    }

private:
    vector<string> m_scripts;
};

class CSvgGrids :
    public CSvgElement
{
public:
    CSvgGrids(CSvgPoint pivot, uint64_t cell_width, uint64_t cell_height,  uint64_t cell_columns, uint64_t cell_rows, vector<string> texts, vector<string> styles, vector<string> row_styles)
        :m_pivot(pivot)
        , m_cell_width(cell_width)
        , m_cell_height(cell_height)
        , m_cell_rows(cell_rows)
        , m_cell_columns(cell_columns)
        ,m_texts(texts)
        ,m_styles(styles)
        ,m_row_styles(row_styles)
    {
    }

    
    virtual string Draw() override
    {
        for (uint64_t i = 0; i <= m_cell_rows; i++)
        {
            auto line = make_shared<CSvgLine>(
                CSvgPoint(m_pivot.m_x, m_pivot.m_y + i * m_cell_height),
                CSvgPoint(m_pivot.m_x + m_cell_columns * m_cell_width, m_pivot.m_y + i * m_cell_height));

            m_lines_group.appendElement(line);
        }

        for (uint64_t i = 0; i <= m_cell_columns; i++)
        {
            auto line = make_shared<CSvgLine>(
                CSvgPoint(m_pivot.m_x + i * m_cell_width, m_pivot.m_y),
                CSvgPoint(m_pivot.m_x + i * m_cell_width, m_pivot.m_y + m_cell_rows * m_cell_height));

            m_lines_group.appendElement(line);
        }

        for (uint64_t i = 0; i < m_cell_columns; i++)
        {
            for (uint64_t j = 0; j < m_cell_rows; j++)
            {
                auto line = make_shared<CSvgRect>(
                    CSvgPoint(m_pivot.m_x + i * m_cell_width, m_pivot.m_y + j * m_cell_height),
                    m_cell_width,
                    m_cell_height,
                    m_row_styles.empty()? "" : m_row_styles[j]);

                m_rects_group.appendElement(line);
            }
        }

        for (uint64_t i=0;i<m_texts.size();i++)
        {
            auto text = make_shared<CSvgText>(CSvgPoint(m_pivot.m_x + (i % m_cell_columns) * m_cell_width + 10,
                m_pivot.m_y + (i / m_cell_columns) * m_cell_height + 20),
                m_texts[i], m_styles.empty() ? "" : m_styles[i]);

            m_texts_group.appendElement(text);
        }

        return m_lines_group.Draw() + m_rects_group.Draw() + m_texts_group.Draw();
    }

    void addLineStyle(string style)
    {
        m_lines_group.addStyle(style);
    }

    void addTextStyle(string style)
    {
        m_texts_group.addStyle(style);
    }

    void addRectStyle(string style)
    {
        m_rects_group.addStyle(style);
    }

private:
    CSvgPoint m_pivot;
    uint64_t m_cell_width;
    uint64_t m_cell_height;
    uint64_t m_cell_rows;
    uint64_t m_cell_columns;

    CSvgGroup m_lines_group;
    CSvgGroup m_texts_group;
    CSvgGroup m_rects_group;
    vector<string> m_texts;
    vector<string> m_styles;
    vector<string> m_row_styles;
};

class CSvgDoc :
    public CSvgElement
{
public:
    CSvgDoc(uint64_t width, uint64_t height, CSvgPoint viewBoxPivot, uint64_t viewBoxWidth, uint64_t viewBoxHeight)
        :m_width(width)
        , m_height(height)
        ,m_viewBoxPivot(viewBoxPivot)
        ,m_viewBoxWidth(viewBoxWidth)
        ,m_viewBoxHeight(viewBoxHeight)
    {

    }

    
    virtual string Draw() override
    {
        stringstream ss;

        ss << "<svg id = 'root' width = '"
            << m_width << "'  height = '"
            << m_height << "' viewBox = '"
            << m_viewBoxPivot.m_x << " " << m_viewBoxPivot.m_y << " "
            << m_viewBoxWidth << " " << m_viewBoxHeight
            << "' xmlns = 'http://www.w3.org/2000/svg' xmlns:xlink = 'http://www.w3.org/1999/xlink' onload='init(evt)'>";

        for (auto element : m_elements)
        {
            ss << element->Draw();
        }

        for (auto element : m_dynamic_elements)
        {
            ss << element->Draw();
        }

        ss << "\n</svg>\n";

        return ss.str();
    }

    void appendElement(shared_ptr<CSvgElement> element)
    {
        m_elements.push_back(element);
    }

    void appendDynamicElement(shared_ptr<CSvgElement> element)
    {
        m_dynamic_elements.push_back(element);
    }

    void resetDynamicElements()
    {
        m_dynamic_elements.clear();
    }

    void Save(string filename)
    {
        ofstream ofs(filename, ios::out);

        string content = Draw();

        ofs << content;

        ofs.close();
    }

private:
    uint64_t m_width;
    uint64_t m_height;
    CSvgPoint m_viewBoxPivot;
    uint64_t m_viewBoxWidth;
    uint64_t m_viewBoxHeight;
    vector<shared_ptr<CSvgElement>> m_elements;    

    vector<shared_ptr<CSvgElement>> m_dynamic_elements;
};

inline string SvgEscapeText(string text, uint64_t limit=2000)
{
    stringstream ss;
    for (auto ch : text)
    {
        if (ch == '<')
            ss << "&lt;";
        else if (ch == '>')
            ss << "&gt;";
        else if (ch == '&')
            ss << "&amp;";
        else
            ss << ch;
    }

    string str = ss.str();
    if (str.size() > limit && limit > 6)
    {
        for (auto i = limit - 6; i < limit; i++)
        {
            if (str[i] == '&')
            {
                return str.substr(0, limit - 6 + i) + "......";
            }
        }
        return str.substr(0, limit - 6) + "......";
    }

    return str;
}