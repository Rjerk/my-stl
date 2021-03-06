template <typename T, typename Allocator>
void Vector<T, Allocator>::fillAndInitialize(size_type n, const T& value)
{
	elements = allocateAndFill(n, value);
	first_free = elements + n;
	end_of_storage = first_free;
}

template <typename T, typename Allocator>
typename Allocator::pointer Vector<T, Allocator>::allocateAndFill(size_type n, const T& value)
{
	auto result = Allocator::allocate(n);
	std::uninitialized_fill_n(result, n, value);
	return result;
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::doDestroy(iterator beg, iterator end)
{
	if (beg != end) {
		for ( ; beg != end ; ++beg) {
			Allocator::destroy(beg);
		}
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::doDeallocate()
{
	if (elements) {
		Allocator::deallocate(elements, end_of_storage-elements);
		elements = first_free = end_of_storage = nullptr;
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::moveToNewChunk(size_type new_cap)
{
	T* new_data = Allocator::allocate(new_cap);
	T* new_first_free = std::uninitialized_copy(elements, first_free, new_data);
	doDestroy(elements, first_free);
	doDeallocate();
	elements = new_data;
	first_free = new_first_free;
	end_of_storage = elements + new_cap;
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::copyBackward(const_iterator first, const_iterator last, const_iterator pos)
{
	for ( ; last >= first; --last)
		Allocator::construct(pos--, *last);
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::checkAndExpand()
{
	if (first_free == end_of_storage) {
		size_type new_cap = empty() ? VEC_MIN_SIZE : (size() << 1);
		expandCapacity(new_cap);
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::expandCapacity(size_type new_cap)
{
	if (new_cap < capacity()) {
		return ;
	}

	auto new_elem = Allocator::allocate(new_cap);
	auto new_free = new_elem;

	if (std::is_nothrow_move_constructible<value_type>()) {
		for (auto iter = elements; iter != first_free; ++iter) {
			new (new_free) (value_type)(std::move(*iter));
			++new_free;
		}
	} else {
		try {
			new_free = std::uninitialized_copy(elements, first_free, new_elem);
		} catch (...) {
			doDestroy(new_elem, new_free);
			Allocator::deallocate(new_elem, new_cap);
			throw;
		}
	}
	doDestroy(begin(), end());
	doDeallocate();
	elements = new_elem;
	first_free = new_free;
	end_of_storage = new_elem + new_cap;
}

template <typename T, typename Allocator>
Vector<T, Allocator>::Vector(const Allocator& alloc_) noexcept
	: elements(nullptr), first_free(nullptr), end_of_storage(nullptr)
{
}

template <typename T, typename Allocator>
Vector<T, Allocator>::Vector(size_type count, const T& value, const Allocator& alloc_)
	: Allocator(alloc_)
{
	fillAndInitialize(count, value);
}

template <typename T, typename Allocator>
Vector<T, Allocator>::Vector(size_t count, const Allocator& alloc_)
	: Vector(count, T(), alloc_)
{
}

template <typename T, typename Allocator>
Vector<T, Allocator>::Vector(const Vector<T, Allocator>& rhs)
{
	elements = Allocator::allocate(rhs.capacity());
	first_free = std::uninitialized_copy(rhs.elements, rhs.first_free, elements);
	end_of_storage = rhs.end_of_storage;
}

template <typename T, typename Allocator>
Vector<T, Allocator>::Vector(Vector<T, Allocator>&& rhs) noexcept
	: elements(nullptr), first_free(nullptr), end_of_storage(nullptr)
{
	this->swap(rhs);
}

template <typename T, typename Allocator>
Vector<T, Allocator>::Vector(std::initializer_list<T> init, const Allocator& alloc_)
	: Allocator(alloc_)
{
	elements = Allocator::allocate(init.size());
	std::uninitialized_copy_n(init.begin(), init.size(), elements);
	first_free = elements + init.size();
	end_of_storage = first_free;
}

template <typename T, typename Allocator>
Vector<T, Allocator>::~Vector()
{
	if (capacity() != 0) {
		doDestroy(elements, first_free);
		doDeallocate();
	}
}

template <typename T, typename Allocator>
Vector<T, Allocator>&
Vector<T, Allocator>::operator=(Vector rhs) noexcept
{
	this->swap(rhs);
	return *this;
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::assign(size_type count, const T& value)
{
	if (count > capacity()) {
		Vector(count, value).swap(*this);
	} else {
		clear();
		std::fill(elements, elements + count, value);
		first_free = elements + count;
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::assign(std::initializer_list<T> ilist)
{
	if (ilist.size() > capacity()) {
		Vector(ilist).swap(*this);
	} else {
		clear();
		std::copy(ilist.begin(), ilist.end(), elements);
		first_free = elements + ilist.size();
	}
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::allocator_type
Vector<T, Allocator>::get_allocator() const
{
	return allocator_type();
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::reference
Vector<T, Allocator>::at(size_type pos)
{
	if (pos >= size())
		throw std::out_of_range("Vector::at()");
	return *(elements+pos);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reference
Vector<T, Allocator>::at(size_type pos) const
{
	if (pos >= size())
		throw std::out_of_range("Vector::at()");
	return *(elements+pos);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::reference
Vector<T, Allocator>::operator[](size_type pos)
{
	return *(elements + pos);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reference
Vector<T, Allocator>::operator[](size_type pos) const
{
	return *(elements + pos);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::reference
Vector<T, Allocator>::front()
{
	return *elements;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reference
Vector<T, Allocator>::front() const
{
	return *elements;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::reference
Vector<T, Allocator>::back()
{
	return *(first_free-1);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reference
Vector<T, Allocator>::back() const
{
	return *(first_free-1);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::pointer
Vector<T, Allocator>::data() noexcept
{
	return elements;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_pointer
Vector<T, Allocator>::data() const noexcept
{
	return static_cast<const_pointer>(elements);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::begin() noexcept
{
	return elements;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_iterator
Vector<T, Allocator>::begin() const noexcept
{
	return static_cast<const_iterator>(elements);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_iterator
Vector<T, Allocator>::cbegin() const noexcept
{
	return static_cast<const_iterator>(elements);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::end() noexcept
{
	return first_free;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_iterator
Vector<T, Allocator>::end() const noexcept
{
	return static_cast<const_iterator>(first_free);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_iterator
Vector<T, Allocator>::cend() const noexcept
{
	return static_cast<const_iterator>(first_free);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::reverse_iterator
Vector<T, Allocator>::rbegin() noexcept
{
	return static_cast<reverse_iterator>(first_free);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reverse_iterator
Vector<T, Allocator>::rbegin() const noexcept
{
	return static_cast<const_reverse_iterator>(first_free);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reverse_iterator
Vector<T, Allocator>::crbegin() const noexcept
{
	return static_cast<const_reverse_iterator>(first_free);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::reverse_iterator
Vector<T, Allocator>::rend() noexcept
{
	return static_cast<reverse_iterator>(elements);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reverse_iterator
Vector<T, Allocator>::rend() const noexcept
{
	return static_cast<const_reverse_iterator>(elements);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::const_reverse_iterator
Vector<T, Allocator>::crend() const noexcept
{
	return static_cast<const_reverse_iterator>(elements);
}

template <typename T, typename Allocator>
bool Vector<T, Allocator>::empty() const noexcept
{
	return elements == first_free;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::size_type
Vector<T, Allocator>::size() const noexcept
{
	return static_cast<size_type>(first_free - elements);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::size_type
Vector<T, Allocator>::max_size() const noexcept
{
	return static_cast<size_type>(0xffffffffUL / sizeof(T));
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::size_type
Vector<T, Allocator>::capacity() const noexcept
{
	return static_cast<size_type>(end_of_storage - elements);
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::reserve(size_type new_cap)
{
	if (new_cap <= capacity())
		return ;

	moveToNewChunk(new_cap);
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::shrink_to_fit()
{
	if (first_free != end_of_storage) {
		moveToNewChunk(size());
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::clear() noexcept
{
	doDestroy(elements, first_free);
	elements = first_free;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::insert(const_iterator pos, const T& value)
{
	auto c = value;
	return emplace(pos, std::move(c));
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::insert(const_iterator pos, T&& value)
{
	return emplace(pos, std::move(value));
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::insert(const_iterator pos, size_type count, const T& value)
{
	auto p = const_cast<iterator>(pos);
	if (count == 0) {
		return p;
	}
	for (size_type i = 0; i < count; ++i) {
		p = insert(p, value);
	}
	return p;
}

template <typename T, typename Allocator>
template <typename InputIt, typename>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::insert(const_iterator pos, InputIt first, InputIt last)
{
	auto p = const_cast<iterator>(pos);
	if (first == last) {
		return p;
	}
	for (auto iter = first; iter != last; ++iter) {
		p = insert(p, *iter);
		++p;
	}
	return p - (last-first);
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::insert(const_iterator pos, std::initializer_list<T> ilist)
{
	return insert(pos, ilist.begin(), ilist.end());
}

template <typename T, typename Allocator>
template <typename... Args>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::emplace(const_iterator pos, Args&&... args)
{
	if (pos < cbegin() && pos > cend()) {
		throw std::out_of_range("Vector::emplace");
	}
	auto n = pos - cbegin();
	checkAndExpand(); // if expand, all iterators are invalidated.
	auto p = elements + n;

	if (p == first_free) {
		Allocator::construct(first_free++, std::forward<Args>(args)...);
		return first_free - 1;
	} else { // p < cend()
		copyBackward(p, first_free-1, first_free);
		++first_free;
		*p = value_type(std::forward<Args>(args)...);
	}
	return p;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::erase(iterator pos)
{
	if (pos + 1 != end())
		std::copy(pos+1, first_free, pos);
	--first_free;
	Allocator::destroy(first_free);
	return pos;
}

template <typename T, typename Allocator>
typename Vector<T, Allocator>::iterator
Vector<T, Allocator>::erase(iterator first, iterator last)
{
	iterator i = std::copy(last, first_free, first);
	doDestroy(i, first_free);
	first_free = first_free - (last - first);
	return first;
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::push_back(const T& value)
{
	if (first_free != end_of_storage) {
		Allocator::construct(first_free, value);
	} else {
		insert(end(), value);
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::push_back(T&& value)
{
	emplace_back(std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
void Vector<T, Allocator>::emplace_back(Args&&...args)
{
	checkAndExpand();
	Allocator::construct(first_free++, std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::pop_back()
{
	if (first_free > elements) {
		--first_free;
		Allocator::destroy(first_free);
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::resize(size_type new_sz)
{
	resize(new_sz, T());
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::resize(size_type new_sz, const value_type& value)
{
	if (new_sz < size()) {
		erase(elements + new_sz, end());
	} else {
		insert(end(), new_sz - size(), value);
	}
}

template <typename T, typename Allocator>
void Vector<T, Allocator>::swap(Vector& rhs) noexcept
{
	using std::swap;
	swap(elements, rhs.elements);
	swap(first_free, rhs.first_free);
	swap(end_of_storage, rhs.end_of_storage);
}

//  non-menber function

template <typename T, typename Alloc>
void swap(Vector<T, Alloc>& lhs, Vector<T, Alloc>& rhs) noexcept
{
	lhs.swap(rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const Vector<T>& v)
{
	std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
	return out;
}

template <typename T, typename Alloc>
bool operator==(const Vector<T,Alloc>& lhs,
                const Vector<T,Alloc>& rhs)
{
	return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Alloc>
bool operator!=(const Vector<T,Alloc>& lhs,
                const Vector<T,Alloc>& rhs)
{
	return lhs.size() != rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Alloc>
bool operator<(const Vector<T,Alloc>& lhs,
               const Vector<T,Alloc>& rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, typename Alloc>
bool operator<=(const Vector<T,Alloc>& lhs,
                const Vector<T,Alloc>& rhs)
{
	return !(lhs < rhs);
}

template <typename T, typename Alloc>
bool operator>(const Vector<T,Alloc>& lhs,
               const Vector<T,Alloc>& rhs)
{
	return rhs < lhs;
}

template <typename T, typename Alloc>
bool operator>=(const Vector<T,Alloc>& lhs,
                const Vector<T,Alloc>& rhs)
{
	return !(lhs < rhs);
}

