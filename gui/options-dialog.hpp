#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>
#include "ui_settings.h"
#include "tracker-pt/ftnoir_tracker_pt_settings.h"
#include "trans_calib.h"
#include "tracker-pt/ftnoir_tracker_pt.h"
#include "filter-accela/ftnoir_filter_accela.h"
#include "opentrack-logic/shortcuts.h"
#include "opentrack-logic/state.hpp"
#include <QObject>
#include <QWidget>
#include <functional>

class OptionsDialog: public QWidget
{
    Q_OBJECT
signals:
    void saving();
public:
    OptionsDialog(std::function<void(bool)> pause_keybindings, State& state);
private:
    main_settings main;
    std::function<void(bool)> pause_keybindings;
    Ui::UI_Settings ui;
    settings_pt pt;
    settings_accela acc;
    QTimer timer;
    TranslationCalibrator trans_calib;
    State& state;
    bool trans_calib_running;

    Tracker_PT* get_pt();
    void closeEvent(QCloseEvent *) override { doCancel(); }
private slots:
    void update_ewma_display(int value);
    void update_rot_display(int value);
    void update_trans_display(int value);
    void update_rot_dz_display(int value);
    void update_trans_dz_display(int value);

    void doOK();
    void doCancel();
    void startstop_trans_calib(bool start);
    void poll_tracker_info();
    void trans_calib_step();
    void bind_key(key_opts &kopts, QLabel* label);
};
