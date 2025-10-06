#include "AccordionCtrl.h"

namespace Upp {

CH_STYLE(AccordionCtrl, Style, StyleDefault)
{
    headerLook          = SColorFace();
    bodyLook            = SColorPaper();
    headerInk           = SColorText();
    headerInkHover      = Blend(SColorHighlight(), SColorText(), 128);
    headerBgHover       = Blend(SColorFace(), Black(), 30);
    dividerColor        = Blend(SColorShadow(), SColorFace(), 200);
    borderColor         = SColorShadow();
    headerFont          = StdFont().Bold();
    headerCy            = 28;
    headerLPad          = 8;
    headerRPad          = 8;
    headerSpacing       = 6;
    dividerThick        = 1;
    iconCx              = 12;
    iconTextGap         = 6;
    sectionVGap         = 2;
    borderWidth         = 0;
    animMs              = 120;
    focusHeaderOnToggle = true;
}

static Image MakeChevronRight(int sz, Color ink) {
    ImageBuffer ib(sz, sz);
    Fill(~ib, RGBAZero(), ib.GetLength());
    BufferPainter p(ib); // AA vector drawing

    int cx = sz / 2, cy = sz / 2;
    int w = sz / 3;
    Vector<Point> pts;
    pts << Point(cx - w/2, cy - w) << Point(cx + w/2, cy) << Point(cx - w/2, cy + w);
    p.DrawPolyline(pts, 2, ink);
    return ib;
}

static Image MakeChevronDown(int sz, Color ink) {
    ImageBuffer ib(sz, sz);
    Fill(~ib, RGBAZero(), ib.GetLength());
    BufferPainter p(ib); // AA vector drawing
    int cx = sz / 2, cy = sz / 2;
    int w = sz / 3;
    Vector<Point> pts;
    pts << Point(cx - w, cy - w/2) << Point(cx, cy + w/2) << Point(cx + w, cy - w/2);
    p.DrawPolyline(pts, 2, ink);
    return ib;
}

// High‑fidelity lock icon with anti‑aliasing, scaled to sz×sz, drawn in 'ink'
static Image MakeChevronLock(int sz, Color ink) {
    // Transparent buffer
    ImageBuffer ib(sz, sz);
    Fill(~ib, RGBAZero(), ib.GetLength());
    BufferPainter p(ib); // AA vector drawing

    // Proportional inset so thick strokes don’t clip at edges
    const double margin = max(0.5, round(sz * 0.04));
    const Rectf inset(margin, margin, sz - margin, sz - margin);
    const double W = inset.GetWidth();
    const double H = inset.GetHeight();

    auto X = [&](double fx){ return inset.left + W * fx; };
    auto Y = [&](double fy){ return inset.top  + H * fy; };

    // Stroke thickness scaled to size (looks good ~12–28px)
    const double T = max(0.5, round(sz * 0.18));     // outline thickness
    const double Tk = max(0.5, round(T * 0.85));     // keyhole stem

    // 1) Body rectangle outline (your proportions)
    p.Begin();
    p.Move(Pointf(X(0.1288), Y(0.4206)));
    p.Line(Pointf(X(0.8498), Y(0.4206)));
    p.Line(Pointf(X(0.8498), Y(0.9442)));
    p.Line(Pointf(X(0.1288), Y(0.9442)));
    p.Close();
    p.Stroke(T, ink);

    // 2) Shackle curve (cubic)
    p.Begin();
    p.Move(Pointf(X(0.2575), Y(0.3863)));
    p.Cubic(Pointf(X(0.2575), Y(0.0086)),
            Pointf(X(0.7210), Y(0.0086)),
            Pointf(X(0.7210), Y(0.3863)));
    p.Stroke(T, ink);

    // 3) Keyhole circle (filled)
    const double R = min(W, H) * 0.0716;
    p.Begin();
    p.Circle(X(0.4614), Y(0.6438), R);
    p.Fill(ink);

    // 4) Keyhole stem (stroke)
    p.Begin();
    p.Move(Pointf(X(0.4635), Y(0.8219)));
    p.Line(Pointf(X(0.4635), Y(0.6953)));
    p.Stroke(Tk, ink);

    return ib;
}

AccordionCtrl::AccordionCtrl()
    : style(&StyleDefault())
    , singleExpand(false)
    , enforceOne(false)
    , hotSection(-1)
    , pressedSection(-1)
{
    iconClosed = MakeChevronRight(12, SColorText());
    iconOpened = MakeChevronDown(12, SColorText());
    iconLock = MakeChevronLock(12, SColorText());
}

AccordionCtrl& AccordionCtrl::SetStyle(const Style& st) {
    style = &st;
    Refresh();
    return *this;
}

const AccordionCtrl::Style& AccordionCtrl::GetStyle() const { return *style; }
int  AccordionCtrl::GetCount() const { return sections.GetCount(); }
int  AccordionCtrl::AddSection(const String& title) { return InsertSection(sections.GetCount(), title); }

int AccordionCtrl::InsertSection(int at, const String& title) {
    ASSERT(at >= 0 && at <= sections.GetCount());
    Section& s = sections.Insert(at);
    s.title = title;
    s.open = false;
    s.animating = false;
    s.currentBodyCy = 0;
    s.targetBodyCy = 0;
    s.animTicket = 0;
    s.hot = false;
    s.pressed = false;
    s.lock = UNLOCKED; // NEW

    s.header.Create<HeaderPane>();
    s.body.Create<ParentCtrl>();
    s.header->owner = this;

    Add(s.header->SizePos());
    Add(s.body->SizePos());

    UpdateHeaderIndices();
    RefreshLayout();
    return at;
}

void AccordionCtrl::RemoveSection(int i) {
    ASSERT(i >= 0 && i < sections.GetCount());
    StopAnimation(i);
    if(sections[i].header) sections[i].header->Remove();
    if(sections[i].body)   sections[i].body->Remove();
    sections.Remove(i);

    if(enforceOne && sections.GetCount() > 0) {
        bool anyOpen = false;
        for(int j = 0; j < sections.GetCount(); j++)
            if(sections[j].open) { anyOpen = true; break; }
        if(!anyOpen) Open(0, false);
    }
    UpdateHeaderIndices();
    RefreshLayout();
}

void AccordionCtrl::Clear() {
    for(int i = 0; i < sections.GetCount(); i++) {
        StopAnimation(i);
        if(sections[i].header) sections[i].header->Remove();
        if(sections[i].body)   sections[i].body->Remove();
    }
    sections.Clear();
    hotSection = -1;
    pressedSection = -1;
    RefreshLayout();
}

Ctrl&       AccordionCtrl::HeaderCtrl(int i) { ASSERT(i >= 0 && i < sections.GetCount()); return *sections[i].header; }
ParentCtrl& AccordionCtrl::BodyCtrl(int i)   { ASSERT(i >= 0 && i < sections.GetCount()); return *sections[i].body; }
bool        AccordionCtrl::IsOpen(int i) const { ASSERT(i >= 0 && i < sections.GetCount()); return sections[i].open; }

void AccordionCtrl::Open(int i, bool animate) {
    ASSERT(i >= 0 && i < sections.GetCount());
    if(sections[i].open) return;
    if(sections[i].lock == LOCKED_CLOSED) return; // cannot open

    if(WhenBeforeToggle(i)) return;

    if(singleExpand) CloseOthers(i);

    sections[i].open = true;
    int targetH = GetBodyMinHeight(i);

    if(animate && animEnabled && animOpenMs > 0)
        StartAnimation(i, targetH, animOpenMs);
    else {
        sections[i].currentBodyCy = targetH;
        sections[i].targetBodyCy  = targetH;
        RefreshLayout();
    }
    WhenOpen(i);
}

void AccordionCtrl::Close(int i, bool animate) {
    ASSERT(i >= 0 && i < sections.GetCount());
    if(!sections[i].open) return;
    if(sections[i].lock == LOCKED_OPEN) return; // cannot close

    if(enforceOne) {
        int openCount = 0;
        for(int j = 0; j < sections.GetCount(); j++)
            if(sections[j].open) openCount++;
        if(openCount <= 1) return;
    }
    if(WhenBeforeToggle(i)) return;

    sections[i].open = false;

    if(animate && animEnabled && animCloseMs > 0)
        StartAnimation(i, 0, animCloseMs);
    else {
        sections[i].currentBodyCy = 0;
        sections[i].targetBodyCy  = 0;
        RefreshLayout();
    }
    WhenClose(i);
}

void AccordionCtrl::Toggle(int i, bool animate) {
    ASSERT(i >= 0 && i < sections.GetCount());
    if(sections[i].open) Close(i, animate);
    else                 Open(i, animate);
}

AccordionCtrl& AccordionCtrl::SingleExpand(bool b) {
    singleExpand = b;
    if(b) {
        int firstOpen = -1;
        for(int i = 0; i < sections.GetCount(); i++)
            if(sections[i].open) {
                if(firstOpen < 0) firstOpen = i;
                else Close(i, false);
            }
    }
    return *this;
}

AccordionCtrl& AccordionCtrl::AtLeastOneOpen(bool b) { enforceOne = b; if(b) EnsureAtLeastOneOpen(-1); return *this; }
AccordionCtrl& AccordionCtrl::SetTitle(int i, const String& title) { ASSERT(i>=0&&i<sections.GetCount()); sections[i].title=title; RefreshSection(i); return *this; }
AccordionCtrl& AccordionCtrl::SetHeaderAlign(int i, int align)     { ASSERT(i>=0&&i<sections.GetCount()); sections[i].align=align; RefreshSection(i); return *this; }
AccordionCtrl& AccordionCtrl::UseDivider(int i, bool use)           { ASSERT(i>=0&&i<sections.GetCount()); sections[i].useDivider=use; RefreshSection(i); return *this; }
AccordionCtrl& AccordionCtrl::SetIcons(Image c, Image e)            { iconClosed=c; iconOpened=e; Refresh(); return *this; }
AccordionCtrl& AccordionCtrl::SetAnimationMs(int ms)                { const_cast<Style*>(style)->animMs=max(0, ms); return *this; }

void AccordionCtrl::Serialize(Stream& s) {
    int version = 1;
    s % version;
    s % singleExpand % enforceOne;

    if(s.IsLoading()) {
        int count; s % count;
        Clear();
        for(int i = 0; i < count; i++) {
            String title; bool open; int align; bool useDivider;
            s % title % open % align % useDivider;
            AddSection(title);
            sections[i].open = open;
            sections[i].align = align;
            sections[i].useDivider = useDivider;
            sections[i].currentBodyCy = open ? GetBodyMinHeight(i) : 0;
            sections[i].targetBodyCy = sections[i].currentBodyCy;
        }
        RefreshLayout();
    } else {
        int count = sections.GetCount();
        s % count;
        for(int i = 0; i < count; i++)
            s % sections[i].title % sections[i].open % sections[i].align % sections[i].useDivider;
    }
}

void AccordionCtrl::Paint(Draw& w) {
    Size sz = GetSize();
    w.DrawRect(sz, SColorPaper());

    if(style->borderWidth > 0) {
        w.DrawRect(0, 0, sz.cx, style->borderWidth, style->borderColor);
        w.DrawRect(0, sz.cy - style->borderWidth, sz.cx, style->borderWidth, style->borderColor);
        w.DrawRect(0, 0, style->borderWidth, sz.cy, style->borderColor);
        w.DrawRect(sz.cx - style->borderWidth, 0, style->borderWidth, sz.cy, style->borderColor);
    }

    for(int i = 0; i < sections.GetCount(); i++) {
        Section& s = sections[i];
        bool hot = (i == hotSection);
        bool pressed = (i == pressedSection);

        ChPaint(w, s.headerRect, style->headerLook);
        if(hot || pressed)
            w.DrawRect(s.headerRect, style->headerBgHover);

        // Icon: lock when locked; otherwise chevron
        Image icon = (s.lock != UNLOCKED) ? iconLock : (s.open ? iconOpened : iconClosed);
        if(!icon.IsEmpty())
            w.DrawImage(s.iconRect.left, s.iconRect.top, icon);

        Color ink = (hot || pressed) ? style->headerInkHover : style->headerInk;
        int textX = s.iconRect.right + style->iconTextGap;
        int textY = s.headerRect.top + (style->headerCy - GetTextSize(s.title, style->headerFont).cy) / 2;
        w.DrawText(textX, textY, s.title, style->headerFont, ink);

        if(s.useDivider && style->dividerThick > 0) {
            int divX = s.headerRect.right - style->headerRPad - 1;
            w.DrawRect(divX, s.headerRect.top + 4, style->dividerThick, style->headerCy - 8, style->dividerColor);
        }

        if(s.currentBodyCy > 0)
            ChPaint(w, s.bodyRect, style->bodyLook);
    }
}

void AccordionCtrl::Layout() {
    Size sz = GetSize();
    int y = style->borderWidth;
    int cx = sz.cx - 2 * style->borderWidth;

    for(int i = 0; i < sections.GetCount(); i++) {
        Section& s = sections[i];

        s.headerRect = RectC(style->borderWidth, y, cx, style->headerCy);
        s.iconRect   = RectC(s.headerRect.left + style->headerLPad,
                             s.headerRect.top + (style->headerCy - style->iconCx) / 2,
                             style->iconCx, style->iconCx);
        if(s.header) { s.header->SetRect(s.headerRect); s.header->index = i; }

        y += style->headerCy;

        int bodyCy = s.currentBodyCy;
        s.bodyRect = RectC(style->borderWidth, y, cx, bodyCy);
        if(s.body) s.body->SetRect(s.bodyRect);

        y += bodyCy + style->sectionVGap;
    }
}

bool AccordionCtrl::Key(dword key, int count) {
    if(sections.GetCount() == 0) return false;

    int current = -1;
    for(int i = 0; i < sections.GetCount(); i++)
        if(sections[i].header && sections[i].header->HasFocusDeep()) { current = i; break; }

    switch(key) {
        case K_DOWN: if(current >= 0 && current < sections.GetCount() - 1) { sections[current + 1].header->SetFocus(); return true; } break;
        case K_UP:   if(current > 0) { sections[current - 1].header->SetFocus(); return true; } break;
        case K_HOME: sections[0].header->SetFocus(); return true;
        case K_END:  sections[sections.GetCount() - 1].header->SetFocus(); return true;
        case K_SPACE:
        case K_ENTER:
            if(current >= 0) { Toggle(current, true); return true; }
            break;
    }
    return Ctrl::Key(key, count);
}

void AccordionCtrl::MouseMove(Point p, dword) {
    if(HasCapture()) {
        int hit = HitTestHeader(p);
        bool nowPressed = (hit == pressedSection);
        if(pressedSection >= 0 && sections[pressedSection].pressed != nowPressed) {
            sections[pressedSection].pressed = nowPressed;
            RefreshSection(pressedSection);
        }
    }
}

void AccordionCtrl::MouseLeave() {
    if(hotSection >= 0 && !HasCapture()) {
        sections[hotSection].hot = false;
        RefreshSection(hotSection);
        hotSection = -1;
    }
}

void AccordionCtrl::LeftUp(Point p, dword) {
    if(!HasCapture()) return;
    int hit = HitTestHeader(p);
    if(pressedSection >= 0) {
        bool inside = (hit == pressedSection);
        sections[pressedSection].pressed = false;
        RefreshSection(pressedSection);
        if(inside) Toggle(pressedSection, true);
        pressedSection = -1;
    }
    ReleaseCapture();
}

// --- HeaderPane -> owner ---
void AccordionCtrl::OnHeaderLeftDown(int i) {
    if(i < 0 || i >= sections.GetCount()) return;
    pressedSection = i;
    sections[i].pressed = true;
    RefreshSection(i);
    SetCapture();
}

void AccordionCtrl::OnHeaderMouseMove(int i) {
    if(i != hotSection) {
        if(hotSection >= 0) { sections[hotSection].hot = false; RefreshSection(hotSection); }
        hotSection = i;
        if(hotSection >= 0) { sections[hotSection].hot = true; RefreshSection(hotSection); }
    }
    // If user pressed, but moved out and released elsewhere, LeftUp handler resolves it.
}

void AccordionCtrl::OnHeaderMouseLeave(int i) {
    if(!HasCapture() && hotSection == i) {
        sections[hotSection].hot = false;
        RefreshSection(hotSection);
        hotSection = -1;
    }
}

int AccordionCtrl::HitTestHeader(Point p) const {
    for(int i = 0; i < sections.GetCount(); i++)
        if(sections[i].headerRect.Contains(p)) return i;
    return -1;
}

int AccordionCtrl::GetBodyMinHeight(int i) const {
    ASSERT(i >= 0 && i < sections.GetCount());
    const Section& s = sections[i];
    int maxBottom = 0;
    if(s.body) for(Ctrl* c = s.body->GetFirstChild(); c; c = c->GetNext())
        maxBottom = max(maxBottom, c->GetRect().bottom);
    return max(0, maxBottom + 8);
}

void AccordionCtrl::StopAnimation(int i) { ASSERT(i>=0&&i<sections.GetCount()); sections[i].animating=false; sections[i].animTicket++; }

void AccordionCtrl::StartAnimation(int i, int targetHeight, int duration_ms) {
    ASSERT(i >= 0 && i < sections.GetCount());
    StopAnimation(i);
    sections[i].animating   = true;
    sections[i].targetBodyCy = targetHeight;
    int ticket = ++sections[i].animTicket;
    // Store duration in targetBodyCy high bits? Simpler: capture locally.
    SetTimeCallback(16, [=] { AnimTick(i, ticket); }); // tick uses animOpen/Close based on sign of delta
    // We’ll keep duration in a lambda-captured value by computing per step from current delta sign.
    // See AnimTick below.
}

void AccordionCtrl::AnimTick(int i, int ticket) {
    if(i < 0 || i >= sections.GetCount()) return;
    if(sections[i].animTicket != ticket)  return;

    Section& s = sections[i];
    int delta = s.targetBodyCy - s.currentBodyCy;

    if(abs(delta) <= 2) {
        s.currentBodyCy = s.targetBodyCy;
        s.animating = false;
        RefreshLayout();
        return;
    }

    // Choose duration by direction
    const int duration_ms = (delta > 0 ? animOpenMs : animCloseMs);
    double t = 16.0 / max(1, duration_ms);   // fraction per frame
    s.currentBodyCy += int(delta * t);

    RefreshLayout();
    SetTimeCallback(16, [=] { AnimTick(i, ticket); });
}

void AccordionCtrl::EnsureAtLeastOneOpen(int skip) {
    if(!enforceOne) return;
    bool anyOpen = false;
    for(int i = 0; i < sections.GetCount(); i++)
        if(i != skip && sections[i].open) { anyOpen = true; break; }
    if(!anyOpen && sections.GetCount() > 0) {
        int toOpen = (skip == 0 && sections.GetCount() > 1) ? 1 : 0;
        Open(toOpen, false);
    }
}

void AccordionCtrl::CloseOthers(int keep) {
    for(int i = 0; i < sections.GetCount(); i++) {
        if(i == keep) continue;
        if(sections[i].open && sections[i].lock != LOCKED_OPEN)
            Close(i, false);
    }
}

void AccordionCtrl::RefreshSection(int i) {
    if(i >= 0 && i < sections.GetCount()) {
        Refresh(sections[i].headerRect);
        Refresh(sections[i].bodyRect);
    }
}

void AccordionCtrl::UpdateHeaderIndices() {
    for(int i = 0; i < sections.GetCount(); i++)
        if(sections[i].header) {
            sections[i].header->owner = this;
            sections[i].header->index = i;
        }
}

AccordionCtrl& AccordionCtrl::SetAnimationEnabled(bool on) {
    animEnabled = on;
    return *this;
}

AccordionCtrl& AccordionCtrl::SetAnimationDurations(int open_ms, int close_ms) {
    animOpenMs  = max(0, open_ms);
    animCloseMs = max(0, close_ms);
    return *this;
}

AccordionCtrl& AccordionCtrl::SetLocked(int i, bool lock) {
    ASSERT(i >= 0 && i < sections.GetCount());
    Section& s = sections[i];
    const bool was_open = s.open;

    // Lock in current state only (no auto open/close here)
    s.lock = lock ? (was_open ? LOCKED_OPEN : LOCKED_CLOSED) : UNLOCKED;

    // Single-open reconciliation:
    // If we just UNLOCKED an OPEN section and another section is open,
    // close this one to restore the single-open invariant.
    if(!lock && singleExpand && was_open) {
        bool other_open = false;
        for(int j = 0; j < sections.GetCount(); ++j) {
            if(j != i && sections[j].open) { other_open = true; break; }
        }
        if(other_open)
            Close(i, /*animate*/ true);
    }

    RefreshSection(i);
    return *this;
}

AccordionCtrl& AccordionCtrl::LockOpen(int i, bool lock) {
    ASSERT(i >= 0 && i < sections.GetCount());
    sections[i].lock = lock ? LOCKED_OPEN : UNLOCKED;
    if(lock) { sections[i].open = true; sections[i].currentBodyCy = GetBodyMinHeight(i); sections[i].targetBodyCy = sections[i].currentBodyCy; }
    RefreshLayout();
    return *this;
}

AccordionCtrl& AccordionCtrl::LockClosed(int i, bool lock) {
    ASSERT(i >= 0 && i < sections.GetCount());
    sections[i].lock = lock ? LOCKED_CLOSED : UNLOCKED;
    if(lock) { sections[i].open = false; sections[i].currentBodyCy = 0; sections[i].targetBodyCy = 0; }
    RefreshLayout();
    return *this;
}

bool AccordionCtrl::IsLocked(int i) const {
    ASSERT(i >= 0 && i < sections.GetCount());
    return sections[i].lock != UNLOCKED;
}

AccordionCtrl& AccordionCtrl::OpenAll(bool animate) {
    for(int i = 0; i < sections.GetCount(); i++)
        if(sections[i].lock != LOCKED_CLOSED) Open(i, animate);
    return *this;
}

AccordionCtrl& AccordionCtrl::CloseAll(bool animate) {
    for(int i = 0; i < sections.GetCount(); i++)
        if(sections[i].lock != LOCKED_OPEN) Close(i, animate);
    return *this;
}



}