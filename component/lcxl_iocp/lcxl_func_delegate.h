#ifndef _LCXL_FUNC_DELEGATE_H_
#define _LCXL_FUNC_DELEGATE_H_
//////////////////////////////////////////////////////////////////////////
//本头文件实现了类成员函数的委托功能
//Author: LCXL
//////////////////////////////////////////////////////////////////////////

///<summary>
///事件模版，函数委托
///T:被委托者的class，可以让委托者的对象冒充被委托者
///E:委托的函数类型
///用法：
///1.定义事件类
///     typedef void (A::*EEventCallBack)(XX XX, int x, int y);
///     class A;//前置申明
///     
///     typedef _LCXLFunctionDelegate<A, EEventCallBack> EMessageEvent;
///		class A {
///		public:
///			
///     public:
///         void SetEMessageEvent(AMessageEvent &a_event) {
///               this->m_a_message_event = a_event;
///         }
///         //触发A事件
///         void DelegateAEvent() {
///				(m_a_message_event.GetInvoker()->*m_a_message_event.Delegate())(0, 0);
///             //或者为：TRIGGER_DELEGATE(m_a_message_event)(0, 0);
///         }  
///     private:
///         AMessageEvent m_e_message_event;
///     }
///
///2.使用
///     class Host{
///		private:
///			A a;
///			void HostAEvent(int X, int Y);
///		public:
///			Host() {
///				A::EMessageEvent event(this, (EEventCallBack)&Host::HostAEvent);
///             //设置事件
///				a.SetAMessageEvent(event);
///				//触发事件
///             a.DelegateAEvent();
///			}
///		}
///(XXX.GetInvoker()->*XXX.Delegate())(XXX, XXX);
///</summary>
template <typename T, typename E>
struct _LCXLFunctionDelegate {
	//调用者
	void * invoker;
	//类成员回调函数
	E delegate_func;
	_LCXLFunctionDelegate():invoker(NULL), delegate_func(NULL) {

	}
	_LCXLFunctionDelegate(void * invoker, E delegate_func):invoker(invoker),delegate_func(delegate_func)  {

	}
	T *GetInvoker() {
		return (T *)invoker;
	}
	//委托函数
	E Delegate() {
		return delegate_func;
	}
	BOOL IsAvaliable() {
		return (invoker!=NULL&&delegate_func!=NULL);
	}
};

///<summary>
///触发委托者函数的宏，用法为DELEGATE(_LCXLFunctionDelegate类型的变量)(委托函数的参数)
///</summary>
#define TRIGGER_DELEGATE(__A) ((__A).GetInvoker()->*(__A).Delegate())

///<summary>
///触发委托者函数的宏，用法为DELEGATE(_LCXLFunctionDelegate类型的变量指针)(委托函数的参数)
///</summary>
#define TRIGGER_DELEGATE_P(__A) ((__A)->GetInvoker()->*(__A)->Delegate())

#endif