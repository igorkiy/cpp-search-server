#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <type_traits>
#include <cassert>
#include <iostream>
#include <utility>
#include <algorithm>
#include <string_view>

using namespace std;


template <typename Type>
class SingleLinkedList {
    // ========================== определение нода списка ============================================
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next)
            : value(val)
            , next_node(next) {
        }
        Type value;
        Node* next_node = nullptr;
    };

    // ========================== определение итератора списка  =========================================    
    template <typename ValueType>
    class BasicIterator {
        friend class SingleLinkedList;

        explicit BasicIterator(Node* node) {
            node_ = node;
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Type;
        using difference_type = std::ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;

        BasicIterator() = default;

        BasicIterator(const BasicIterator<Type>& other) noexcept {
            node_ = other.node_;
        }

        BasicIterator(const BasicIterator<const Type>& other) noexcept {
            node_ = other.node_;
        }

        BasicIterator& operator=(const BasicIterator<ValueType>& rhs) = default;

        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return (node_ == rhs.node_);
        }

        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return (node_ != rhs.node_);
        }

        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            return (node_ == rhs.node_);
        }

        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return (node_ != rhs.node_);
        }

        BasicIterator& operator++() noexcept {
            if (node_) { node_ = node_->next_node; };
            return *this;
        }

        BasicIterator operator++(int) noexcept {
            auto old_value(*this);
            if (node_) { node_ = node_->next_node; };
            return old_value;
        }

        [[nodiscard]] reference operator*() const noexcept {
            return node_->value;
        }

        [[nodiscard]] pointer operator->() const noexcept {
            return &node_->value;
        }

    private:
        Node* node_ = nullptr;
    };

    //=========================== определение списка ==================================================

        // _____________________ управление итераторами _________________________

public:
    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;

    using Iterator = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

    [[nodiscard]] Iterator begin() noexcept {
        return Iterator(head_.next_node);
    }

    [[nodiscard]] Iterator end() noexcept {
        return Iterator(nullptr);
    }

    [[nodiscard]] ConstIterator begin() const noexcept {
        return ConstIterator(head_.next_node);
    }

    [[nodiscard]] ConstIterator end() const noexcept {
        return ConstIterator(nullptr);
    }

    [[nodiscard]] ConstIterator cbegin() const noexcept {
        return ConstIterator(head_.next_node);
    }

    [[nodiscard]] ConstIterator cend() const noexcept {
        return ConstIterator(nullptr);
    }

    [[nodiscard]] Iterator before_begin() noexcept {
        return Iterator(&head_);
    }

    [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return ConstIterator(const_cast<Node*>(&head_));
    }

    [[nodiscard]] ConstIterator before_begin() const noexcept {
        return ConstIterator(const_cast<Node*>(&head_));
    }


    // _______________________ конструирование и инициализаия списка _______________________________


    Node* last = nullptr;
    SingleLinkedList() : last(nullptr) {}

    ~SingleLinkedList() {
        Clear();
    }

    SingleLinkedList(std::initializer_list<Type> values) {
        if (values.size()) {
            InitRange(values.begin(), values.end());
        }
    }

    SingleLinkedList(const SingleLinkedList& other) {
        if (!other.IsEmpty()) {
            InitRange(other.begin(), other.end());
        }
    }

    SingleLinkedList& operator=(const SingleLinkedList& rhs) {
        if (this != &rhs) {
            InitRange(rhs.begin(), rhs.end());
        }
        return *this;
    }

    template<typename Iterator>  //  так как инициализация диапозоном встречается многократно выносим ее в отдельный метод
    void InitRange(Iterator begin, Iterator end) {
        try {
            SingleLinkedList tmp;
            for (auto it = begin; it != end; ++it) {
                tmp.PushBack(*it);
            }
            this->swap(tmp);
        }
        catch (const std::bad_alloc&) {
            throw;
        }
    }

    void swap(SingleLinkedList& other) noexcept {
        Node* tmp = other.head_.next_node;
        other.head_.next_node = head_.next_node;
        head_.next_node = tmp;
        tmp = other.last;
        other.last = last;
        last = tmp;
    }

    //_________________________________ методы для работы со списком _______________________________________

    [[nodiscard]] size_t GetSize() const noexcept {
        int size = 0;
        Node* p = head_.next_node;
        while (p) {
            size += 1;
            p = p->next_node;
        }
        return size;

    }

    [[nodiscard]] bool IsEmpty() const noexcept {
        return head_.next_node == nullptr;
    }

    void PushFront(const Type& value) {
        head_.next_node = new Node(value, head_.next_node);
        if (!last) {
            last = head_.next_node;
        }
        ++size_;
    }

    void PushBack(const Type& value) {
        if (last) {
            last->next_node = new Node(value, nullptr);
            last = last->next_node;
        }
        else {
            head_.next_node = new Node(value, head_.next_node);
            last = head_.next_node;
        }
        ++size_;
    }

    void PopFront() noexcept {
        if (GetSize()) {
            Node* p = head_.next_node;
            head_.next_node = p->next_node;
            delete p;
        }
    }

    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        auto new_node = new Node(value, pos.node_->next_node);
        pos.node_->next_node = new_node;
        ++size_;
        return Iterator{ pos.node_->next_node };
    }

    Iterator EraseAfter(ConstIterator pos) noexcept {
        if (pos.node_->next_node != nullptr) {
            Node* to_erase = pos.node_->next_node;
            pos.node_->next_node = to_erase->next_node;
            delete to_erase;
            to_erase = nullptr;
            --size_;
        }

        return Iterator{ pos.node_->next_node };
    }

    void Clear() noexcept {
        while (head_.next_node) {
            Node* next = head_.next_node->next_node;
            delete head_.next_node;
            head_.next_node = next;
        }
        head_.next_node = nullptr;
        last = nullptr;
    }

private:
    // Фиктивный узел, используется для вставки "перед первым элементом"
    Node head_;
    size_t size_;
};

//===================================== перегрузка функций ==============================================

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return (lhs.GetSize() == rhs.GetSize() &&
        equal(lhs.cbegin(), lhs.cend(), rhs.cbegin())
        );
}

template <typename Type>
bool operator!=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return (lhs.GetSize() != rhs.GetSize() || lhs != rhs);
}

template <typename Type>
bool operator<(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return lexicographical_compare(lhs.cbegin(), lhs.cend(),
        rhs.cbegin(), rhs.cend());
}

template <typename Type>
bool operator<=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return (lhs < rhs || lhs == rhs);
}

template <typename Type>
bool operator>(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs <= rhs);
}

template <typename Type>
bool operator>=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs < rhs);
}
