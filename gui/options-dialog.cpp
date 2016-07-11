/* Copyright (c) 2015, Stanislaw Halik <sthalik@misaki.pl>

 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#include "options-dialog.hpp"
#include "tracker-pt/camera.h"
#include "keyboard.h"
#include "opentrack-logic/state.hpp"
#include <QPushButton>
#include <QLayout>
#include <QDialog>

static QString kopts_to_string(const key_opts& kopts)
{
    if (static_cast<QString>(kopts.guid) != "")
    {
        const int btn = kopts.button & ~Qt::KeyboardModifierMask;
        const int mods = kopts.button & Qt::KeyboardModifierMask;
        QString mm;
        if (mods & Qt::ControlModifier) mm += "Control+";
        if (mods & Qt::AltModifier) mm += "Alt+";
        if (mods & Qt::ShiftModifier) mm += "Shift+";
        return mm + "Joy button " + QString::number(btn);
    }
    if (static_cast<QString>(kopts.keycode) == "")
        return "None";
    return kopts.keycode;
}

OptionsDialog::OptionsDialog(std::function<void(bool)> pause_keybindings, State& state) :
    pause_keybindings(pause_keybindings),
    state(state),
    trans_calib_running(false)
{
    ui.setupUi(this);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(doOK()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(doCancel()));

    tie_setting(main.tray_enabled, ui.trayp);

    tie_setting(main.center_at_startup, ui.center_at_startup);

    tie_setting(pt.camera_mode, ui.camera_mode);

    tie_setting(pt.threshold, ui.threshold_slider);

    tie_setting(pt.min_point_size, ui.mindiam_spin);
    tie_setting(pt.max_point_size, ui.maxdiam_spin);

    tie_setting(pt.t_MH_x, ui.tx_spin);
    tie_setting(pt.t_MH_y, ui.ty_spin);
    tie_setting(pt.t_MH_z, ui.tz_spin);

    tie_setting(pt.fov, ui.camera_fov);

    tie_setting(pt.model_used, ui.model_used);

    connect(ui.ewma_slider, SIGNAL(valueChanged(int)), this, SLOT(update_ewma_display(int)));
    connect(ui.rotation_slider, SIGNAL(valueChanged(int)), this, SLOT(update_rot_display(int)));
    connect(ui.rot_dz_slider, SIGNAL(valueChanged(int)), this, SLOT(update_rot_dz_display(int)));
    connect(ui.translation_slider, SIGNAL(valueChanged(int)), this, SLOT(update_trans_display(int)));
    connect(ui.trans_dz_slider, SIGNAL(valueChanged(int)), this, SLOT(update_trans_dz_display(int)));

    tie_setting(acc.rot_threshold, ui.rotation_slider);
    tie_setting(acc.trans_threshold, ui.translation_slider);
    tie_setting(acc.ewma, ui.ewma_slider);
    tie_setting(acc.rot_deadzone, ui.rot_dz_slider);
    tie_setting(acc.trans_deadzone, ui.trans_dz_slider);

    update_rot_display(ui.rotation_slider->value());
    update_trans_display(ui.translation_slider->value());
    update_ewma_display(ui.ewma_slider->value());
    update_rot_dz_display(ui.rot_dz_slider->value());
    update_trans_dz_display(ui.trans_dz_slider->value());

    tie_setting(pt.auto_threshold, ui.auto_threshold);

    connect(&timer,SIGNAL(timeout()), this,SLOT(poll_tracker_info()));
    connect( ui.tcalib_button,SIGNAL(toggled(bool)), this,SLOT(startstop_trans_calib(bool)) );

    timer.start(100);

    tie_setting(main.tcomp_p, ui.tcomp_enable);
    tie_setting(main.tcomp_tz, ui.tcomp_rz);

    tie_setting(main.a_yaw.invert, ui.invert_yaw);
    tie_setting(main.a_pitch.invert, ui.invert_pitch);
    tie_setting(main.a_roll.invert, ui.invert_roll);
    tie_setting(main.a_x.invert, ui.invert_x);
    tie_setting(main.a_y.invert, ui.invert_y);
    tie_setting(main.a_z.invert, ui.invert_z);

    tie_setting(main.a_yaw.src, ui.src_yaw);
    tie_setting(main.a_pitch.src, ui.src_pitch);
    tie_setting(main.a_roll.src, ui.src_roll);
    tie_setting(main.a_x.src, ui.src_x);
    tie_setting(main.a_y.src, ui.src_y);
    tie_setting(main.a_z.src, ui.src_z);

    tie_setting(main.camera_yaw, ui.camera_yaw);
    tie_setting(main.camera_pitch, ui.camera_pitch);
    tie_setting(main.camera_roll, ui.camera_roll);

    struct tmp
    {
        key_opts& opt;
        QLabel* label;
        QPushButton* button;
    } tuples[] =
    {
        { main.key_center, ui.center_text, ui.bind_center },
        { main.key_toggle, ui.toggle_text, ui.bind_toggle },
        { main.key_toggle_press, ui.toggle_held_text, ui.bind_toggle_held },
        { main.key_zero, ui.zero_text, ui.bind_zero },
        { main.key_zero_press, ui.zero_held_text, ui.bind_zero_held },
        { main.key_start_tracking, ui.start_tracking_text, ui.bind_start },
        { main.key_stop_tracking, ui.stop_tracking_text , ui.bind_stop},
        { main.key_toggle_tracking, ui.toggle_tracking_text, ui.bind_toggle_tracking },
        { main.key_restart_tracking, ui.restart_tracking_text, ui.bind_restart_tracking }
    };

    for (const tmp& val_ : tuples)
    {
        tmp val = val_;
        val.label->setText(kopts_to_string(val.opt));
        connect(&val.opt.keycode,
                static_cast<void (base_value::*)(const QString&)>(&base_value::valueChanged),
                val.label,
                [=](const QString&) -> void { val.label->setText(kopts_to_string(val.opt)); });
        {
            connect(val.button, &QPushButton::pressed, this, [=]() -> void { bind_key(val.opt, val.label); });
        }
    }
}

void OptionsDialog::bind_key(key_opts& kopts, QLabel* label)
{
    kopts.button = -1;
    kopts.guid = "";
    kopts.keycode = "";
    QDialog d;
    QHBoxLayout l;
    l.setMargin(0);
    KeyboardListener k;
    l.addWidget(&k);
    d.setLayout(&l);
    d.setFixedSize(QSize(500, 300));
    d.setWindowFlags(Qt::Dialog);
    d.setWindowModality(Qt::ApplicationModal);
    connect(&k,
            &KeyboardListener::key_pressed,
            &d,
            [&](QKeySequence s) -> void
            {
                kopts.keycode = s.toString(QKeySequence::PortableText);
                kopts.guid = "";
                kopts.button = -1;
                d.close();
            });
    connect(&k, &KeyboardListener::joystick_button_pressed,
            &d,
            [&](QString guid, int idx, bool held) -> void
            {
                if (!held)
                {
                    kopts.guid = guid;
                    kopts.keycode = "";
                    kopts.button = idx;
                    d.close();
                }
            });
    connect(main.b.get(), &options::detail::opt_bundle::reloading, &d, &QDialog::close);
    pause_keybindings(true);
    d.show();
    d.exec();
    pause_keybindings(false);
    label->setText(kopts_to_string(kopts));
}

void OptionsDialog::doOK() {
    pt.b->save();
    acc.b->save();
    main.b->save();
    ui.game_detector->save();
    close();
    emit saving();
}

void OptionsDialog::doCancel() {
    pt.b->reload();
    acc.b->reload();
    main.b->reload();
    ui.game_detector->revert();
    close();
}

void OptionsDialog::startstop_trans_calib(bool start)
{
    auto tracker = get_pt();
    if (!tracker)
    {
        ui.tcalib_button->setChecked(false);
        return;
    }

    if (start)
    {
        qDebug()<<"TrackerDialog:: Starting translation calibration";
        trans_calib.reset();
        trans_calib_running = true;
        pt.t_MH_x = 0;
        pt.t_MH_y = 0;
        pt.t_MH_z = 0;
    }
    else
    {
        qDebug()<<"TrackerDialog:: Stopping translation calibration";
        trans_calib_running = false;
        {
            auto tmp = trans_calib.get_estimate();
            pt.t_MH_x = tmp[0];
            pt.t_MH_y = tmp[1];
            pt.t_MH_z = tmp[2];
        }
    }
}

void OptionsDialog::poll_tracker_info()
{
    auto tracker = get_pt();
    CamInfo info;
    const bool running = tracker && tracker->get_cam_info(&info);
    if (running)
    {
        QString to_print;

        // display caminfo
        to_print = QString::number(info.res_x)+"x"+QString::number(info.res_y)+" @ "+QString::number(info.fps)+" FPS";
        ui.caminfo_label->setText(to_print);

        // display pointinfo
        int n_points = tracker->get_n_points();
        to_print = QString::number(n_points);
        if (n_points == 3)
            to_print += " OK!";
        else
            to_print += " BAD!";
        ui.pointinfo_label->setText(to_print);

        // update calibration
        if (trans_calib_running) trans_calib_step();
    }
    else
    {
        ui.caminfo_label->setText("Tracker offline");
        ui.pointinfo_label->setText("");
    }
    ui.tcalib_button->setEnabled(running);
}

void OptionsDialog::trans_calib_step()
{
    auto tracker = get_pt();
    if (tracker)
    {
        Affine X_CM = tracker->pose();
        trans_calib.update(X_CM.R, X_CM.t);
    }
}

Tracker_PT* OptionsDialog::get_pt()
{
    auto work = state.work.get();
    if (!work)
        return nullptr;
    auto ptr = work->libs.pTracker;
    if (ptr)
        return static_cast<Tracker_PT*>(ptr.get());
    return nullptr;
}

void OptionsDialog::update_rot_display(int value)
{
    ui.rot_gain->setText(QString::number((value + 1) * 10 / 100.) + "°");
}

void OptionsDialog::update_trans_display(int value)
{
    ui.trans_gain->setText(QString::number((value + 1) * 5 / 100.) + "mm");
}

void OptionsDialog::update_ewma_display(int value)
{
    ui.ewma_label->setText(QString::number(value * 2) + "ms");
}

void OptionsDialog::update_rot_dz_display(int value)
{
    ui.rot_dz->setText(QString::number(value * 2 / 100.) + "°");
}

void OptionsDialog::update_trans_dz_display(int value)
{
    ui.trans_dz->setText(QString::number(value * 1 / 100.) + "mm");
}
