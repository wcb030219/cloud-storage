#ifndef OPEWIDGET_H
#define OPEWIDGET_H

#include <QObject>
#include <QListWidget>
#include "friend.h"
#include "book.h"
#include <QStackedWidget>
#include <QLabel>

class opewidget : public QWidget
{
    Q_OBJECT
public:
    explicit opewidget(QWidget *parent = nullptr);
    static opewidget &getInstance();
    Friend *getFriend();
    Book *getBook();

    void setLoginName(const QString &name);
signals:

private:
    QListWidget *m_pListW;
    Friend *m_pFriend;
    Book *m_pBook;
    QStackedWidget *m_pSW;
    QLabel *m_pUserLabel;
};

#endif // OPEWIDGET_H
