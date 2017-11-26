#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QImage>

class Worker : public QObject
{
    Q_OBJECT
    public:
        Worker(const QImage& img): bin(img) {}

    public slots:
        void doWork();
    signals:
        void workFinished();
        void EnableView(bool);
    private:
        QImage bin;
};

#endif // WORKER_H
