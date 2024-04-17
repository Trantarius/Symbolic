#pragma once
#include <cstdlib>
#include <stdexcept>
#include <deque>

class RefCounted;

template<typename T>
class Ref;

template<typename T>
class Array;

class RefCounted{
	size_t refcount=0;
protected:
	explicit RefCounted(){}
	template<typename T>
	friend class Ref;

	RefCounted(const RefCounted& b)=delete;
};

#ifndef NDEBUG
template<typename T>
struct NullRefException : public std::runtime_error{};
#endif

template<typename Type>
class Ref{
protected:
	Type* ptr = nullptr;
	inline void set_to(Type* p) {
		if(ptr!=nullptr){
			ptr->refcount--;
			if(ptr->refcount<=0){
				delete ptr;
			}
		}
		ptr = p;
		if(p!=nullptr){
			ptr->refcount++;
		}
	}
	typedef std::add_const<Type>::type ConstType;
public:
	~Ref(){
		set_to(nullptr);
	}

	Ref(){}
	Ref(Type* p){
		set_to(p);
	}
	Ref(const Ref<Type>& b){
		set_to(b.ptr);
	}
	Ref(std::nullptr_t){}

	bool operator==(void* p) const {
		return ptr==p;
	}
	template<typename Other>
	bool operator==(const Ref<Other>& b) const {
		return ptr==b.get_ptr();
	}
	bool operator==(std::nullptr_t) const {
		return ptr==nullptr;
	}

	Type& operator*() const{
#ifndef NDEBUG
		if(ptr==nullptr){
			throw NullRefException<Type>();
		}
#endif
		return *ptr;
	}

	Type* operator->() const{
#ifndef NDEBUG
		if(ptr==nullptr){
			throw NullRefException<Type>();
		}
#endif
		return ptr;
	}

	operator Ref<const Type>() const {
		return Ref<const Type>(ptr);
	}

	Type* get_ptr() const{
		return ptr;
	}

	size_t get_refcount() const{
		return ptr->refcount;
	}

	template<typename Other>
	Ref<Other> cast_to() const{
		return dynamic_cast<Other*>(ptr);
	}
};

template<typename T>
struct Array : public RefCounted, std::deque<T>{
	typedef std::deque<T>::iterator iterator;
	typedef std::deque<T>::const_iterator const_iterator;
	using std::deque<T>::deque;
};
