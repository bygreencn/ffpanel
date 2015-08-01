#ifndef PLAYPATHDIALOG_H
#define PLAYPATHDIALOG_H

#include <QDialog>

namespace Ui {
class PlayPathDialog;
}

class PlayPathDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlayPathDialog(QWidget *parent = 0);
    ~PlayPathDialog();

    QString          getPlayUrl();
    bool             m_isPlay;

private slots:

    void on_bswBtn_released();

    void on_playBtn_released();

private:
    Ui::PlayPathDialog *ui;
    QPoint    m_dragPosition;
    bool      m_moving;
    void      mousePressEvent(QMouseEvent *event);
    void      mouseMoveEvent(QMouseEvent *event);
    void      mouseReleaseEvent(QMouseEvent* event);
};

#endif // PLAYPATHDIALOG_H
