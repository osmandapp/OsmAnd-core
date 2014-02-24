#ifndef _OSMAND_CORE_Q_IMMUTABLE_ITERATOR_H_
#define _OSMAND_CORE_Q_IMMUTABLE_ITERATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>

namespace OsmAnd
{
    template<class ITERATOR_TYPE>
    class QImmutableIterator Q_DECL_FINAL
    {
    public:
        typedef QImmutableIterator<ITERATOR_TYPE> QImmutableIteratorT;
        typedef decltype(*ITERATOR_TYPE()) ValueRefecenceType;
        typedef typename std::add_pointer< typename std::remove_reference<typename ValueRefecenceType>::type >::type ValuePointerType;
    private:
        ITERATOR_TYPE _current;
        ITERATOR_TYPE _end;
    public:
        inline QImmutableIterator(const ITERATOR_TYPE& begin_, const ITERATOR_TYPE& end_)
            : _current(begin_)
            , _end(end_)
            , end(_end)
        {
        }

        inline QImmutableIterator(const QImmutableIteratorT& that)
            : _current(that._current)
            , _end(that._end)
            , end(_end)
        {
        }

        inline ValueRefecenceType operator*() const
        {
            return *_current;
        }

        inline ValuePointerType operator->() const
        {
            return &(*_current);
        }

        inline QImmutableIterator& operator++()
        {
            ++_current;
            return *this;
        }

        inline QImmutableIterator& operator--()
        {
            --_current;
            return *this;
        }

        inline bool operator==(const QImmutableIteratorT& that) const
        {
            return this->_current == that._current && this->_end == that._end;
        }

        inline bool operator!=(const QImmutableIteratorT& that) const
        {
            return this->_current != that._current || this->_end != that._end;
        }

        inline QImmutableIteratorT& operator=(const QImmutableIteratorT& that)
        {
            if(this != &that)
            {
                _current = that._current;
                _end = that._end;
            }
            return *this;
        }

        inline operator ITERATOR_TYPE() const
        {
            return _current;
        }

        inline QImmutableIteratorT& operator=(const ITERATOR_TYPE& that)
        {
            _current = that;
            return *this;
        }

        inline operator bool() const
        {
            return _current != _end;
        }

        const ITERATOR_TYPE& end;

        inline QImmutableIteratorT getEnd() const
        {
            return QImmutableIteratorT(_end, _end);
        }
    };

    template<typename CONTAINER>
    Q_DECL_CONSTEXPR QImmutableIterator<decltype(std::begin(*(new CONTAINER())))> iteratorOf(CONTAINER& container)
    {
        return QImmutableIterator<decltype(std::begin(*(new CONTAINER())))>(std::begin(container), std::end(container));
    }

    template<class CONTAINER>
    Q_DECL_CONSTEXPR QImmutableIterator<decltype(std::begin(CONTAINER()))> iteratorOf(const CONTAINER& container)
    {
        return QImmutableIterator<decltype(std::begin(CONTAINER()))>(std::begin(container), std::end(container));
    }

    template<typename CONTAINER>
    Q_DECL_CONSTEXPR QImmutableIterator<decltype(std::begin(*(new CONTAINER())))> endOf(CONTAINER& container)
    {
        return QImmutableIterator<decltype(std::begin(*(new CONTAINER())))>(std::end(container), std::end(container));
    }

    template<class CONTAINER>
    Q_DECL_CONSTEXPR QImmutableIterator<decltype(std::begin(CONTAINER()))> endOf(const CONTAINER& container)
    {
        return QImmutableIterator<decltype(std::begin(CONTAINER()))>(std::end(container), std::end(container));
    }
}

#endif // !defined(_OSMAND_CORE_Q_IMMUTABLE_ITERATOR_H_)
