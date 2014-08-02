#ifndef _OSMAND_CORE_PRIVATE_IMPLEMENTATION_H_
#define _OSMAND_CORE_PRIVATE_IMPLEMENTATION_H_

#include <OsmAndCore/stdlib_common.h>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

namespace OsmAnd
{
    template<class PIMPL_CLASS>
    class PrivateImplementation Q_DECL_FINAL
    {
        Q_DISABLE_COPY(PrivateImplementation);

    public:
        typedef typename std::remove_const<PIMPL_CLASS>::type NonConstPIMPL;
        typedef typename std::add_const<NonConstPIMPL>::type ConstPIMPL;

    private:
        const std::shared_ptr<NonConstPIMPL> _pimplInstance;
    protected:
    public:
        PrivateImplementation(NonConstPIMPL* const pimplInstance)
            : _pimplInstance(pimplInstance)
        {
        }

        ~PrivateImplementation()
        {
        }

        inline NonConstPIMPL* operator->()
        {
            return _pimplInstance.get();
        }

        inline NonConstPIMPL* get()
        {
            return _pimplInstance.get();
        }

        inline operator NonConstPIMPL*()
        {
            return _pimplInstance.get();
        }

        inline NonConstPIMPL& operator*()
        {
            return *_pimplInstance;
        }

        inline operator std::shared_ptr<NonConstPIMPL>()
        {
            return _pimplInstance;
        }

        inline ConstPIMPL* operator->() const
        {
            return const_cast<ConstPIMPL*>(_pimplInstance.get());
        }

        inline ConstPIMPL* get() const
        {
            return const_cast<ConstPIMPL*>(_pimplInstance.get());
        }

        inline operator ConstPIMPL*() const
        {
            return const_cast<ConstPIMPL*>(_pimplInstance.get());
        }

        inline ConstPIMPL& operator*() const
        {
            return *_pimplInstance;
        }

        inline operator std::shared_ptr<ConstPIMPL>() const
        {
            return _pimplInstance;
        }
    };

    template<class PIMPL_INTERFACE_CLASS>
    class ImplementationInterface Q_DECL_FINAL
    {
        Q_DISABLE_COPY(ImplementationInterface);

    public:
        typedef typename std::remove_const<PIMPL_INTERFACE_CLASS>::type NonConstInterface;
        typedef typename std::add_const<NonConstInterface>::type ConstInterface;

    private:
        NonConstInterface* const _interface;
    protected:
    public:
        ImplementationInterface(NonConstInterface* const interface_)
            : _interface(interface_)
        {
        }

        ~ImplementationInterface()
        {
        }

        inline NonConstInterface* operator->()
        {
            return _interface;
        }

        inline NonConstInterface* get()
        {
            return _interface;
        }

        inline operator NonConstInterface*()
        {
            return _interface;
        }

        inline NonConstInterface& operator*()
        {
            return *_interface;
        }

        inline ConstInterface* operator->() const
        {
            return const_cast<ConstInterface*>(_interface);
        }

        inline ConstInterface* get() const
        {
            return const_cast<ConstInterface*>(_interface);
        }

        inline operator ConstInterface*() const
        {
            return const_cast<ConstInterface*>(_interface);
        }

        inline ConstInterface& operator*() const
        {
            return *_interface;
        }
    };
}

#endif // !defined(_OSMAND_CORE_PRIVATE_IMPLEMENTATION_H_)
