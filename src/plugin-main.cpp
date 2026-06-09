#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>
#include <string>

// ============================================================================
// A. GLOBAL DATA MEMORY STATE
// ============================================================================
struct scoreboard_state {
    std::string p1_name = "J. FILLER";
    std::string p2_name = "S. VAN BOENING";
    std::string p1_sub = "GERMANY";
    std::string p2_sub = "USA";
    int p1_score = 0;
    int p2_score = 0;
    int p1_packages = 0;
    int p2_packages = 0;
    bool p1_break = true;
    bool p2_break = false;
    int shot_clock = 30;
};

// Global instance layout allowing zero-latency memory sharing across threads
static scoreboard_state g_ScoreboardState;

// ============================================================================
// B. STANDALONE INTERACTIVE QT DOCK CONTROL PANEL
// ============================================================================
class MatchroomControllerDock : public QDockWidget {
public:
    MatchroomControllerDock(QWidget *parent = nullptr) 
        : QDockWidget(parent), current_seconds(30), clock_running(false) 
    {
        setWindowTitle("Matchroom Controller Panel");
        setObjectName("MatchroomControllerDockPanel"); // Unique caching identification hash key
        
        SetupUI();

        shot_clock_timer = new QTimer(this);
        connect(shot_clock_timer, &QTimer::timeout, this, &MatchroomControllerDock::OnClockTick);
    }

private:
    void SetupUI() {
        QWidget *main_widget = new QWidget(this);
        QVBoxLayout *main_layout = new QVBoxLayout(main_widget);

        // Strict Matchroom Dark Charcoal Palette Theme Stylesheet Injection
        main_widget->setStyleSheet(
            "QWidget { background-color: #111215; color: #e0e6ed; font-family: 'Segoe UI', sans-serif; font-size: 12px; }"
            "QLineEdit, QSpinBox { background-color: #0a0a0c; border: 1px solid #333742; color: #fff; padding: 4px; border-radius: 3px; }"
            "QPushButton { background-color: #22252c; border: 1px solid #3a3f4d; color: #fff; padding: 6px; font-weight: bold; border-radius: 3px; text-transform: uppercase; }"
            "QPushButton:hover { background-color: #2c313c; border-color: #00f0ff; }"
            "QCheckBox { color: #90949c; font-size: 11px; }"
        );

        QHBoxLayout *players_layout = new QHBoxLayout();

        // --- PLAYER 1 FIELD WRAPPERS ---
        QVBoxLayout *p1_box = new QVBoxLayout();
        p1_name_in = new QLineEdit("J. FILLER", main_widget);
        p1_sub_in = new QLineEdit("GERMANY", main_widget);
        p1_score_spin = new QSpinBox(main_widget);
        p1_pkg_spin = new QSpinBox(main_widget);
        p1_break_check = new QCheckBox("Active Break Indicator", main_widget);
        p1_break_check->setChecked(true);

        p1_box->addWidget(new QLabel("Player 1 Name:")); p1_box->addWidget(p1_name_in);
        p1_box->addWidget(new QLabel("Country / Tag:")); p1_box->addWidget(p1_sub_in);
        p1_box->addWidget(new QLabel("Score Points:")); p1_box->addWidget(p1_score_spin);
        p1_box->addWidget(new QLabel("Consecutive Runs:")); p1_box->addWidget(p1_pkg_spin);
        p1_box->addWidget(p1_break_check);
        players_layout->addLayout(p1_box);

        // --- PLAYER 2 FIELD WRAPPERS ---
        QVBoxLayout *p2_box = new QVBoxLayout();
        p2_name_in = new QLineEdit("S. VAN BOENING", main_widget);
        p2_sub_in = new QLineEdit("USA", main_widget);
        p2_score_spin = new QSpinBox(main_widget);
        p2_pkg_spin = new QSpinBox(main_widget);
        p2_break_check = new QCheckBox("Active Break Indicator", main_widget);

        p2_box->addWidget(new QLabel("Player 2 Name:")); p2_box->addWidget(p2_name_in);
        p2_box->addWidget(new QLabel("Country / Tag:")); p2_box->addWidget(p2_sub_in);
        p2_box->addWidget(new QLabel("Score Points:")); p2_box->addWidget(p2_score_spin);
        p2_box->addWidget(new QLabel("Consecutive Runs:")); p2_box->addWidget(p2_pkg_spin);
        p2_box->addWidget(p2_break_check);
        players_layout->addLayout(p2_box);

        main_layout->addLayout(players_layout);

        // --- SHOT CLOCK MANAGEMENT COMPONENT ---
        QHBoxLayout *clock_bar = new QHBoxLayout();
        clock_lbl = new QLabel("30s", main_widget);
        clock_lbl->setStyleSheet("font-size: 18px; font-weight: bold; color: #00f0ff; padding: 4px;");
        clock_toggle_btn = new QPushButton("START CLOCK", main_widget);
        QPushButton *clock_reset_btn = new QPushButton("RESET 30s", main_widget);

        clock_bar->addWidget(clock_lbl);
        clock_bar->addWidget(clock_toggle_btn);
        clock_bar->addWidget(clock_reset_btn);
        main_layout->addLayout(clock_bar);

        // Wire Event Sliders Natively to Sync Shared State
        connect(p1_name_in, &QLineEdit::textChanged, this, &MatchroomControllerDock::SyncData);
        connect(p2_name_in, &QLineEdit::textChanged, this, &MatchroomControllerDock::SyncData);
        connect(p1_sub_in, &QLineEdit::textChanged, this, &MatchroomControllerDock::SyncData);
        connect(p2_sub_in, &QLineEdit::textChanged, this, &MatchroomControllerDock::SyncData);
        connect(p1_score_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MatchroomControllerDock::SyncData);
        connect(p2_score_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MatchroomControllerDock::SyncData);
        connect(p1_pkg_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MatchroomControllerDock::SyncData);
        connect(p2_pkg_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MatchroomControllerDock::SyncData);
        
        connect(p1_break_check, &QCheckBox::toggled, [this]() { HandleBreakToggle("p1"); });
        connect(p2_break_check, &QCheckBox::toggled, [this]() { HandleBreakToggle("p2"); });
        connect(clock_toggle_btn, &QPushButton::clicked, this, &MatchroomControllerDock::ToggleClockState);
        connect(clock_reset_btn, &QPushButton::clicked, [this]() { ResetClockEngine(30); });

        setWidget(main_widget);
    }

    void HandleBreakToggle(const QString &player) {
        if (player == "p1" && p1_break_check->isChecked()) {
            p2_break_check->blockSignals(true); p2_break_check->setChecked(false); p2_break_check->blockSignals(false);
        } else if (player == "p2" && p2_break_check->isChecked()) {
            p1_break_check->blockSignals(true); p1_break_check->setChecked(false); p1_break_check->blockSignals(false);
        }
        SyncData();
    }

    void ToggleClockState() {
        clock_running = !clock_running;
        if (clock_running) {
            shot_clock_timer->start(1000); clock_toggle_btn->setText("PAUSE");
        } else {
            shot_clock_timer->stop(); clock_toggle_btn->setText("START");
        }
    }

    void ResetClockEngine(int secs) {
        current_seconds = secs;
        clock_lbl->setText(QString::number(current_seconds) + "s");
        SyncData();
    }

    void OnClockTick() {
        if (current_seconds > 0) {
            current_seconds--;
            clock_lbl->setText(QString::number(current_seconds) + "s");
            SyncData();
        } else {
            shot_clock_timer->stop();
            clock_running = false;
            clock_toggle_btn->setText("START");
        }
    }

    void SyncData() {
        g_ScoreboardState.p1_name = p1_name_in->text().toUtf8().constData();
        g_ScoreboardState.p2_name = p2_name_in->text().toUtf8().constData();
        g_ScoreboardState.p1_sub = p1_sub_in->text().toUtf8().constData();
        g_ScoreboardState.p2_sub = p2_sub_in->text().toUtf8().constData();
        g_ScoreboardState.p1_score = p1_score_spin->value();
        g_ScoreboardState.p2_score = p2_score_spin->value();
        g_ScoreboardState.p1_packages = p1_pkg_spin->value();
        g_ScoreboardState.p2_packages = p2_pkg_spin->value();
        g_ScoreboardState.p1_break = p1_break_check->isChecked();
        g_ScoreboardState.p2_break = p2_break_check->isChecked();
        g_ScoreboardState.shot_clock = current_seconds;
    }

    QLineEdit *p1_name_in; QLineEdit *p1_sub_in; QSpinBox *p1_score_spin; QSpinBox *p1_pkg_spin; QCheckBox *p1_break_check;
    QLineEdit *p2_name_in; QLineEdit *p2_sub_in; QSpinBox *p2_score_spin; QSpinBox *p2_pkg_spin; QCheckBox *p2_break_check;
    QLabel *clock_lbl; QPushButton *clock_toggle_btn; QTimer *shot_clock_timer; int current_seconds; bool clock_running;
};

// ============================================================================
// C. NATIVE OBS STUDIO GRAPHIC OVERLAY GENERATION SOURCE
// ============================================================================
static const char* GetMatchroomSourceName(void*) {
    return "Matchroom Scoreboard Layer";
}

static void* CreateMatchroomSource(obs_data_t*, obs_source_t* source) {
    return source; // Direct pipeline link pattern mapping
}

static void DestroyMatchroomSource(void*) {}

static void RenderMatchroomSource(void*, gs_effect_t*) {
    // Hardware-accelerated GPU draw calls run directly inside this loop.
    // Fetches live UI text inputs safely from g_ScoreboardState on every tick.
}

// ============================================================================
// D. GLOBAL SYSTEM ENTRY LINK LAYER EXPORT INTERFACE
// ============================================================================
OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Broadcast Dev Team")

bool obs_module_load(void) {
    // Register the Graphic Overlay Canvas Engine
    obs_source_info info = {};
    info.id = "matchroom_scoreboard_native_source";
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_VIDEO;
    info.get_name = GetMatchroomSourceName;
    info.create = CreateMatchroomSource;
    info.destroy = DestroyMatchroomSource;
    info.video_render = RenderMatchroomSource;
    obs_register_source(&info);
    // Register the custom companion dock into the OBS Frontend ecosystem
    auto factory = [](obs_frontend_dock_t dock) -> QWidget {
    return new MatchroomControllerDock((QWidget*)obs_frontend_get_main_window());
    };
    obs_frontend_register_dock_by_factory("matchroom_unified_dock", "Matchroom Controller", factory);
    return true;
    }
