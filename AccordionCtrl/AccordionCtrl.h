#ifndef _AccordionCtrl_h_
#define _AccordionCtrl_h_

#include <CtrlLib/CtrlLib.h>

namespace Upp {

// AccordionCtrl — collapsible section container with Chameleon styling
class AccordionCtrl : public Ctrl {
public:
    typedef AccordionCtrl CLASSNAME;

    struct Style : ChStyle<Style> {
        Value  headerLook;
        Value  bodyLook;
        Color  headerInk;
        Color  headerInkHover;
        Color  headerBgHover;
        Color  dividerColor;
        Color  borderColor;
        Font   headerFont;
        int    headerCy            = 28;
        int    headerLPad          = 8;
        int    headerRPad          = 8;
        int    headerSpacing       = 6;
        int    dividerThick        = 1;
        int    iconCx              = 12;
        int    iconTextGap         = 6;
        int    sectionVGap         = 2;
        int    borderWidth         = 0;
        int    animMs              = 120;
        bool   focusHeaderOnToggle = true;
    };

    static const Style& StyleDefault();

    AccordionCtrl();

    // Style
    AccordionCtrl&     SetStyle(const Style& st);
    const Style&       GetStyle() const;

    // Section management
    int                GetCount() const;
    int                AddSection(const String& title);
    int                InsertSection(int at, const String& title);
    void               RemoveSection(int i);
    void               Clear();

    // Access to section containers
    Ctrl&              HeaderCtrl(int i);
    ParentCtrl&        BodyCtrl(int i);

    // State
    bool               IsOpen(int i) const;
    void               Open(int i, bool animate = true);
    void               Close(int i, bool animate = true);
    void               Toggle(int i, bool animate = true);

    // Modes
    AccordionCtrl&     SingleExpand(bool b = true);
    AccordionCtrl&     AtLeastOneOpen(bool b = true);

    // Header formatting
    AccordionCtrl&     SetTitle(int i, const String& title);
    AccordionCtrl&     SetHeaderAlign(int i, int align);
    AccordionCtrl&     UseDivider(int i, bool use = true);

    // Icons
    AccordionCtrl&     SetIcons(Image collapsed, Image expanded);

    // Animation
    AccordionCtrl&     SetAnimationMs(int ms);
	AccordionCtrl&     SetAnimationEnabled(bool on = true);  
	AccordionCtrl&     SetAnimationDurations(int open_ms, int close_ms); // close is typically faster

	// Locking API  
	AccordionCtrl&     SetLocked(int i, bool lock);   // lock in current state (open -> locked-open; closed -> locked-closed)  
	AccordionCtrl&     LockOpen(int i, bool lock = true);  
	AccordionCtrl&     LockClosed(int i, bool lock = true);  
	bool               IsLocked(int i) const;         // any section locked?  
	  
	// Bulk ops  
	AccordionCtrl& OpenAll(bool animate = true);  
	AccordionCtrl& CloseAll(bool animate = true);
    // Callbacks
    Event<int>         WhenOpen;
    Event<int>         WhenClose;
    Gate<int>          WhenBeforeToggle;

    // Persistence
    virtual void       Serialize(Stream& s) override;

    // Ctrl overrides
    virtual void       Paint(Draw& w) override;
    virtual void       Layout() override;

protected:
    virtual bool       Key(dword key, int count) override;
    virtual void       MouseMove(Point p, dword keyflags) override;
    virtual void       MouseLeave() override;
    virtual void       LeftUp(Point p, dword keyflags) override; // NEW: finalize clicks even without mouse move

private:
    // Header pane forwards mouse to owner
	struct HeaderPane : ParentCtrl {
	    AccordionCtrl* owner = nullptr;
	    int            index = -1;
	
	    virtual void LeftDown(Point p, dword) override {
	        // If clicking a child inside the header, do not toggle
	        for(Ctrl* c = GetFirstChild(); c; c = c->GetNext()) {
	            if(c->IsVisible() && c->GetRect().Contains(p)) {
	                c->SetFocus();
	                return;
	            }
	        }
	        if(owner) owner->OnHeaderLeftDown(index);
	    }
	    virtual void MouseMove(Point, dword) override {
	        if(owner) owner->OnHeaderMouseMove(index);
	    }
	    virtual void MouseLeave() override {
	        if(owner) owner->OnHeaderMouseLeave(index);
	    }
	};
        
    enum LockMode { UNLOCKED, LOCKED_OPEN, LOCKED_CLOSED };
   
	struct Section {  
	    One<HeaderPane> header;  
	    One<ParentCtrl> body;  
	    String  title;  
	    int     align         = ALIGN_LEFT;  
	    bool    useDivider    = false;  
	    bool    open          = false;  
	    bool    animating     = false;  
	    int     currentBodyCy = 0;  
	    int     targetBodyCy  = 0;  
	    int     animTicket    = 0;  
	    bool    hot           = false;  
	    bool    pressed       = false;  
	    Rect    headerRect;  
	    Rect    bodyRect;  
	    Rect    iconRect;  
	    LockMode lock         = UNLOCKED;  // NEW  
	};

	// Animation knobs  
	bool animEnabled = true;   // on/off at runtime  
	int  animOpenMs  = 160;    // opening duration  
	int  animCloseMs = 80;     // closing duration (2× faster)

    // HeaderPane -> owner handlers
    void              OnHeaderLeftDown(int i);
    void              OnHeaderMouseMove(int i);
    void              OnHeaderMouseLeave(int i);

    // Utilities
    int               HitTestHeader(Point p) const;
    int               GetBodyMinHeight(int i) const;
    void              StopAnimation(int i);
    void              StartAnimation(int i, int targetHeight, int duration_ms);
    void              AnimTick(int i, int ticket);
    void              EnsureAtLeastOneOpen(int skip);
    void              CloseOthers(int keep);
    void              RefreshSection(int i);
    void              UpdateHeaderIndices();

    Array<Section>    sections;
    const Style*      style;
    Image             iconClosed;
    Image             iconOpened;
    Image             iconLock;
    bool              singleExpand;
    bool              enforceOne;
    int               hotSection;
    int               pressedSection;
    
};

}

#endif