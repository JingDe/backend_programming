
template<typename T>
class NgxListIterator final : public boot::iterator_facade<NgxListIterator<T>, T, boost::single_pass_traverse_tag>
{
public:
	typedef boost::iterator_facade<NgxListIterator<T>, T, boost::single_pass_traverse_tag> super_type;
	typedef typename supter_type::reference reference;

	NgxListIterator(ngx_list_t* l) :m_part(&l->part), //指向链表头结点
		m_data(static_cast<T*>(m_part->elts)) // 指向链表头节点的数组元素
	{}

	NgxListIterator() = default;
	~NgxListIterator() = default;

	NgxListIterator(NgxListIterator const&) = default;
	NgxListIterator& operator=(const NgxListIterator&) = default;

	BOOST_EXPLICIT_OPERATOR_BOOL()

	bool operator!() const
	{
		return !m_part || !m_data || !m_part->nelts;
	}



private:
	friend class boost::iterator_core_access;

	reference dereference() const
	{
		NgxException::require(m_data);
		return m_data[m_count];
	}

	void increment()
	{
		if (!m_part || !m_data)
			return;

		++m_count;
		if (m_count >= m_part->nelts)
		{
			m_count = 0;
			m_part = m_part->next;

			m_data = m_part ? static_cast<T*>(m_part->elts) : nullptr;
		}
	}

	bool equal(NgxListIterator const& o) const
	{
		return m_part == o.m_part && m_data == o.m_data && m_count == o.m_count;
	}

	ngx_list_part_t* m_part = nullptr; // 节点指针
	T* m_data = nullptr; // 节点内数组指针
	ngx_uint_t m_count = 0; // 节点内数组索引
};

template<typename T>
class NgxList final : public NgxWrapper<ngx_list_t>
{
public:
	typedef NgxWrappter<ngx_list_t> super_type;
	typedef NgxList this_type;
	typedef T value_type;

	NgxList(const NgxPool& p, ngx_uint_t n = 10) :super_type(p.list<T>(n))
	{}

	NgxList(ngx_list_t* l) :super_type(l)
	{}

	NgxList(ngx_list_t& l) :super_type(&l)
	{}

	~NgxList() = default;

	T& prepare() const
	{
		auto tmp = ngx_list_push(get());
		NgxException::require(tmp);
		assert(tmp);
		return *reinterpret_cast<T*>(tmp);
	}

	void push(const T& x) const
	{
		prepare() = x;
	}

	bool empty() const
	{
		return !get()->part.nelts;
	}

	void clear() const
	{
		get()->part.nelts = 0;
		get()->part.next = nullptr;
		get()->last = &get()->part;
	}

	template<typename U>
	NgxList<U> reshape(ngx_uint_t n = 0; ngx_pool_t* pool = nullptr) const
	{
		auto rc = ngx_list_init(get(), pool ? pool : get()->pool, n ? n : get()->nalloc, sizeof(U));
		NgxException::require(rc);
		return get();
	}

	void reinit(ngx_uint_t n = 0) const
	{
		reshape<T>(n);
	}

	void merge(const this_type& l) const
	{
		auto f = [this](const value_type& v)
		{
			prepare() = v;
		};
		l.visit(f);
	}

	typedef NgxListIterator<T> iterator;
	typedef const iterator const_iterator;

	iterator begin() const
	{
		return iterator(get());
	}
	iterator end() const
	{
		return iterator();
	}

	template<typename V>
	void visit(V v) const
	{
		auto iter = begin();
		for (; iter; ++iter)
		{
			v(*iter);
		}
	}

	template<typename Predicate>
	iterator find(Predicate p) const
	{
		auto iter = begin();
		for (; iter; ++iter)
		{
			if (p(*iter))
			{
				return iter;
			}
		}
		return end();
	}
};


// usage:

NgxList<ngx_int_t> l(r, 1);

l.push(2000);
l.push(2008);
l.push(2015);

assert(!l.begin() == 2000);

auto p = l.find([](ngx_int_t x) {
	return x == 2000;
});

assert(p != l.end());
assert(*p == 2000);

for (auto p = l.begin(); p; ++p)
{
	std::cout << *p << ", ";
}

