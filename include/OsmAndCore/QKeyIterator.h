#ifndef _OSMAND_CORE_Q_KEY_ITERATOR_H_
#define _OSMAND_CORE_Q_KEY_ITERATOR_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>

namespace OsmAnd
{
    template<class CONTAINER>
    class KeyContainerWrapper Q_DECL_FINAL
    {
    public:
        typedef KeyContainerWrapper<CONTAINER> KeyContainerWrapperT;
        typedef CONTAINER Container;
        typedef typename CONTAINER::iterator UnderlyingIterator;
        typedef typename CONTAINER::const_iterator UnderlyingConstIterator;
        typedef typename CONTAINER::key_type UnderlyingKey;
        typedef typename std::add_const<typename CONTAINER::key_type>::type UnderlyingConstKey;

    private:
    protected:
        CONTAINER* pContainer;
    public:
        inline KeyContainerWrapper(CONTAINER& container_)
            : pContainer(&container_)
        {
        }

        inline KeyContainerWrapper(const KeyContainerWrapperT& that)
            : pContainer(that.pContainer)
        {
        }

        inline ~KeyContainerWrapper()
        {
        }

        inline KeyContainerWrapperT& operator=(const KeyContainerWrapperT& that)
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

            inline UnderlyingKey operator*() const
            {
                return _underlyingIterator.key();
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

        friend class OsmAnd::KeyContainerWrapper<CONTAINER>;
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

            inline UnderlyingConstKey operator*() const
            {
                return _underlyingIterator.key();
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

        friend class OsmAnd::KeyContainerWrapper<CONTAINER>;
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
    Q_DECL_CONSTEXPR KeyContainerWrapper<CONTAINER> keysOf(CONTAINER& container)
    {
        return KeyContainerWrapper<CONTAINER>(container);
    }

    template<class CONTAINER>
    class KeyConstContainerWrapper Q_DECL_FINAL
    {
    public:
        typedef KeyConstContainerWrapper<CONTAINER> KeyConstContainerWrapperT;
        typedef CONTAINER Container;
        typedef typename CONTAINER::const_iterator UnderlyingConstIterator;
        typedef typename std::add_const<typename CONTAINER::key_type>::type UnderlyingConstKey;

    private:
    protected:
        const CONTAINER* pContainer;
    public:
        inline KeyConstContainerWrapper(const CONTAINER& container_)
            : pContainer(&container_)
        {
        }

        inline KeyConstContainerWrapper(const KeyConstContainerWrapperT& that)
            : pContainer(that.pContainer)
        {
        }

        inline ~KeyConstContainerWrapper()
        {
        }

        inline KeyConstContainerWrapper& operator=(const KeyConstContainerWrapper& that)
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

            inline UnderlyingConstKey operator*() const
            {
                return _underlyingIterator.key();
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

        friend class OsmAnd::KeyConstContainerWrapper<CONTAINER>;
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
    Q_DECL_CONSTEXPR KeyConstContainerWrapper<CONTAINER> keysOf(const CONTAINER& container)
    {
        return KeyConstContainerWrapper<CONTAINER>(container);
    }
}

#endif // !defined(_OSMAND_CORE_Q_KEY_ITERATOR_H_)
