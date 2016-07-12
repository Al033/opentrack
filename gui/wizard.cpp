#include "wizard.h"
#include "opentrack-logic/state.hpp"
#include "tracker-pt/ftnoir_tracker_pt_settings.h"
#include "filter-accela/ftnoir_filter_accela.h"

Wizard::Wizard() : QWizard(nullptr)
{
    ui.setupUi(this);
    setModal(Qt::ApplicationModal);
    connect(this, SIGNAL(accepted()), this, SLOT(set_data()));
}

static constexpr double tz[][2] = {
    { 16.5327205657959, 13.0232553482056 },
    { 55.4535026550293, 100 },
    { 56.8312301635742, 100 },
    { -1, -1 },
};

static constexpr double yaw[][2] = {
    { 10.7462686567164, 20.9302325581395 },
    { 30, 115 },
    { 41.9517784118652, 180 },
    { -1, -1 },
};

static constexpr double pitch[][2] = {
    { 10.1262916188289, 27.6279069767442 },
    { 32.4454649827784, 180 },
    { -1, -1 },
};

static constexpr double roll[][2] = {
    { 12.3995409011841, 25.9534893035889 },
    { 54.3513221740723, 180 },
    { -1, -1 },
};

static void set_mapping(Mapping& m, const double spline[][2])
{
    m.opts.altp = false;
    m.curve.removeAllPoints();
    for (int i = 0; spline[i][0] >= 0; i++)
        m.curve.addPoint(QPointF(spline[i][0], spline[i][1]));
}

void Wizard::set_data()
{
    Model m;

    if (ui.clip_model->isChecked())
        m = ClipRight;
    else if (ui.clip_model_left->isChecked())
        m = ClipLeft;
    else // ui.cap_model
        m = Cap;

    State state;

    set_mapping(state.pose(TZ), tz);
    set_mapping(state.pose(Yaw), yaw);
    set_mapping(state.pose(Pitch), pitch);
    set_mapping(state.pose(Roll), roll);
    state.pose.save_mappings();

    settings_pt pt;
    pt.threshold = 31;
    pt.min_point_size = 0;
    pt.max_point_size = 50;
    pt.fov = 1;
    pt.camera_mode = 0;
    pt.model_used = m;
    pt.b->save();

    settings_accela acc;
    acc.ewma = 49;
    acc.rot_threshold = 29;
    acc.rot_deadzone = 29;
    acc.trans_deadzone = 33;
    acc.trans_threshold = 19;
    acc.b->save();
}
