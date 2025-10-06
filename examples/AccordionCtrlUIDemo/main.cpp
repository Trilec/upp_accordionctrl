#include <CtrlLib/CtrlLib.h>
#include <AccordionCtrl/AccordionCtrl.h>
#include <Painter/Painter.h>

using namespace Upp;

// =====================================================
// ShowcaseCard  (RoundedRectangle via BufferPainter)
// =====================================================
class ShowcaseCard : public Ctrl {
public:
    ShowcaseCard& SetTitle(const String& s, Font f = StdFont().Bold().Height(22)) { title = s; titleFont = f; Refresh(); return *this; }
    ShowcaseCard& SetBody (const String& s, Font f = StdFont().Height(12))        { body  = s; bodyFont  = f; Refresh(); return *this; }
    ShowcaseCard& SetBadge(const String& s, Font f = StdFont().Height(12))        { badge = s; badgeFont = f; Refresh(); return *this; }

    ShowcaseCard& EnableFrame(bool on = true)  { showFrame = on; Refresh(); return *this; }
    ShowcaseCard& EnableFill(bool on = true)   { fillBody  = on; Refresh(); return *this; }
    ShowcaseCard& SetRadius(int px)            { radius    = max(2, px); Refresh(); return *this; }
    ShowcaseCard& SetPadding(int x, int y)     { padx = x; pady = y; Refresh(); return *this; }
    ShowcaseCard& SetColors(Color face, Color stroke,
                            Color tInk = SColorText(),
                            Color bInk = SColorText(),
                            Color gInk = SColorDisabled())
    { bg = face; border = stroke; titleInk = tInk; bodyInk = bInk; badgeInk = gInk; Refresh(); return *this; }

    virtual Size GetMinSize() const override { return Size(DPI(220), DPI(90)); }

private:
    // content
    String title, body, badge;

    // style
    int   radius   = DPI(12);
    int   padx     = DPI(12);
    int   pady     = DPI(10);
    int   gapTitle = DPI(4);
    bool  showFrame = true;
    bool  fillBody  = true;

    // fonts/inks
    Font  titleFont = StdFont().Bold().Height(22);
    Font  bodyFont  = StdFont().Height(12);
    Font  badgeFont = StdFont().Height(12);
    Color titleInk  = SColorText();
    Color bodyInk   = SColorText();
    Color badgeInk  = SColorDisabled();
    Color bg        = Blend(SColorPaper(), SColorFace(), 32);
    Color border    = SColorShadow();

    static String WrapToWidth(const String& s, Font f, int w) {
        if(IsNull(s) || w <= 8) return s;
        Vector<String> words = Split(s, CharFilterWhitespace);
        String out, line;
        for(const String& wrd : words) {
            String probe = line.IsEmpty() ? wrd : line + ' ' + wrd;
            if(GetTextSize(probe, f).cx <= w || line.IsEmpty())
                line = probe;
            else {
                if(!out.IsEmpty()) out << '\n';
                out << line;
                line = wrd;
            }
        }
        if(!line.IsEmpty()) {
            if(!out.IsEmpty()) out << '\n';
            out << line;
        }
        return out;
    }

    virtual void Paint(Draw& w) override {
	    const Size sz = GetSize();
	
	    // --- Transparent offscreen ---
	    ImageBuffer ib(sz);
	    RGBA *px = ~ib;
	    for(int i = 0; i < ib.GetLength(); ++i)
	        px[i] = RGBAZero();              // (0,0,0,0) premultiplied transparent
	
	    {
	        BufferPainter p(ib);             // AA paths on the buffer
	
	        if(fillBody) {
	            p.Begin();
	            p.RoundedRectangle(0.5, 0.5, sz.cx - 1.0, sz.cy - 1.0,
	                               (double)radius, (double)radius);
	            p.End();
	            p.Fill(bg);                  // can be opaque or semi-transparent
	        }
	
	        if(showFrame) {
	            p.Begin();
	            p.RoundedRectangle(1.0, 1.0, sz.cx - 2.0, sz.cy - 2.0,
	                               (double)radius, (double)radius);
	            p.End();
	            p.Stroke(2.0, border);
	        }
	    }
	
	    // Composite with alpha (no hairlines, no SetKind needed)
	    w.DrawImage(0, 0, ib);
	
	    // --- Foreground text ---
	    Rect c = Rect(sz).Deflated(padx, pady);
	
	    const int titleLH = titleFont.GetLineHeight();
	    const int badgeLH = badgeFont.GetLineHeight();
	    const int lineH   = max(titleLH, badgeLH);
	
	    int y = c.top;
	
	    if(!IsNull(badge)) {
	        const Size bs = GetTextSize(badge, badgeFont);
	        w.DrawText(c.right - bs.cx, y + (lineH - badgeLH) / 2,
	                   badge, badgeFont, badgeInk);
	    }
	
	    w.DrawText(c.left, y + (lineH - titleLH) / 2, title, titleFont, titleInk);
	    y += lineH + gapTitle;
	
	    if(!IsNull(body)) {
	        const String wrapped = WrapToWidth(body, bodyFont, c.GetWidth());
	        const int lh = bodyFont.GetLineHeight();
	        for(const String& line : Split(wrapped, '\n')) {
	            w.DrawText(c.left, y, line, bodyFont, bodyInk);
	            y += lh;
	            if(y > c.bottom) break;
	        }
	    }
	}

};

struct AccordionDemo : TopWindow {
    typedef AccordionDemo CLASSNAME;

    ShowcaseCard card;
    // Controls row under the card
    ParentCtrl  toolbar;
    Option      optSingle;
    Option      optAnim;
    Button      btnOpenAll;
    Button      btnCloseAll;

    AccordionCtrl accordion;

    // Locks on headers
    Option      lock0, lock1, lock2;

    // Section bodies (a bit richer)
    Label       lblName, lblTuning, lblColor, lblAbout,lblScale;
    EditString  editName;
    Option      optEnable, optAuto;
    SliderCtrl  sliderValue;
    DropList    dropTheme, dropScale;
    ColorPusher colorPicker;

    AccordionDemo() {
        Title("Accordion Control Showcase — U++ 2025.1");
        Sizeable().Zoomable();
        SetRect(0, 0, 760, 540);

        // Showcase card
        card.SetTitle("Accordion Control Showcase")
            .SetBody("Testing multi vs single expand, per-section locks, header widgets, and animation controls. "
                     "Toggle options below and interact with headers or embedded widgets.")
            .SetBadge("Demo")
            .EnableFrame(false)
            .SetPadding(DPI(12), DPI(10))
            .SetRadius(DPI(12));
        Add(card.HSizePos(8, 8).TopPos(8, DPI(106)));

        // Toolbar row
        Add(toolbar.HSizePos(8, 8).TopPos(DPI(122), 28));
        toolbar.Add(optSingle.LeftPos(0, 160).VCenterPos(20));
        optSingle.SetLabel("Single open mode");
        optSingle <<= true;

        toolbar.Add(optAnim.LeftPos(170, 150).VCenterPos(20));
        optAnim.SetLabel("Enable animation");
        optAnim <<= true;

        toolbar.Add(btnOpenAll.LeftPos(340, 100).VCenterPos(24));
        btnOpenAll.SetLabel("Open all");

        toolbar.Add(btnCloseAll.LeftPos(450, 100).VCenterPos(24));
        btnCloseAll.SetLabel("Close all");

        // Accordion fills the rest
        Add(accordion.HSizePos(8, 8).VSizePos(DPI(156), 8));
        accordion.SingleExpand(true).AtLeastOneOpen(false);
        accordion.SetAnimationEnabled(true).SetAnimationDurations(160, 80); // close faster

        // Section 0
        int s0 = accordion.AddSection("General Settings");
        Ctrl& h0 = accordion.HeaderCtrl(s0);
        ParentCtrl& b0 = accordion.BodyCtrl(s0);
        lock0.SetLabel("Lock");
        h0.Add(lock0.RightPos(8, 80).VCenterPos(20));

        lblName.SetLabel("Name:");
        editName.SetData("Jane Doe");
        optEnable.SetLabel("Enable feature"); optEnable.Set(true);
        b0.Add(lblName.LeftPos(8, 80).TopPos(8, 20));
        b0.Add(editName.LeftPos(96, 220).TopPos(8, 24));
        b0.Add(optEnable.LeftPos(8, 150).TopPos(40, 20));

        // Section 1
        int s1 = accordion.AddSection("Advanced Options");
        Ctrl& h1 = accordion.HeaderCtrl(s1);
        ParentCtrl& b1 = accordion.BodyCtrl(s1);
        Button btnReset; btnReset.SetLabel("Reset");
        optAuto.SetLabel("Auto");
        h1.Add(btnReset.RightPos(96, 80).VCenterPos(22));
        h1.Add(optAuto.RightPos(8, 80).VCenterPos(20));
        lock1.SetLabel("Lock"); h1.Add(lock1.RightPos(188, 60).VCenterPos(20));
        lblTuning.SetLabel("Tuning parameter:");
        sliderValue.MinMax(0,100).SetData(50);
        b1.Add(lblTuning.LeftPos(8, 150).TopPos(8, 20));
        b1.Add(sliderValue.LeftPos(8, 280).TopPos(32, 24));

        // Section 2
        int s2 = accordion.AddSection("Appearance");
        Ctrl& h2 = accordion.HeaderCtrl(s2);
        ParentCtrl& b2 = accordion.BodyCtrl(s2);
        lock2.SetLabel("Lock"); h2.Add(lock2.RightPos(8, 60).VCenterPos(20));
        lblColor.SetLabel("Theme:");
        dropTheme.Add("Light").Add("Dark").Add("Auto"); dropTheme.SetIndex(0);
        dropScale.Add("100%").Add("125%").Add("150%").SetIndex(0);
        colorPicker.SetData(Color(64,128,255));
        b2.Add(lblColor.LeftPos(8, 80).TopPos(8, 20));
        b2.Add(dropTheme.LeftPos(96, 150).TopPos(8, 24));
        b2.Add(colorPicker.LeftPos(8, 100).TopPos(40, 24));
		lblScale.SetLabel("Scale:");  
		b2.Add(lblScale.LeftPos(8, 80).TopPos(72, 20));

        b2.Add(dropScale.LeftPos(96, 100).TopPos(72, 24));

        // Section 3
        int s3 = accordion.AddSection("About");
        ParentCtrl& b3 = accordion.BodyCtrl(s3);
        lblAbout.SetLabel("AccordionCtrl — collapsible sections with Chameleon styling.\n"
                          "- Whole header area toggles (widgets remain interactive)\n"
                          "- Single/Multi expand\n"
                          "- Per-section lock in current state\n"
                          "- Keyboard: Up/Down/Home/End + Space/Enter\n"
                          "- Animation configurable; close is faster\n");
        b3.Add(lblAbout.LeftPos(8, 480).TopPos(8, 120));

        // Default open first
        accordion.Open(0, false);

        // Wire toolbar
        optSingle.WhenAction = [=] {
            accordion.SingleExpand((bool)~optSingle);
            btnOpenAll.Enable(!optSingle); // only meaningful in multi
        };
        optAnim.WhenAction = [=] { accordion.SetAnimationEnabled((bool)~optAnim); };
        btnOpenAll.WhenAction = [=] { accordion.OpenAll(true); };
        btnCloseAll.WhenAction = [=] { accordion.CloseAll(true); };

        // Lock checkboxes — lock in current state
        auto apply_lock = [=](int idx, const Option& opt) {
            accordion.SetLocked(idx, (bool)~opt);
        };
        lock0.WhenAction = [=] { apply_lock(s0, lock0); };
        lock1.WhenAction = [=] { apply_lock(s1, lock1); };
        lock2.WhenAction = [=] { apply_lock(s2, lock2); };

        // Header widget actions
        btnReset.SetLabel("Reset");  
		h1.Add(btnReset.RightPos(96, 80).VCenterPos(22));  
		btnReset.WhenAction = [=] { sliderValue <<= 50; optAuto <<= false; PromptOK("Advanced reset."); };
    }
};

GUI_APP_MAIN { AccordionDemo().Run(); }