// =============================================================================
// WaveformView.hpp -- GTKWave-style digital waveform viewer.
//   mouse wheel = zoom around the pointer, left-drag = pan,
//   hover = cursor with the value of every signal at that time.
// =============================================================================
#ifndef WAVEFORMVIEW_HPP
#define WAVEFORMVIEW_HPP

#include "SimRunner.hpp"
#include <QWidget>

class WaveformView : public QWidget {
    Q_OBJECT
public:
    explicit WaveformView(QWidget* parent = nullptr);

    void setData(const QVector<WaveSignal>& waves, int endTime);
    void clearData();

signals:
    void cursorTimeChanged(int t);   // emitted while the mouse hovers over the waves

public slots:
    void zoomFit();
    void zoomIn();
    void zoomOut();

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent*) override;

private:
    static char valueAt(const WaveSignal& s, int t);
    double xOf(double t) const;
    double tOf(double x) const;
    void zoomAround(double factor, double anchorX);

    QVector<WaveSignal> waves_;
    int    endTime_;
    double t0_;          // time at the left edge of the wave area
    double scale_;       // pixels per time unit
    bool   autoFit_;     // keep fitting to the window until the user zooms
    bool   hasCursor_;
    int    cursorT_;
    bool   dragging_;
    int    dragX_;
    double dragT0_;
};

#endif