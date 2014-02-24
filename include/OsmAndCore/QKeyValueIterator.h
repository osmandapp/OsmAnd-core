#ifndef _OSMAND_CORE_Q_KEY_VALUE_ITERATOR_H_
#define _OSMAND_CORE_Q_KEY_VALUE_ITERATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

namespace OsmAnd
{
    template<class CONTAINER>
    struct KeyValueContainerWrapper
    {
        typedef KeyValueContainerWrapper<CONTAINER> KeyValueContainerWrapperT;
        typedef CONTAINER Container;
        typedef typename CONTAINER::iterator UnderlyingIterator;
        typedef typename CONTAINER::const_iterator UnderlyingConstIterator;

        CONTAINER& container;

        inline KeyValueContainerWrapper(CONTAINER& container_)
            : container(container_)
        {
        }

        inline KeyValueContainerWrapper(const KeyValueContainerWrapperT& that)
            : container(that.container)
        {
        }

        struct Iterator
        {
            inline UnderlyingIterator operator*() const
            {
                return _underlyingIterator;
            }

            inline Iterator& operator++()
            {
                ++_underlyingIterator;
                return *this;
            }

            inline Iterator& operator--()
            {
                --_underlyingIterator;
                return *this;
            }

            inline bool operator==(const Iterator& that) const
            {
                return this->_underlyingIterator == that._underlyingIterator;
            }

            inline bool operator!=(const Iterator& that) const
            {
                return this->_underlyingIterator != that._underlyingIterator;
            }

        private:
            inline Iterator(const UnderlyingIterator& underlyingIterator_)
                : _underlyingIterator(underlyingIterator_)
            {
            }

            UnderlyingIterator _underlyingIterator;

        friend struct OsmAnd::KeyValueContainerWrapper<CONTAINER>;
        };

        inline Iterator begin()
        {
            return Iterator(container.begin());
        }
        inline Iterator end()
        {
            return Iterator(container.end());
        }

        struct ConstIterator
        {
            inline UnderlyingConstIterator operator*() const
            {
                return _underlyingIterator;
            }

            inline ConstIterator& operator++()
            {
                ++_underlyingIterator;
                return *this;
            }

            inline ConstIterator& operator--()
            {
                --_underlyingIterator;
                return *this;
            }

            inline bool operator==(const ConstIterator& that) const
            {
                return this->_underlyingIterator == that._underlyingIterator;
            }

            inline bool operator!=(const ConstIterator& that) const
            {
                return this->_underlyingIterator != that._underlyingIterator;
            }

        private:
            inline ConstIterator(const UnderlyingConstIterator& underlyingIterator_)
                : _underlyingIterator(underlyingIterator_)
            {
            }

            UnderlyingConstIterator _underlyingIterator;

        friend struct OsmAnd::KeyValueContainerWrapper<CONTAINER>;
        };

        inline ConstIterator begin() const
        {
            return ConstIterator(container.cbegin());
        }
        inline ConstIterator end() const
        {
            return ConstIterator(container.cend());
        }
        inline ConstIterator cbegin() const
        {
            return ConstIterator(container.cbegin());
        }
        inline ConstIterator cend() const
        {
            return ConstIterator(container.cend());
        }

    friend struct OsmAnd::KeyValueContainerWrapper<CONTAINER>;
    };

    template<class CONTAINER>
    Q_DECL_CONSTEXPR KeyValueContainerWrapper<CONTAINER> rangeOf(CONTAINER& container)
    {
        return KeyValueContainerWrapper<CONTAINER>(container);
    }

    template<class CONTAINER>
    struct KeyValueConstContainerWrapper
    {
        typedef KeyValueConstContainerWrapper<CONTAINER> KeyValueConstContainerWrapperT;
        typedef CONTAINER Container;
        typedef typename CONTAINER::const_iterator UnderlyingConstIterator;

        const CONTAINER& container;

        inline KeyValueConstContainerWrapper(const CONTAINER& container_)
            : container(container_)
        {
        }

        inline KeyValueConstContainerWrapper(const KeyValueConstContainerWrapperT& that)
            : container(that.container)
        {
        }

        struct ConstIterator
        {
            inline UnderlyingConstIterator operator*() const
            {
                return _underlyingIterator;
            }

            inline ConstIterator& operator++()
            {
                ++_underlyingIterator;
                return *this;
            }

            inline ConstIterator& operator--()
            {
                --_underlyingIterator;
                return *this;
            }

            inline bool operator==(const ConstIterator& that) const
            {
                return this->_underlyingIterator == that._underlyingIterator;
            }

            inline bool operator!=(const ConstIterator& that) const
            {
                return this->_underlyingIterator != that._underlyingIterator;
            }

        private:
            inline ConstIterator(const UnderlyingConstIterator& underlyingIterator_)
                : _underlyingIterator(underlyingIterator_)
            {
            }

            UnderlyingConstIterator _underlyingIterator;

        friend struct OsmAnd::KeyValueConstContainerWrapper<CONTAINER>;
        };

        inline ConstIterator begin() const
        {
            return ConstIterator(container.cbegin());
        }
        inline ConstIterator end() const
        {
            return ConstIterator(container.cend());
        }
        inline ConstIterator cbegin() const
        {
            return ConstIterator(container.cbegin());
        }
        inline ConstIterator cend() const
        {
            return ConstIterator(container.cend());
        }

        friend struct OsmAnd::KeyValueConstContainerWrapper<CONTAINER>;
    };

    template<class CONTAINER>
    Q_DECL_CONSTEXPR KeyValueConstContainerWrapper<CONTAINER> rangeOf(const CONTAINER& container)
    {
        return KeyValueConstContainerWrapper<CONTAINER>(container);
    }
}

#endif // !defined(_OSMAND_CORE_Q_KEY_VALUE_ITERATOR_H_)
