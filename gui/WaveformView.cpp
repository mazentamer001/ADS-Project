#include "WaveformView.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontDatabase>
#include <QtGlobal>
#include <cmath>

namespace {
const int kNameW = 140;   // width of the signal-name column
const int kTopH  = 28;    // height of the time axis
const int kRowH  = 38;    // height of one signal row

QColor colorOf(char v) {
    switch (v) {
        case '1': return QColor(90, 230, 110);
        case '0': return QColor(50, 170, 80);
        case 'Z': return QColor(90, 160, 255);
        default:  return QColor(235, 70, 70);   // X
    }
}
}  // namespace

WaveformView::WaveformView(QWidget* parent)
    : QWidget(parent), endTime_(0), t0_(0), scale_(1), autoFit_(true),
      hasCursor_(false), cursorT_(0), dragging_(false), dragX_(0), dragT0_(0) {
    setMouseTracking(true);
    setMinimumHeight(kTopH + kRowH);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void WaveformView::setData(const QVector<WaveSignal>& waves, int endTime) {
    waves_ = waves;
    endTime_ = endTime;
    setMinimumHeight(kTopH + qMax(1, waves_.size()) * kRowH + 4);
    zoomFit();
}

void WaveformView::clearData() {
    waves_.clear();
    endTime_ = 0;
    setMinimumHeight(kTopH + kRowH);
    update();
}

double WaveformView::xOf(double t) const { return kNameW + (t - t0_) * scale_; }
double WaveformView::tOf(double x) const { return t0_ + (x - kNameW) / scale_; }

void WaveformView::zoomFit() {
    autoFit_ = true;
    const double w = qMax(50, width() - kNameW - 20);
    scale_ = (endTime_ > 0) ? w / (endTime_ * 1.03) : 1.0;
    t0_ = 0;
    update();
}

void WaveformView::zoomAround(double factor, double anchorX) {
    autoFit_ = false;
    const double tm = tOf(anchorX);
    scale_ = qBound(0.02, scale_ * factor, 5000.0);
    t0_ = tm - (anchorX - kNameW) / scale_;
    update();
}

void WaveformView::zoomIn()  { zoomAround(1.5, kNameW + (width() - kNameW) / 2.0); }
void WaveformView::zoomOut() { zoomAround(1.0 / 1.5, kNameW + (width() - kNameW) / 2.0); }

void WaveformView::resizeEvent(QResizeEvent*) {
    if (autoFit_) zoomFit();
}

void WaveformView::wheelEvent(QWheelEvent* e) {
    if (e->angleDelta().y() == 0) return;
    zoomAround(e->angleDelta().y() > 0 ? 1.25 : 0.8, e->pos().x());
    e->accept();
}

void WaveformView::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && e->pos().x() >= kNameW) {
        dragging_ = true;
        dragX_ = e->pos().x();
        dragT0_ = t0_;
    }
}

void WaveformView::mouseMoveEvent(QMouseEvent* e) {
    if (dragging_) {
        autoFit_ = false;
        t0_ = dragT0_ - (e->pos().x() - dragX_) / scale_;
    }
    hasCursor_ = e->pos().x() >= kNameW;
    cursorT_ = qMax(0, int(std::lround(tOf(e->pos().x()))));
    update();
    if (hasCursor_) emit cursorTimeChanged(cursorT_);
}

void WaveformView::mouseReleaseEvent(QMouseEvent*) { dragging_ = false; }

void WaveformView::leaveEvent(QEvent*) {
    hasCursor_ = false;
    update();
}

char WaveformView::valueAt(const WaveSignal& s, int t) {
    char v = 'X';
    for (int k = 0; k < s.times.size(); k++) {
        if (s.times[k] <= t) v = s.values[k];
        else break;
    }
    return v;
}

void WaveformView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(18, 18, 22));

    // ---- wave area (clipped so nothing draws over the name column) ----
    p.save();
    p.setClipRect(kNameW, 0, width() - kNameW, height());

    // time axis ticks
    static const double mult[3] = {1, 2, 5};
    double base = 1;
    int mi = 0;
    double step = 1;
    while (step * scale_ < 70) {
        if (++mi == 3) { mi = 0; base *= 10; }
        step = base * mult[mi];
    }
    const double tEnd = tOf(width());
    double tick = std::floor(qMax(0.0, t0_) / step) * step;
    p.setFont(font());
    for (; tick <= tEnd; tick += step) {
        const double x = xOf(tick);
        p.setPen(QColor(40, 40, 48));
        p.drawLine(QPointF(x, kTopH), QPointF(x, height()));
        p.setPen(QColor(170, 170, 180));
        p.drawText(QPointF(x + 3, 17), QString::number(tick, 'f', 0));
    }
    p.setPen(QColor(70, 70, 80));
    p.drawLine(kNameW, kTopH, width(), kTopH);

    // signals
    for (int i = 0; i < waves_.size(); i++) {
        const WaveSignal& s = waves_[i];
        const int y0 = kTopH + i * kRowH;
        const int hi = y0 + 9, lo = y0 + kRowH - 9, mid = (hi + lo) / 2;
        p.setPen(QColor(38, 38, 46));
        p.drawLine(kNameW, y0 + kRowH, width(), y0 + kRowH);

        QVector<int> ts;
        QVector<char> vs;
        ts.append(0);
        vs.append('X');
        for (int k = 0; k < s.times.size(); k++) {
            ts.append(s.times[k]);
            vs.append(s.values[k]);
        }

        double prevY = -1;
        for (int k = 0; k < ts.size(); k++) {
            const double xs = xOf(ts[k]);
            double xe = xOf(k + 1 < ts.size() ? ts[k + 1] : endTime_);
            if (xe < xs) xe = xs;
            const char v = vs[k];
            const double y = (v == '1') ? hi : (v == '0') ? lo : mid;

            if (xe >= kNameW && xs <= width()) {
                p.setPen(QPen(colorOf(v), 2));
                if (prevY >= 0 && prevY != y) p.drawLine(QPointF(xs, prevY), QPointF(xs, y));
                if (v == 'X') {
                    p.fillRect(QRectF(xs, hi, xe - xs, lo - hi), QColor(235, 70, 70, 60));
                    p.drawLine(QPointF(xs, hi), QPointF(xe, hi));
                    p.drawLine(QPointF(xs, lo), QPointF(xe, lo));
                } else {
                    p.drawLine(QPointF(xs, y), QPointF(xe, y));
                }
            }
            prevY = y;
        }
    }

    // cursor
    if (hasCursor_) {
        const double x = xOf(cursorT_);
        p.setPen(QPen(QColor(255, 210, 60), 1));
        p.drawLine(QPointF(x, 0), QPointF(x, height()));
        p.drawText(QPointF(x + 4, kTopH - 4), QString("t=%1").arg(cursorT_));
    }
    p.restore();

    // ---- name column ----
    p.fillRect(0, 0, kNameW, height(), QColor(28, 28, 34));
    p.setPen(QColor(70, 70, 80));
    p.drawLine(kNameW, 0, kNameW, height());
    p.setPen(QColor(170, 170, 180));
    p.drawText(QPointF(8, 17), "Signals");
    for (int i = 0; i < waves_.size(); i++) {
        const WaveSignal& s = waves_[i];
        const int y0 = kTopH + i * kRowH;
        QColor c = (s.kind == 0) ? QColor(130, 190, 255)
                 : (s.kind == 1) ? QColor(120, 230, 140)
                                 : QColor(190, 190, 200);
        p.setPen(c);
        QString label = s.name;
        if (hasCursor_) label += QString(" = %1").arg(QChar(valueAt(s, cursorT_)));
        p.drawText(QRect(8, y0, kNameW - 12, kRowH), Qt::AlignVCenter | Qt::AlignLeft, label);
    }

    if (waves_.isEmpty()) {
        p.setPen(QColor(130, 130, 140));
        p.drawText(QRect(kNameW, kTopH, width() - kNameW, 60), Qt::AlignCenter,
                   "Press Run (F5) to simulate");
    }
}