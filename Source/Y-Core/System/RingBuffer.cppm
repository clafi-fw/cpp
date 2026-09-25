export module ClaFi.Core.System.RingBuffer;

import ClaFi.StdLib;

namespace ClaFi
{
    // RingBuffer class solely made by Google Vertex Studio AI
    // with no my modification, in two go.
    // (in the first iteration it only missed resizing ability)

    /*
    Because it completely fulfills standard library container traits,
    it inherently works with features like std::ranges, range-based for loops,
    standard algorithms, and custom complex objects seamlessly.

    Usage Example

    // 1. Create a RingBuffer that stores the last 3 strings.
    RingBuffer<std::string> buffer(3);

    // 2. Push elements into it.
    buffer.push_back("Apple");
    buffer.push_back("Banana");
    buffer.push_back("Cherry");

    // "Date" pushes "Apple" out. "Banana" is now the oldest element.
    buffer.push_back("Date");

    // 3. Iterating oldest to latest (Standard Range-based for-loop)
    std::cout << "Current Elements: ";
    for (const auto& item : buffer) {
        std::cout << item << " ";
    }
    std::cout << "\n";
    // Output: Banana Cherry Date

    // 4. Using C++20 Ranges (Reverse the output)
    std::cout << "Reversed via views: ";
    for (const auto& item : buffer | std::views::reverse) {
        std::cout << item << " ";
    }
    std::cout << "\n";
    // Output: Date Cherry Banana

    // 5. Random Access Iterators feature
    std::sort(buffer.begin(), buffer.end()); // sorts perfectly in place
    std::cout << "Sorted alphabetically: ";
    for (const auto& item : buffer) {
         std::cout << item << " ";
    }
    // Output: Banana Cherry Date

    */

    export template <typename T, typename Allocator = std::allocator<T>>
    class RingBuffer {
    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = std::allocator_traits<Allocator>::pointer;
        using const_pointer = std::allocator_traits<Allocator>::const_pointer;

    private:
        allocator_type alloc_;
        pointer data_{ nullptr };
        size_type capacity_{ 0 };
        size_type size_{ 0 };
        size_type head_{ 0 }; // Points to the oldest element

    public:
        // --- Iterators ---
        template <bool IsConst>
        class Iterator {
        public:
            // C++20 iterator concepts & tags
            using iterator_category = std::random_access_iterator_tag;
            using iterator_concept = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = std::conditional_t<IsConst, const T*, T*>;
            using reference = std::conditional_t<IsConst, const T&, T&>;

        private:
            pointer data_{ nullptr };
            size_type capacity_{ 0 };
            size_type head_{ 0 };
            size_type index_{ 0 }; // Logical index from oldest (0) to latest (size - 1)

            template <bool> friend class Iterator;

        public:
            Iterator() = default;

            constexpr Iterator(pointer data, size_type capacity, size_type head, size_type index) noexcept
                : data_(data), capacity_(capacity), head_(head), index_(index) {
            }

            // Implicit conversion from iterator to const_iterator.
            //
            // A TEMPLATE, so that it is not also the copy constructor of Iterator<false>. A
            // constructor taking const Iterator& is exactly that whatever its constraint says,
            // and declaring one suppresses the implicit copy constructor and the move with it -
            // which leaves Iterator<false> uncopyable, so anything returning one by value fails
            // to compile and the type satisfies no iterator concept. A constructor template is
            // never a copy constructor, so both implicit ones stand.
            template <bool OtherConst>
                requires (IsConst && !OtherConst)
            constexpr Iterator(const Iterator<OtherConst>& other) noexcept
                : data_(other.data_), capacity_(other.capacity_), head_(other.head_), index_(other.index_) {
            }

            constexpr reference operator*() const noexcept {
                return *(data_ + (head_ + index_) % capacity_);
            }

            constexpr pointer operator->() const noexcept {
                return data_ + (head_ + index_) % capacity_;
            }

            constexpr Iterator& operator++() noexcept {
                ++index_;
                return *this;
            }

            constexpr Iterator operator++(int) noexcept {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            constexpr Iterator& operator--() noexcept {
                --index_;
                return *this;
            }

            constexpr Iterator operator--(int) noexcept {
                Iterator tmp = *this;
                --(*this);
                return tmp;
            }

            constexpr Iterator& operator+=(difference_type n) noexcept {
                index_ = static_cast<size_type>(static_cast<difference_type>(index_) + n);
                return *this;
            }

            constexpr Iterator& operator-=(difference_type n) noexcept {
                index_ = static_cast<size_type>(static_cast<difference_type>(index_) - n);
                return *this;
            }

            friend constexpr Iterator operator+(Iterator it, difference_type n) noexcept { return it += n; }
            friend constexpr Iterator operator+(difference_type n, Iterator it) noexcept { return it += n; }
            friend constexpr Iterator operator-(Iterator it, difference_type n) noexcept { return it -= n; }

            friend constexpr difference_type operator-(const Iterator& a, const Iterator& b) noexcept {
                return static_cast<difference_type>(a.index_) - static_cast<difference_type>(b.index_);
            }

            constexpr reference operator[](difference_type n) const noexcept {
                return *(*this + n);
            }

            constexpr bool operator==(const Iterator& other) const noexcept {
                return index_ == other.index_ && data_ == other.data_;
            }

            constexpr auto operator<=>(const Iterator& other) const noexcept {
                return index_ <=> other.index_;
            }
        };

        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;

        // --- Constructors & Destructor ---
        constexpr explicit RingBuffer(size_type capacity, const allocator_type& alloc = allocator_type())
            : alloc_(alloc), capacity_(capacity) {
            if (capacity_ > 0) {
                data_ = std::allocator_traits<allocator_type>::allocate(alloc_, capacity_);
            }
        }

        constexpr ~RingBuffer() {
            if (data_) {
                clear();
                std::allocator_traits<allocator_type>::deallocate(alloc_, data_, capacity_);
            }
        }

        // Rule of Five
        constexpr RingBuffer(const RingBuffer& other)
            : alloc_(std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.alloc_)),
            capacity_(other.capacity_) {
            if (capacity_ > 0) {
                data_ = std::allocator_traits<allocator_type>::allocate(alloc_, capacity_);
                try {
                    for (const auto& item : other) {
                        push_back(item);
                    }
                }
                catch (...) {
                    clear();
                    std::allocator_traits<allocator_type>::deallocate(alloc_, data_, capacity_);
                    throw;
                }
            }
        }

        constexpr RingBuffer(RingBuffer&& other) noexcept
            : alloc_(std::move(other.alloc_)), data_(other.data_),
            capacity_(other.capacity_), size_(other.size_), head_(other.head_) {
            other.data_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
            other.head_ = 0;
        }

        constexpr RingBuffer& operator=(const RingBuffer& other) {
            if (this == std::addressof(other)) return *this;
            RingBuffer temp(other);
            swap(temp);
            return *this;
        }

        constexpr RingBuffer& operator=(RingBuffer&& other) noexcept {
            swap(other);
            return *this;
        }

        constexpr void swap(RingBuffer& other) noexcept {
            std::swap(alloc_, other.alloc_);
            std::swap(data_, other.data_);
            std::swap(capacity_, other.capacity_);
            std::swap(size_, other.size_);
            std::swap(head_, other.head_);
        }

        friend constexpr void swap(RingBuffer& a, RingBuffer& b) noexcept { a.swap(b); }

        // --- Capacity & Accessors ---
        constexpr size_type size() const noexcept { return size_; }
        constexpr size_type capacity() const noexcept { return capacity_; }
        constexpr bool empty() const noexcept { return size_ == 0; }
        constexpr bool full() const noexcept { return size_ == capacity_; }
        constexpr float fillRatio() const noexcept { return static_cast<float>(size_) / static_cast<float>(capacity_); }

        constexpr reference front() { return *begin(); }
        constexpr const_reference front() const { return *begin(); }
        constexpr reference back() { return *(--end()); }
        constexpr const_reference back() const { return *(--end()); }

        constexpr reference operator[](size_type i) { return *(begin() + i); }
        constexpr const_reference operator[](size_type i) const { return *(begin() + i); }

        // --- Iterators ---
        constexpr iterator begin() noexcept { return iterator(data_, capacity_, head_, 0); }
        constexpr iterator end() noexcept { return iterator(data_, capacity_, head_, size_); }
        constexpr const_iterator begin() const noexcept { return const_iterator(data_, capacity_, head_, 0); }
        constexpr const_iterator end() const noexcept { return const_iterator(data_, capacity_, head_, size_); }
        constexpr const_iterator cbegin() const noexcept { return begin(); }
        constexpr const_iterator cend() const noexcept { return end(); }

        // --- Modifiers ---
        template <typename... Args>
        constexpr reference emplace_back(Args&&... args) {
            if (capacity_ == 0) throw std::logic_error("Pushing to zero-capacity RingBuffer");

            size_type tail = (head_ + size_) % capacity_;
            if (size_ < capacity_) {
                std::construct_at(data_ + tail, std::forward<Args>(args)...);
                ++size_;
            }
            else {
                // Overwrite oldest item
                std::destroy_at(data_ + tail);
                std::construct_at(data_ + tail, std::forward<Args>(args)...);
                head_ = (head_ + 1) % capacity_; // Shift the head forward
            }
            return *(data_ + tail);
        }

        constexpr void push_back(const T& value) { emplace_back(value); }
        constexpr void push_back(T&& value) { emplace_back(std::move(value)); }

        constexpr void pop_front() noexcept {
            if (size_ > 0) {
                std::destroy_at(data_ + head_);
                head_ = (head_ + 1) % capacity_;
                --size_;
            }
        }

        constexpr void clear() noexcept {
            for (size_type i = 0; i < size_; ++i) {
                std::destroy_at(data_ + (head_ + i) % capacity_);
            }
            size_ = 0;
            head_ = 0;
        }
        // --- Capacity Management ---

        // Changes the maximum capacity of the RingBuffer.
        // If shrinking, it safely retains the LATEST elements.
        constexpr void set_capacity(size_type new_capacity) {
            if (new_capacity == capacity_) return;

            pointer new_data = nullptr;
            if (new_capacity > 0) {
                new_data = std::allocator_traits<allocator_type>::allocate(alloc_, new_capacity);
            }

            // Figure out how many elements we can keep, and skip the oldest ones if we shrink
            size_type new_size = std::min(size_, new_capacity);
            size_type elements_to_skip = size_ - new_size;

            size_type i = 0;
            try {
                // Move elements to the new buffer
                for (; i < new_size; ++i) {
                    std::construct_at(new_data + i, std::move_if_noexcept((*this)[elements_to_skip + i]));
                }
            }
            catch (...) {
                // Strong exception guarantee: clean up new buffer if move throws, leave old intact
                for (size_type j = 0; j < i; ++j) {
                    std::destroy_at(new_data +
                        j);
                }
                if (new_capacity > 0) {
                    std::allocator_traits<allocator_type>::deallocate(alloc_, new_data, new_capacity);
                }
                throw;
            }

            // Destroy moved-from elements and deallocate old buffer
            clear();
            if (capacity_ > 0) {
                std::allocator_traits<allocator_type>::deallocate(alloc_, data_, capacity_);
            }

            data_ = new_data;
            capacity_ = new_capacity;
            size_ = new_size;
            head_ = 0; // New buffer is laid out contiguously from index 0
        }

        // --- Resizing ---

        // Resizes the active number of elements.
        // New elements are default-constructed.
        constexpr void resize(size_type new_size) {
            if (new_size > capacity_) {
                set_capacity(new_size); // Auto-grow capacity if requested size is larger
            }

            if (new_size < size_) {
                // Shrink: destroy the newest elements at the tail
                for (size_type i = new_size; i < size_; ++i) {
                    std::destroy_at(data_ + (head_ + i) % capacity_);
                }
            }
            else if (new_size > size_) {
                // Grow: default-construct new elements at the tail
                for (size_type i = size_; i < new_size; ++i) {
                    std::construct_at(data_ + (head_ + i) % capacity_);
                }
            }
            size_ = new_size;
        }

        // Resizes the active number of elements.
        // New elements are copy-constructed from `value`.
        constexpr void resize(size_type new_size, const T& value) {
            if (new_size > capacity_) {
                set_capacity(new_size);
            }

            if (new_size < size_) {
                for (size_type i = new_size; i < size_; ++i) {
                    std::destroy_at(data_ + (head_ + i) % capacity_);
                }
            }
            else if (new_size > size_) {
                for (size_type i = size_; i < new_size; ++i) {
                    std::construct_at(data_ + (head_ + i) % capacity_, value);
                }
            }
            size_ = new_size;
        }
    };

}
