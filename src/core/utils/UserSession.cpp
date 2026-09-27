/**
 * @file UserSession.cpp
 * @brief 当前用户会话管理实现文件
 */

#include "UserSession.h"

UserSession& UserSession::instance()
{
    static UserSession s_instance;
    return s_instance;
}
