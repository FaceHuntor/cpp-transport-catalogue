#include "svg.h"

using namespace std::literals;
using namespace svg;

struct ColorPrinter {
    std::ostream& out;

    void operator()(std::monostate) const {
        out << "none"sv;
    }

    void operator()(const std::string& color) const {
        out << color;
    }

    void operator()(const Rgb& color) const {
        out << "rgb(" << int(color.red) << "," << int(color.green) << "," << int(color.blue) << ")";
    }

    void operator()(const Rgba& color) const {
        out << "rgba(" << int(color.red) << "," << int(color.green) << "," << int(color.blue) << "," << color.opacity
            << ")";
    }
};


std::ostream& operator<<(std::ostream& out, StrokeLineCap item) {
    switch (item) {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin item) {
    switch (item) {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(ColorPrinter{out}, color);
    return out;
}

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\""sv;
    out << " r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(svg::Point point) {
    points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    context.out << "<polyline points=\"";
    bool not_first = false;
    for(const auto& p : points_) {
        if(not_first) {
            context.out << " ";
        }
        context.out << p.x << "," << p.y;
        not_first = true;
    }
    context.out << "\"";
    RenderAttrs(context.out);
    context.out << "/>";
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    context.out << "<text";
    RenderAttrs(context.out);
    context.out << " x=\"" << pos_.x <<"\"";
    context.out << " y=\"" << pos_.y <<"\"";
    context.out << " dx=\"" << offset_.x <<"\"";
    context.out << " dy=\"" << offset_.y <<"\"";
    context.out << " font-size=\"" << size_ <<"\"";
    if(font_family_) {
        context.out << " font-family=\"" << *font_family_ <<"\"";
    }
    if(font_weight_) {
        context.out << " font-weight=\"" << *font_weight_ <<"\"";
    }
    context.out << ">";
    RenderEscapedText(data_, context.out);
    context.out << "</text>";
}

void Text::RenderEscapedText(std::string_view text, std::ostream& out) {
    for(const auto c: text) {
        switch (c) {
            case '\"':
                out << "&quot;";
                break;
            case '\'':
                out << "&apos;";
                break;
            case '<':
                out << "&lt;";
                break;
            case '>':
                out << "&gt;";
                break;
            case '&':
                out << "&amp;";
                break;
            default:
                out << c;
        }
    }
}


// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    RenderContext obj_context = RenderContext(out, 0, 2).Indented();

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;

    for(const auto& obj: objects_) {
        obj->Render(obj_context);
    }

    out << "</svg>";
}

}  // namespace svg