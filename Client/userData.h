#ifndef USERDATA_H
#define USERDATA_H

#include <QObject>
//用户数据
class UserData : public QObject
{
    Q_OBJECT
public:
    explicit UserData(QObject *parent = nullptr);
public:
    bool isLogin;
};

#endif // USERDATA_H
