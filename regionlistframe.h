#ifndef REGIONLISTFRAME_H
#define REGIONLISTFRAME_H

#include <QFrame>
#include <QStringList>
#include<QListWidgetItem>

namespace Ui {
    class RegionListFrame;
}

class RegionListFrame : public QFrame
{
    Q_OBJECT

private:
    QStringList mRegionDescriptions;

public:
    explicit RegionListFrame(QWidget *parent = 0);
    ~RegionListFrame();

    void setRegionData( const QStringList &info );

    void moveToBottomLeftCorner();

public slots:
    void listCurrentItemChanged( QListWidgetItem *, QListWidgetItem * );

signals:
    // this is emitted when the current region selection is changed
    void currentRegionChanged(int newRegionIdx);

private:
    Ui::RegionListFrame *ui;
};

#endif // REGIONLISTFRAME_H
