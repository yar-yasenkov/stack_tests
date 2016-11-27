#include <iostream>
#include <stdexcept>
#include <memory>
#include <thread>
#include <mutex>




class bitset
{
public:
	explicit
		bitset(size_t size) /*strong*/;
	bitset(bitset const & other) = delete;
	auto operator =(bitset const & other)->bitset & = delete;
	bitset(bitset && other) = delete;
	auto operator =(bitset && other)->bitset & = delete;
	auto set(size_t index) /*strong*/ -> void;
	auto reset(size_t index) /*strong*/ -> void;
	auto test(size_t index) /*strong*/ -> bool;
	auto counter() /*noexcept*/ -> size_t;
	auto size() /*noexcept*/ -> size_t;
private:
	std::unique_ptr<bool[]>  ptr_;
	size_t size_;
	size_t counter_;
};
auto bitset::size()-> size_t {
	return size_;
}
auto bitset::counter()-> size_t {
	return counter_;
}
bitset::bitset(size_t size) :
	ptr_(std::make_unique<bool[]>(size)),
	size_(size), counter_(0) {
}
auto bitset::reset(size_t index)->void {
	if (index >= 0 && index < size_) { ptr_[index] = false; --counter_; }
	else throw std::logic_error("Bad Value");
}
auto bitset::set(size_t index)->void {
	if (index >= 0 && index < size_) { ptr_[index] = true; ++counter_; }
	else throw std::logic_error("Bad Value");
}
auto bitset::test(size_t index) ->bool {
	if (index >= 0 && index < size_) return !ptr_[index];
	else throw std::logic_error("Bad Value");
}

//__________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________

template <typename T>
class allocator
{
public:
	explicit
		allocator(std::size_t size = 0) /*strong*/;
	allocator(allocator const & other) /*strong*/;
	auto operator =(allocator const & other)->allocator & = delete;
	~allocator();/*noexcept*/

	auto resize() /*strong*/ -> void;

	auto construct(T * ptr, T const & value) /*strong*/ -> void;
	auto destroy(T * ptr) /*noexcept*/ -> void;

	auto get() /*noexcept*/ -> T *;
	auto get() const /*noexcept*/ -> T const *;

	auto count() const /*noexcept*/ -> size_t;
	auto full() const /*noexcept*/ -> bool;
	auto empty() const /*noexcept*/ -> bool;
	auto swap(allocator & other) /*noexcept*/ -> void;
private:
	auto destroy(T * first, T * last) /*noexcept*/ -> void;

	size_t size_;
	T * ptr_;
	std::unique_ptr<bitset> map_;
};

//__________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________




template <typename T>//конструктор с параметром 
allocator<T>::allocator(size_t size) : ptr_(static_cast<T *>(size == 0 ? nullptr : operator new(size * sizeof(T)))), size_(size), map_(std::make_unique<bitset>(size)) {

};

template<typename T>//конструктор копирования 
allocator<T>::allocator(allocator const & tmp) :allocator<T>(tmp.size_) {
	for (size_t i = 0; i < size_; ++i) {
		construct(ptr_ + i, tmp.ptr_[i]);
	}
}

template <typename T>//деструктор
allocator<T>::~allocator() {
	if (this->count() > 0) {
		destroy(ptr_, ptr_ + size_);
	}
	operator delete(ptr_);
};

template <typename T>//реализация свап
auto allocator<T>::swap(allocator & other)->void {
	std::swap(ptr_, other.ptr_);
	std::swap(size_, other.size_);
	std::swap(map_, other.map_);
};

template <typename T>//инициализация
auto allocator<T>::construct(T * ptr, T const & value)->void {
	if (ptr < ptr_ && ptr >= ptr_ + size_) {
		throw std::out_of_range("Error");
	}
	new(ptr) T(value);
	map_->set(ptr - ptr_);



}

template <typename T>
auto allocator<T>::destroy(T * ptr)->void
{
	if (!map_->test(ptr - ptr_) && ptr >= ptr_&&ptr <= ptr_ + this->count())
	{
		ptr->~T();
		map_->reset(ptr - ptr_);
	}else {
		throw std::out_of_range("Error");
	}
}


template <typename T>
auto allocator<T>::destroy(T * first, T * last)->void
{
	if (first >= ptr_&&last <= ptr_ + this->count()) {
		for (; first != last; ++first) destroy(&*first);
		
	}else {
		throw std::out_of_range("Error");
	}
}

template<typename T>//увеличиваем память
auto allocator<T>::resize()-> void {
	allocator<T> buff(size_ * 2 + (size_ == 0));
	for (size_t i = 0; i < size_; ++i) {
		if (buff.map_->test(i))
		{
			buff.construct(buff.get() + i, ptr_[i]);
		}
	}
	this->swap(buff);
}

template<typename T>//проверка на пустоту
auto allocator<T>::empty() const -> bool {
	return (map_->counter() == 0);
}

template<typename T>//проверка на заполненность
auto allocator<T>::full() const -> bool {
	return (map_->counter() == size_);
}

template<typename T>//получить ptr_
auto allocator<T>::get() -> T * {
	return ptr_;
}

template<typename T>//получить ptr_ const метод
auto allocator<T>::get() const -> T const * {
	return ptr_;
}

template<typename T>//вернуть count_
auto allocator<T>::count() const -> size_t {
	return map_->counter();
}




//__________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________

template <typename T>
class stack
{
public:
	explicit
		stack(size_t size = 0);/*strong*/
	auto operator =(stack const & other) /*strong*/ -> stack &;
	stack(stack const & other);/*strong*/
	auto empty() const /*noexcept*/ -> bool;
	auto count() const /*noexcept*/ -> size_t;
	auto push(T const & value) /*strong*/ -> void;
	auto pop() /*strong*/ -> void;
	auto top() /*strong*/ -> T &;
	auto top() const /*strong*/ -> T const &;
	auto operator==(stack const & rhs) -> bool; /*noexcept*/

private:
	allocator<T> allocate;
	mutable std::mutex mutex_;
};
//__________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________

template <typename T>
stack<T>::stack(stack const & tmp) {
	std::lock_guard<std::mutex> lock(tmp.mutex_);
	allocator<T> alloc = tmp.allocate;
	allocate.swap(alloc);
}

template <typename T>
stack<T>::stack(size_t size) : allocate(size),mutex_(){};

template<typename T>
auto stack<T>::empty() const->bool {
	std::lock_guard<std::mutex> lock(mutex_);
	return (allocate.count() == 0);
}


template <typename T>
auto stack<T>::push(T const &val)->void {
	std::lock_guard<std::mutex> lock(mutex_);
	if (allocate.full()) {
		allocate.resize();
	}
	allocate.construct(allocate.get() + this->count(), val);
}



template <typename T>
auto stack<T>::operator=(const stack &tmp)->stack& {
	std::lock(mutex_, tmp.mutex_);
	std::lock_guard<std::mutex> self_lock(mutex_, std::adopt_lock);
	std::lock_guard<std::mutex> other_lock(tmp.mutex_, std::adopt_lock);
	if (this != &tmp) {
		(allocator<T>(tmp.allocate)).swap(this->allocate);
	}
	return *this;
}


template <typename T>
auto stack<T>::count() const->size_t {
	std::lock_guard<std::mutex> lock(mutex_);
	return allocate.count();
}

template <typename T>
auto stack<T>::pop()->void {
	std::lock_guard<std::mutex> lock(mutex_);
	if (this->count() == 0) throw std::logic_error("Empty!");
	allocate.destroy(allocate.get() + (this->count() - 1));
}

template <typename T>
auto stack<T>::top() const->const T&{
	std::lock_guard<std::mutex> lock(mutex_);
	if (this->count() == 0) throw std::logic_error("Empty!");
return(*(allocate.get() + this->count() - 1));

}

template <typename T>
auto stack<T>::top()->T& {
	std::lock_guard<std::mutex> lock(mutex_);
	if (this->count() == 0) throw std::logic_error("Empty!");
	return(*(allocate.get() + this->count() - 1));
}
template<typename T> /*noexcept*/
inline auto stack<T>::operator==(stack const & rhs) -> bool {
	std::lock_guard<std::mutex> lock(mutex_);
	if (rhs.allocate.count() != this->allocate.count()) {
		return false;
	}
	else {
		for (size_t i = 0; i < allocate.count(); i++) {
			if (this->allocate.get()[i] != rhs.allocate.get()[i]) {
				return false;
			}
		}
	}
	return true;
}
