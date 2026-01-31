#ifndef OPENDB_H
#define OPENDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
class OpenDB : public QObject
{
    Q_OBJECT
public:
    explicit OpenDB(QObject *parent = nullptr);

    static OpenDB& getInstance();
    void init();
    ~OpenDB();

    bool handleRegist(const char *name,const char *pwd); //注册
    bool handleLogin(const char *name,const char *pwd); //
    void handleOffline(const char *name);
    QStringList handleALLOnline();
    int handleSearchUsr(const char *name);
    int handleAddFriend(const char *pername, const char *name);
    void handleAgreeAddFriend(const char *pername, const char *name);
    QStringList handleFlushFriend(const char *name);
    bool handleDelFriend(const char *name, const char *friendname);

signals:

public slots:
private:
    QSqlDatabase m_db;
};


#endif // OPENDB_H
