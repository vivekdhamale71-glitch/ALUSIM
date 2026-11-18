// =====================================================================
//  The six screens of the application. Each view is a class behind
//  the abstract View interface; the App only talks to that interface.
// =====================================================================
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "core/Arithmetic.h"
#include "engine/ds/HashTable.h"
#include "engine/ds/Searching.h"
#include "engine/ds/Sorting.h"
#include "app/Sim.h"
#include "app/UI.h"

namespace vas::ui {

class View {
public:
    virtual ~View() = default;
    virtual const char* name() const = 0;
    virtual Icon icon() const = 0;
    virtual void draw(const Rect& area, Sim& sim, UI& ui) = 0;
    virtual void setSub(int) {}   // select a sub-mode (used by --sub for screenshots)
};

// ------------------------------------------------------------------ helpers
Highlighter statementHighlighter();
Highlighter asmHighlighter();
Highlighter englishHighlighter();
void drawAsmLine(float x, float y, const std::string& line, float alpha = 1.0f, int px = 14);
std::string hex64(std::uint64_t v, bool grouped = false);
void drawRegisterPanel(const Rect& r, Sim& sim, UI& ui);

// ------------------------------------------------------------------ views
class CompileView : public View {
public:
    const char* name() const override { return "Compiler"; }
    Icon icon() const override { return Icon::Code; }
    void draw(const Rect& area, Sim& sim, UI& ui) override;

private:
    void drawTokens(const Rect& r, Sim& sim, UI& ui, int upto);
    void drawEnglish(const Rect& r, Sim& sim, UI& ui);
    float transT_ = 0;       // progress of the English -> code translation animation
    void drawParser(const Rect& r, Sim& sim, UI& ui, int ev);
    void drawAssembly(const Rect& r, Sim& sim, UI& ui, int ev);
    void drawSymbols(const Rect& r, Sim& sim, UI& ui, int ev);

    EditorState editor_;
    bool sampleOpen_ = false;   // the examples drop-down list is open
    int seenGen_ = -1;
    float evTime_ = 0;       // time inside the current timeline event
    int ev_ = 0;             // current timeline event index
    bool playing_ = true;
    float speed_ = 1.0f;
    std::vector<float> tokenBorn_;
};

class DatapathView : public View {
public:
    const char* name() const override { return "Datapath"; }
    Icon icon() const override { return Icon::Chip; }
    void draw(const Rect& area, Sim& sim, UI& ui) override;

private:
    void drawDiagram(const Rect& r, Sim& sim, UI& ui);
    void drawInfo(const Rect& r, Sim& sim, UI& ui);
    void drawWaveform(const Rect& r, Sim& sim, UI& ui);
    void drawControlDetail(const Rect& r, Sim& sim, UI& ui);
    float zoom_ = 1.0f;
    gfx::Vec2 pan_{0, 0};
    bool dragging_ = false;
    gfx::Vec2 dragStart_, panStart_;
    float unitGlow_[16] = {};
};

class PipelineView : public View {
public:
    const char* name() const override { return "Pipeline"; }
    Icon icon() const override { return Icon::Layers; }
    void draw(const Rect& area, Sim& sim, UI& ui) override;

private:
    bool forwarding_ = true;
    bool singlePort_ = false;
    float cycle_ = 0;
    bool playing_ = true;
    float scroll_ = 0;
    int seenGen_ = -1;
};

class ArithmeticView : public View {
public:
    const char* name() const override { return "Arithmetic"; }
    Icon icon() const override { return Icon::Grid; }
    void draw(const Rect& area, Sim& sim, UI& ui) override;
    void setSub(int m) override { mode_ = m; }

private:
    int mode_ = 0;
    int a_ = 7, b_ = -3;
    int ua_ = 13, ub_ = 11;
    int dividend_ = 100, divisor_ = 7;
    int bits_ = 8;
    core::ArithTrace trace_;
    std::string key_;
    float t_ = 0;
    bool playing_ = true;
    int shown_ = 0;
};

class DataLabView : public View {
public:
    const char* name() const override { return "Data Lab"; }
    Icon icon() const override { return Icon::Database; }
    void draw(const Rect& area, Sim& sim, UI& ui) override;
    void setSub(int m) override { mode_ = m; }

private:
    void drawHashing(const Rect& r, Sim& sim, UI& ui);
    void drawSorting(const Rect& r, Sim& sim, UI& ui);
    void drawSearching(const Rect& r, Sim& sim, UI& ui);
    int mode_ = 0;

    // hashing
    int hashMethod_ = 0;
    std::unique_ptr<ds::HashTable> table_;
    std::vector<std::string> keys_;
    std::vector<ds::ProbeTrace> traces_;
    std::vector<std::vector<ds::Bucket>> snapshots_;
    std::string hashKey_;
    float hashT_ = 0;
    int hashShown_ = 0;

    // sorting
    int sortAlgo_ = 0;
    std::vector<std::string> sortLabels_;
    std::vector<int> sortData_, sortWork_;
    std::vector<ds::SortStep> sortSteps_;
    std::string sortKey_;
    float sortT_ = 0;
    int sortPos_ = 0;
    bool sortPlaying_ = true;
    float sortSpeed_ = 1.0f;

    // searching
    int searchAlgo_ = 1;
    int target_ = 0;
    std::vector<int> searchData_;
    std::vector<std::string> searchLabels_;
    std::vector<ds::SearchStep> searchSteps_;
    std::string searchKey_;
    float searchT_ = 0;
    int searchPos_ = 0;
};

class ChipView : public View {
public:
    const char* name() const override { return "Parse & Chip"; }
    Icon icon() const override { return Icon::Cube; }
    void draw(const Rect& area, Sim& sim, UI& ui) override;
    void configure(int mode, bool flat) { mode_ = mode; want2D_ = flat; flat_ = flat ? 1.f : 0.f; }

    // expression tree of one statement, built from the compiler's postfix
    struct TreeNode {
        std::string label;
        int kind = 0;            // 0 operator, 1 variable, 2 number, 3 assign root, 4 relop root
        int left = -1, right = -1;
        int born = 0;            // build step in which the node appears
        int consumed = -1;       // build step in which its parent takes it
        int slot = 0;            // stack slot while it waits on the stack
        float tx = 0, ty = 0;    // final position in the tree
        std::vector<int> lines;  // code lines generated for this node
        std::string tag;         // where its value lives (rbx / [a] / 5)
        std::string note;        // code-generation explanation
    };
    struct ParseScene {
        std::string title;
        std::vector<TreeNode> nodes;
        std::vector<int> lines;  // every code line of the statement
        int root = -1;
        float width = 1;
    };

private:
    void initTextures();
    void drawChip(const Rect& vp, Sim& sim, UI& ui);
    void drawParse(const Rect& vp, Sim& sim, UI& ui);
    void buildScenes(Sim& sim);
    void applyCamera(const Rect& vp, float yaw, float pitch, float dist, float flatYaw, float flatPitch, float tx,
                     float ty, float tz);

    int mode_ = 0;               // 0 = parse tree, 1 = chip
    bool want2D_ = false;
    float flat_ = 0;             // 0 = 3D perspective ... 1 = flat orthographic 2D
    std::vector<ParseScene> scenes_;
    int scene_ = 0;
    float sceneT_ = 0;
    bool scenePlaying_ = true;
    int seenGen_ = -1;
    float hold_ = 0;
    float pyaw_ = 22, ppitch_ = 14, pdist_ = 16;
    float yaw_ = -32, pitch_ = 52, dist_ = 21;
    bool perspective_ = true;
    bool autoRotate_ = true;
    bool dragging_ = false;
    gfx::Vec2 last_;
    unsigned boardTex_ = 0;
    float glow_[16] = {};
    float pulse_ = 0;
};

} // namespace vas::ui
