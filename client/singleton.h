#ifndef SINGLETON_H
#define SINGLETON_H

#include <global.h>
#include <iostream>
#include <mutex>
template <typename T>
/*
    单例模式模板类
    任意类（比如 class A、class B）只要继承这个模板类，就能自动变成 “单例”—— 也就是整个程序运行期间，这个类只能创建一个对象，全局共享这一个实例
*/
class Singleton{
protected:
    Singleton()=default;
    Singleton(const Singleton<T>&)=delete ;
    Singleton& operator = (const Singleton<T> & st) = delete ;
    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag,[&](){
           _instance =std::shared_ptr<T> (new T);//思考：为什么是new 而不是make_shared是标准库  make share需要调用构造函数 这里构造函数是私有 不可访问。
        });

        return _instance;
    }
    void PrintAddress(){
        std::cout<<_instance.get()<<std::endl;
    }

    ~Singleton(){
        //std::cout<<"this is singleton destruet"<<std::endl;
    }


};
template <typename T>
std::shared_ptr<T>Singleton<T>::_instance=nullptr;

#endif // SINGLETON_H
