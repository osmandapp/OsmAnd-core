#ifndef _OSMAND_CORE_Q_CACHING_ITERATOR_H_
#define _OSMAND_CORE_Q_CACHING_ITERATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>

namespace OsmAnd
{
    template<class CONTAINER, class ITERATOR_TYPE>
    class QCachingConstIterator Q_DECL_FINAL
    {
    public:
        typedef QCachingConstIterator<CONTAINER, ITERATOR_TYPE> QCachingConstIteratorT;
        typedef decltype(*ITERATOR_TYPE()) ValueRefecenceType;
        typedef decltype(&(*ITERATOR_TYPE())) ValuePointerType;
    private:
        ITERATOR_TYPE _current;
        ITERATOR_TYPE _end;
    public:
        inline QCachingConstIterator(const CONTAINER& container)
            : _current(std::begin(container))
            , _end(std::end(container))
            , current(_current)
            , end(_end)
        {
        }

        inline QCachingConstIterator(const ITERATOR_TYPE& begin_, const ITERATOR_TYPE& end_)
            : _current(begin_)
            , _end(end_)
            , current(_current)
            , end(_end)
        {
        }

        inline QCachingConstIterator(const QCachingConstIteratorT& that)
            : _current(that._current)
            , _end(that._end)
            , current(_current)
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

        inline QCachingConstIterator& operator++()
        {
            ++_current;
            return *this;
        }

        inline QCachingConstIterator& operator--()
        {
            --_current;
            return *this;
        }

        inline bool operator==(const QCachingConstIteratorT& that) const
        {
            return this->_current == that._current && this->_end == that._end;
        }

        inline bool operator!=(const QCachingConstIteratorT& that) const
        {
            return this->_current != that._current || this->_end != that._end;
        }

        inline QCachingConstIteratorT& operator=(const QCachingConstIteratorT& that)
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

        inline QCachingConstIteratorT& operator=(const ITERATOR_TYPE& that)
        {
            _current = that;
            return *this;
        }

        inline operator bool() const
        {
            return _current != _end;
        }

        const ITERATOR_TYPE& current;
        const ITERATOR_TYPE& end;

        inline QCachingConstIteratorT getEnd() const
        {
            return QCachingConstIteratorT(_end, _end);
        }
    };

    template<class CONTAINER>
    Q_DECL_CONSTEXPR QCachingConstIterator<CONTAINER, decltype(std::begin(CONTAINER()))> iteratorOf(const CONTAINER& container)
    {
        return QCachingConstIterator<CONTAINER, decltype(std::begin(CONTAINER()))>(std::begin(container), std::end(container));
    }

    template<class CONTAINER>
    Q_DECL_CONSTEXPR QCachingConstIterator<CONTAINER, decltype(std::begin(CONTAINER()))> endOf(const CONTAINER& container)
    {
        return QCachingConstIterator<CONTAINER, decltype(std::begin(CONTAINER()))>(std::end(container), std::end(container));
    }

    template<class CONTAINER, class ITERATOR_TYPE>
    class QCachingIterator Q_DECL_FINAL
    {
    public:
        typedef QCachingIterator<CONTAINER, ITERATOR_TYPE> QCachingIteratorT;
        typedef decltype(*ITERATOR_TYPE()) ValueRefecenceType;
        typedef decltype(&(*ITERATOR_TYPE())) ValuePointerType;
    private:
        ITERATOR_TYPE _current;
        ITERATOR_TYPE _end;
    public:
        inline QCachingIterator(CONTAINER& container)
            : _current(std::begin(container))
            , _end(std::end(container))
            , current(_current)
            , end(_end)
        {
        }

        inline QCachingIterator(const ITERATOR_TYPE& begin_, const ITERATOR_TYPE& end_)
            : _current(begin_)
            , _end(end_)
            , current(_current)
            , end(_end)
        {
        }

        inline QCachingIterator(const QCachingIterator& that)
            : _current(that._current)
            , _end(that._end)
            , current(_current)
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

        inline QCachingIterator& operator++()
        {
            ++_current;
            return *this;
        }

        inline QCachingIterator& operator--()
        {
            --_current;
            return *this;
        }

        inline bool operator==(const QCachingIterator& that) const
        {
            return this->_current == that._current && this->_end == that._end;
        }

        inline bool operator!=(const QCachingIterator& that) const
        {
            return this->_current != that._current || this->_end != that._end;
        }

        inline QCachingIterator& operator=(const QCachingIterator& that)
        {
            if(this != &that)
            {
                _current = that._current;
                _end = that._end;
            }
            return *this;
        }

        inline ITERATOR_TYPE get() const
        {
            return _current;
        }

        inline QCachingIterator& set(const ITERATOR_TYPE& that)
        {
            _current = that;
            return *this;
        }

        inline operator bool() const
        {
            return _current != _end;
        }

        const ITERATOR_TYPE& current;
        const ITERATOR_TYPE& end;

        inline QCachingIterator getEnd() const
        {
            return QCachingIterator(_end, _end);
        }

        inline void update(CONTAINER& container)
        {
            _end = std::end(container);
        }
    };

    template<typename CONTAINER>
    Q_DECL_CONSTEXPR QCachingIterator<CONTAINER, decltype(std::begin(*(new CONTAINER())))> endOf(CONTAINER& container)
    {
        return QCachingIterator<CONTAINER, decltype(std::begin(*(new CONTAINER())))>(std::end(container), std::end(container));
    }

    template<typename CONTAINER>
    Q_DECL_CONSTEXPR QCachingIterator<CONTAINER, decltype(std::begin(*(new CONTAINER())))> iteratorOf(CONTAINER& container)
    {
        return QCachingIterator<CONTAINER, decltype(std::begin(*(new CONTAINER())))>(std::begin(container), std::end(container));
    }
}

#endif // !defined(_OSMAND_CORE_Q_CACHING_ITERATOR_H_)
