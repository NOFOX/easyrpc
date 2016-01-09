/* Copyright(C)
* For free
* All right reserved
* 
*/
/**
* @file RCFServerWrapper.h
* @brief RCF服务端通信框架包装类
* @author highway-9, 787280310@qq.com
* @version 1.1.0
* @date 2016-01-04
*/

#ifndef _RCFSERVERWRAPPER_H
#define _RCFSERVERWRAPPER_H

#include "RCFServerImpl.h"
#include <boost/shared_ptr.hpp>

/**
* @brief RCF服务端通信框架包装类
*
* @tparam T 类类型
*/
template<typename T>
class RCFServerWrapper
{
public:
    /**
    * @brief RCFServerWrapper 构造函数
    */
    RCFServerWrapper();

    /**
    * @brief ~RCFServerWrapper 析构函数
    */
    ~RCFServerWrapper();

    /**
    * @brief init 初始化RCF服务器端
    *
    * @param port 监听端口，默认为50001
    */
    void init(unsigned int port = 50001);

    /**
    * @brief start 开始服务器
    *
    * @return 成功返回true，否则返回false
    */
    bool start();

    /**
    * @brief stop 停止服务器
    *
    * @return 成功返回true，否则返回false
    */
    bool stop();

    /**
    * @brief deinit 反初始化，释放一些资源
    */
    void deinit();

    /**
    * @brief setMessageHandler 设置消息处理类
    *
    * @tparam T2 类类型
    * @param rcfMessageHandler 消息处理类对象
    */
    template<typename T2>
    void setMessageHandler(T2* rcfMessageHandler)
    {
        if (m_impl != NULL)
        {
            m_impl->setMessageHandler(rcfMessageHandler);
        }
    }

private:
    typedef boost::shared_ptr<RCFServerImpl<T> > RCFServerImplPtr;
    RCFServerImplPtr           m_impl;     ///< RCF服务器实现类指针
};

#endif