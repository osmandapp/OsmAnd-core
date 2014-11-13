#ifndef _OSMAND_CORE_Q_KEY_VALUE_ITERATOR_H_
#define _OSMAND_CORE_Q_KEY_VALUE_ITERATOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

namespace OsmAnd
{
    template<class CONTAINER>
    class KeyValueContainerWrapper Q_DECL_FINAL
    {
    public:
        typedef KeyValueContainerWrapper<CONTAINER> KeyValueContainerWrapperT;
        typedef CONTAINER Container;
        typedef typename CONTAINER::iterator UnderlyingIterator;
        typedef typename CONTAINER::const_iterator UnderlyingConstIterator;

    private:
    protected:
        CONTAINER* pContainer;
    public:
        inline KeyValueContainerWrapper(CONTAINER& container_)
            : pContainer(&container_)
        {
        }

        inline KeyValueContainerWrapper(const KeyValueContainerWrapperT& that)
            : pContainer(that.pContainer)
        {
        }

        inline ~KeyValueContainerWrapper()
        {
        }

        inline KeyValueContainerWrapperT& operator=(const KeyValueContainerWrapperT& that)
        {
            if (this != &that)
            {
                pContainer = that.pContainer;
            }
            return *this;
        }

        struct Iterator Q_DECL_FINAL
        {
            inline Iterator(const Iterator& that)
                : _underlyingIterator(that._underlyingIterator)
            {
            }

            inline ~Iterator()
            {
            }

            inline Iterator& operator=(const Iterator& that)
            {
                if (this != &that)
                {
                    _underlyingIterator = that._underlyingIterator;
                }
                return *this;
            }

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

        friend class OsmAnd::KeyValueContainerWrapper<CONTAINER>;
        };
        typedef Iterator iterator;

        inline Iterator begin()
        {
            return Iterator(pContainer->begin());
        }
        inline Iterator end()
        {
            return Iterator(pContainer->end());
        }

        struct ConstIterator Q_DECL_FINAL
        {
            inline ConstIterator(const ConstIterator& that)
                : _underlyingIterator(that._underlyingIterator)
            {
            }

            inline ~ConstIterator()
            {
            }

            inline ConstIterator& operator=(const ConstIterator& that)
            {
                if (this != &that)
                {
                    _underlyingIterator = that._underlyingIterator;
                }
                return *this;
            }

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

        friend class OsmAnd::KeyValueContainerWrapper<CONTAINER>;
        };
        typedef ConstIterator const_iterator;

        inline ConstIterator begin() const
        {
            return ConstIterator(pContainer->cbegin());
        }
        inline ConstIterator end() const
        {
            return ConstIterator(pContainer->cend());
        }
        inline ConstIterator cbegin() const
        {
            return ConstIterator(pContainer->cbegin());
        }
        inline ConstIterator cend() const
        {
            return ConstIterator(pContainer->cend());
        }
    };

    template<class CONTAINER>
    Q_DECL_CONSTEXPR KeyValueContainerWrapper<CONTAINER> rangeOf(CONTAINER& container)
    {
        return KeyValueContainerWrapper<CONTAINER>(container);
    }

    template<class CONTAINER>
    class KeyValueConstContainerWrapper Q_DECL_FINAL
    {
    public:
        typedef KeyValueConstContainerWrapper<CONTAINER> KeyValueConstContainerWrapperT;
        typedef CONTAINER Container;
        typedef typename CONTAINER::const_iterator UnderlyingConstIterator;

    private:
    protected:
        const CONTAINER* pContainer;
    public:
        inline KeyValueConstContainerWrapper(const CONTAINER& container_)
            : pContainer(&container_)
        {
        }

        inline KeyValueConstContainerWrapper(const KeyValueConstContainerWrapperT& that)
            : pContainer(that.pContainer)
        {
        }

        inline ~KeyValueConstContainerWrapper()
        {
        }

        inline KeyValueConstContainerWrapper& operator=(const KeyValueConstContainerWrapper& that)
        {
            if (this != &that)
            {
                pContainer = that.pContainer;
            }
            return *this;
        }

        struct ConstIterator Q_DECL_FINAL
        {
            inline ConstIterator(const ConstIterator& that)
                : _underlyingIterator(that._underlyingIterator)
            {
            }

            inline ~ConstIterator()
            {
            }

            inline ConstIterator& operator=(const ConstIterator& that)
            {
                if (this != &that)
                {
                    _underlyingIterator = that._underlyingIterator;
                }
                return *this;
            }

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

        friend class OsmAnd::KeyValueConstContainerWrapper<CONTAINER>;
        };
        typedef ConstIterator const_iterator;

        inline ConstIterator begin() const
        {
            return ConstIterator(pContainer->cbegin());
        }
        inline ConstIterator end() const
        {
            return ConstIterator(pContainer->cend());
        }
        inline ConstIterator cbegin() const
        {
            return ConstIterator(pContainer->cbegin());
        }
        inline ConstIterator cend() const
        {
            return ConstIterator(pContainer->cend());
        }
    };

    template<class CONTAINER>
    Q_DECL_CONSTEXPR KeyValueConstContainerWrapper<CONTAINER> rangeOf(const CONTAINER& container)
    {
        return KeyValueConstContainerWrapper<CONTAINER>(container);
    }
}

#endif // !defined(_OSMAND_CORE_Q_KEY_VALUE_ITERATOR_H_)
