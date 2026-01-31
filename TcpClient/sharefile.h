#ifndef SHAREFILE_H
#define SHAREFILE_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QScrollArea>
#include <QListWidget>
#include <QCheckBox>



class ShareFile : public QWidget
{
    Q_OBJECT
public:
    explicit ShareFile(QWidget *parent = nullptr);
    static ShareFile &getInstance();
    void updateFriend(QListWidget *pFriendList);
    // 设置要分享的文件信息
    void setFileInfo(const QString &strPath, const QString &strFileName);

public slots:
    void selectAll();
    void cancelSelect();
    void okShare();
    void cancelShare();

signals:

private:
    QPushButton *m_pSelectAllPB;
    QPushButton *m_pCancelSelectPB;
    QPushButton *m_pOKPB;
    QPushButton *m_pCancelPB;

    QScrollArea *m_pSA;
    QWidget *m_pFriendW;
    QVBoxLayout *m_pFriendWVBL;
    QButtonGroup *m_pButtonGroup;


    QString m_strFilePath;      // 当前路径
    QString m_strFileName;      // 文件名
};

#endif // SHAREFILE_H
