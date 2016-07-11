/* Copyright (c) 2014-2015, Stanislaw Halik <sthalik@misaki.pl>

 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#pragma once

#include <QMainWindow>
#include <QKeySequence>
#include <QShortcut>
#include <QPixmap>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QString>
#include <QMenu>

#include <vector>
#include <tuple>

#include "ui_main.h"

#include "opentrack-compat/options.hpp"
#include "opentrack-logic/main-settings.hpp"
#include "opentrack/plugin-support.hpp"
#include "opentrack-logic/tracker.h"
#include "opentrack-logic/shortcuts.h"
#include "opentrack-logic/work.hpp"
#include "opentrack-logic/state.hpp"
#include "curve-config.h"
#include "options-dialog.hpp"
#include "process_detector.h"
#include "software-update-dialog.hpp"

using namespace options;

class MainWindow : public QMainWindow, public State
{
    Q_OBJECT

    Shortcuts global_shortcuts;
    module_settings m;
    Ui::OpentrackUI ui;
    mem<QSystemTrayIcon> tray;
    QTimer pose_update_timer;
    QTimer det_timer;
    QTimer config_list_timer;
    mem<OptionsDialog> options_widget;
    mem<MapWidget> mapping_widget;
    QShortcut kbd_quit;
    mem<IProtocolDialog> pProtocolDialog;
    process_detector_worker det;
    QMenu profile_menu;
    bool is_refreshing_profiles;
    volatile bool keys_paused;
    update_dialog::query update_query;

    mem<dylib> current_protocol()
    {
        return modules.protocols().value(ui.iconcomboProtocol->currentIndex(), nullptr);
    }

    void changeEvent(QEvent* e) override;

    void load_settings();
    void load_mappings();
    void updateButtonState(bool running, bool inertialp);
    void display_pose(const double* mapped, const double* raw);
    void ensure_tray();
    void set_title(const QString& game_title = QStringLiteral(""));
    static bool get_new_config_name_from_dialog(QString &ret);
    void set_profile(const QString& profile);
    bool maybe_not_close_tracking();
    void closeEvent(QCloseEvent *e) override;
    void register_shortcuts();
    void set_keys_enabled(bool flag);
private slots:
    void save_modules();
    void exit();
    void profile_selected(const QString& name);

    void showProtocolSettings();
    void show_options_dialog();
    void showCurveConfiguration();
    void showHeadPose();

    void restore_from_tray(QSystemTrayIcon::ActivationReason);
    void maybe_start_profile_from_executable();

    bool make_empty_config_internal();
    void make_copied_config();
    void make_config_from_wizard();
    void open_config_directory();
    void refresh_config_list();

    void startTracker();
    void stopTracker();
    void reload_options();
    void mark_minimized(bool is_minimized);
signals:
    void emit_start_tracker();
    void emit_stop_tracker();
    void emit_toggle_tracker();
    void emit_restart_tracker();

    void emit_minimized(bool);
public:
    MainWindow();
    ~MainWindow();
    static void set_working_directory();
    void warn_on_config_not_writable();
};
