#include "playpathdialog.h"
#include <qfiledialog.h>
#include <QtGui>
#include "mainwindow.h"
#include "ui_playpathdialog.h"


extern QString playCmdPrefix;

PlayPathDialog::PlayPathDialog(QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::PlayPathDialog)
{
    ui->setupUi(this);
    connect(ui->closeBtn, SIGNAL(released()), this, SLOT(close()));
}

PlayPathDialog::~PlayPathDialog()
{
    delete ui;
}

QString PlayPathDialog::getPlayUrl()
{
    return ui->pathLine->text();
}


void PlayPathDialog::on_bswBtn_released()
{
    QString filename = QFileDialog::getOpenFileName(this, "open file", " ",  "Allfile(*.*)");
    if (!filename.isEmpty()) {
        ui->pathLine->setText(QString("\"") + filename + QString("\""));
    }
}

void PlayPathDialog::on_playBtn_released()
{
    m_isPlay = true;
    close();
}


void PlayPathDialog::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton) {
        m_moving = true;
        m_dragPosition = event->globalPos();
    }
}

void PlayPathDialog::mouseMoveEvent(QMouseEvent* event)
{
    if( event->buttons().testFlag(Qt::LeftButton) && m_moving) {
        this->move(this->pos() + (event->globalPos() - m_dragPosition));
        m_dragPosition = event->globalPos();
    }
}

void PlayPathDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
        m_moving = false;
}
