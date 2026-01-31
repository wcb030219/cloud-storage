#include "opendb.h"
#include <QMessageBox>
#include <QDebug>
#include <QSqlError>
#include <QCoreApplication>

OpenDB::OpenDB(QObject *parent)
    : QObject{parent}
{
}

OpenDB &OpenDB::getInstance()
{
    static OpenDB instance;
    return instance;
}

void OpenDB::init()
{
    if(QSqlDatabase::drivers().isEmpty())
        qDebug()<<"No database drivers found";

    m_db = QSqlDatabase::addDatabase("QSQLITE");

    QString appPath = QCoreApplication::applicationDirPath();
    m_db.setDatabaseName(appPath + "/cloud_s.db");
    //m_db.setDatabaseName("D:\\QT\\QtSqlite\\cloud_s.db");

    if(!m_db.open()) {
        qDebug()<<"db not open:" << m_db.lastError().text();
        return;
    }

    // 创建userinfo表（如果不存在）
    QSqlQuery query(m_db);
    QString createTableSQL =
        "CREATE TABLE IF NOT EXISTS userinfo ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name VARCHAR(32) UNIQUE NOT NULL, "
        "pwd VARCHAR(32) NOT NULL,"
        "online integer)";
    QString createFriendSQL =
        "CREATE TABLE IF NOT EXISTS friend ("
        "id INTEGER, "
        "friendId INTEGER, "
        "PRIMARY KEY (id, friendId))";

    if(!query.exec(createFriendSQL)) {
        qDebug() << "创建表失败:" << query.lastError().text();
    } else {
        qDebug() << "数据库表已准备好";
    }

    if(!query.exec(createTableSQL)) {
        qDebug() << "创建表失败:" << query.lastError().text();
    } else {
        qDebug() << "数据库表已准备好";
    }
}



bool OpenDB::handleRegist(const char *name, const char *pwd)
{
    if(NULL == name||NULL == pwd){
        return false;
    }

    QSqlQuery query(m_db);

    query.prepare("INSERT INTO userinfo(name, pwd) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(pwd);

    bool success = query.exec();
    if (!success) {
        qDebug() << "插入用户失败:" << query.lastError().text();
        // 如果是用户名重复，可以检查错误类型
        if (query.lastError().nativeErrorCode() == "19") { // SQLite唯一约束错误
            qDebug() << "用户名已存在";
        }
    } else {
        qDebug() << "注册成功:";
    }

    return success;
}

bool OpenDB::handleLogin(const char *name, const char *pwd)
{
    if(NULL == name||NULL == pwd){
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM userinfo WHERE name = ? AND pwd = ? AND online = 0");
    query.addBindValue(name);
    query.addBindValue(pwd);

    if(!query.exec()) {
        qDebug() << "登录查询失败:" << query.lastError().text();
        return false;
    }

    if(query.next()){
        QSqlQuery updateQuery(m_db);
        updateQuery.prepare("UPDATE userinfo SET online = 1 WHERE name = ? AND pwd = ?");
        updateQuery.addBindValue(name);
        updateQuery.addBindValue(pwd);

        if(!updateQuery.exec()) {
            qDebug() << "更新在线状态失败:" << updateQuery.lastError().text();
            return false;
        }
        qDebug() << "登录成功，更新了" << updateQuery.numRowsAffected() << "行";
        return true;
    } else {
        // 检查失败原因
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT online FROM userinfo WHERE name = ? AND pwd = ?");
        checkQuery.addBindValue(name);
        checkQuery.addBindValue(pwd);
        if(checkQuery.exec() && checkQuery.next()) {
            bool isOnline = checkQuery.value(0).toInt() > 0;
            qDebug() << "用户名密码正确，但用户已在线:" << isOnline;
        } else {
            qDebug() << "用户名或密码错误";
        }
        return false;
    }
}

void OpenDB::handleOffline(const char *name)
{
    if(NULL == name){
        return;
    }
    QSqlQuery query(m_db);
    query.prepare("update userinfo set online = 0 where name = ?");
    query.addBindValue(name);
    query.exec();
}

QStringList OpenDB::handleALLOnline()
{
    QSqlQuery query(m_db);
    query.prepare("select name from userinfo where online = 1");
    query.exec();
    QStringList result;
    result.clear();

    while(query.next()){
        result.append(query.value(0).toString());
    }
    return result;
}

int OpenDB::handleSearchUsr(const char *name)
{
    if(NULL == name){
        return -1;
    }
    QSqlQuery query(m_db);
    query.prepare("select online from userinfo where name = ?");
    query.addBindValue(name);
    query.exec();
    if(query.next()){
        int ret = query.value(0).toInt();
        if(1 == ret){
            return 1;
        }else if(0 == ret){
            return 0;
        }
    }else return -1;
}

int OpenDB::handleAddFriend(const char *pername, const char *name)
{
    if(nullptr == pername || nullptr == name){
        return -1;
    }
    QSqlQuery query(m_db);
    query.prepare("select * from friend where (id = (select id from userinfo where name =?) and"
                  "friendId = (select id from userinfo where name = ?)) "
                  "or (id = (select id from userinfo where name =?) and"
                  "friendId = (select id from userinfo where name = ?))");
    query.addBindValue(pername);
    query.addBindValue(name);
    query.addBindValue(name);
    query.addBindValue(pername);
    query.exec();
    if(query.next()){

        return 0; // is my friend
    }else{
        QSqlQuery unquery(m_db);
        unquery.prepare("select online from userinfo where name = ?");
        unquery.addBindValue(pername);
        unquery.exec();
        if(unquery.next()){
            int ret = unquery.value(0).toInt();
            if(1 == ret){
                return 1;  //online
            }else if(0 == ret){
                return 2; // offline
            }
        }else return 3; // no user
    }
}

void OpenDB::handleAgreeAddFriend(const char *pername, const char *name)
{
    if(nullptr == pername || nullptr == name){
        return;
    }
    QSqlQuery query(m_db);
    query.prepare("insert into friend(id,friendId) values((select id from userinfo where name = ?),"
                  "(select id from userinfo where name = ?))");
    query.addBindValue(pername);
    query.addBindValue(name);
    query.exec();

    query.prepare("insert into friend(id,friendId) values((select id from userinfo where name = ?),"
                  "(select id from userinfo where name = ?))");
    query.addBindValue(name);     // A
    query.addBindValue(pername);  // B
    query.exec();
}

QStringList OpenDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();
    if(nullptr == name){
        return strFriendList;
    }
    QSqlQuery query(m_db);
    query.prepare("select name from userinfo where online =1 and "
                  "id in (select id from friend where "
                  "friendId=(select id from userinfo where name= ?))");
    query.addBindValue(name);
    query.exec();
    while(query.next()){
        strFriendList.append(query.value(0).toString());
    }

    query.prepare("select name from userinfo where online =1 and "
                  "id in (select friendId from friend where "
                  "id=(select id from userinfo where name= ?))");
    query.addBindValue(name);
    query.exec();
    strFriendList.removeDuplicates();
    return strFriendList;
}

bool OpenDB::handleDelFriend(const char *name, const char *friendname)
{
    if(nullptr == name || nullptr == friendname){
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("delete from friend where "
                  "id = (select id from userinfo where name = ?)"
                  "and friendId= (select id from userinfo where name = ?)");
    query.addBindValue(name);
    query.addBindValue(friendname);
    query.exec();

    query.prepare("delete from friend where "
                  "id = (select id from userinfo where name = ?)"
                  "and friendId= (select id from userinfo where name = ?)");
    query.addBindValue(friendname);
    query.addBindValue(name);
    query.exec();

    return true;
}





OpenDB::~OpenDB()
{
    m_db.close();
}


