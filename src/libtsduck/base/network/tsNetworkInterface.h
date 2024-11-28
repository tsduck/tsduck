//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of a network interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsIPAddressMask.h"
#include "tsCerrReport.h"
#include "tsSingleton.h"

namespace ts {

    class NetworkInterface;
    using NetworkInterfaceVector = std::vector<NetworkInterface>;  //!< Vector of network interfaces.

    //!
    //! Description of a network interface.
    //! @ingroup net
    //!
    //! The concept of "network interface" is very system-specific. There are physical and virtual
    //! interfaces, interface names and addresses. Here, a NetworkInterface instance describes one
    //! IP address. Depending on the system, several NetworkInterface instances may have the same
    //! interface name.
    //!
    class TSDUCKDLL NetworkInterface : public StringifyInterface
    {
        TS_RULE_OF_FIVE(NetworkInterface, override);
    public:
        NetworkInterface() = default;       //!< Default constructor.
        IPAddressMask address {};           //!< IP address and mask/prefix.
        UString       name {};              //!< Interface name, system specific.
        bool          loopback = false;     //!< This is a software loopback interface.
        int           index = -1;           //!< Interface index, system specific, negative if meaningless.

        static constexpr int AnyIndex = 0;  //!< Interface index value meaning "any interface" in IPv6 system API.

        // Inherited methods.
        virtual UString toString() const override;

        //!
        //! Get the list of all local network interfaces in the system.
        //! @param [out] addresses Receives the list of all local IP interfaces.
        //! @param [in] loopback If false, the loopback addresses are skipped.
        //! @param [in] gen Report addresses for the specified generations only.
        //! @param [in] force_reload If true, force a reload of the list of interfaces.
        //! By default, the list is loaded once and kept in cache. If no network interface
        //! is dynamically added, there is no need to rebuild the list each time.
        //! @param [in] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool GetAll(NetworkInterfaceVector& addresses, bool loopback = true, IP gen = IP::Any, bool force_reload = false, Report& report = CERR);

        //!
        //! Get the list of all local IP addresses in the system.
        //! @param [out] addresses A vector of IPAddress which receives the list of all local IP addresses.
        //! @param [in] loopback If false, the loopback addresses are skipped.
        //! @param [in] gen Report addresses for the specified generations only.
        //! @param [in] force_reload If true, force a reload of the list of interfaces.
        //! By default, the list is loaded once and kept in cache. If no network interface
        //! is dynamically added, there is no need to rebuild the list each time.
        //! @param [in] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool GetAll(IPAddressVector& addresses, bool loopback = true, IP gen = IP::Any, bool force_reload = false, Report& report = CERR);

        //!
        //! Get the list of all local network interfaces by index.
        //! @param [out] indexes Receives the set of indexes of all local IP interfaces.
        //! If an interface has multiple IP addresses, the index is returned only once.
        //! If interface indexes are meaningless for that operating system, the returned set is empty.
        //! @param [in] loopback If false, the loopback addresses are skipped.
        //! @param [in] gen Report addresses for the specified generations only.
        //! @param [in] force_reload If true, force a reload of the list of interfaces.
        //! By default, the list is loaded once and kept in cache. If no network interface
        //! is dynamically added, there is no need to rebuild the list each time.
        //! @param [in] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool GetAll(std::set<int>& indexes, bool loopback = true, IP gen = IP::Any, bool force_reload = false, Report& report = CERR);

        //!
        //! Check if a local system interface has a specified IP address.
        //! @param [in] address The IP address to check.
        //! @param [in] force_reload If true, force a reload of the list of interfaces.
        //! By default, the list is loaded once and kept in cache. If no network interface
        //! is dynamically added, there is no need to rebuild the list each time.
        //! @param [in] report Where to report errors.
        //! @return True is @a address is the address of a local system interface, false otherwise.
        //!
        static bool IsLocal(const IPAddress& address, bool force_reload = false, Report& report = CERR);

        //!
        //! Find the interface index for a local system interface identified by IP address.
        //! @param [in] address The IP address to check.
        //! @param [in] force_reload If true, force a reload of the list of interfaces.
        //! By default, the list is loaded once and kept in cache. If no network interface
        //! is dynamically added, there is no need to rebuild the list each time.
        //! @param [in] report Where to report errors.
        //! @return Interface index for @a address, -1 if @a address if not a local interface.
        //!
        static int ToIndex(const IPAddress& address, bool force_reload = false, Report& report = CERR);

        //!
        //! Find the first IP address of a network interface identified by its index.
        //! @param [out] address The returned IP address.
        //! @param [in] index The interface index to find.
        //! @param [in] gen Report addresses for the specified generations only.
        //! @param [in] force_reload If true, force a reload of the list of interfaces.
        //! By default, the list is loaded once and kept in cache. If no network interface
        //! is dynamically added, there is no need to rebuild the list each time.
        //! @param [in] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool ToAddress(IPAddress& address, int index, IP gen = IP::Any, bool force_reload = false, Report& report = CERR);

    private:
        // The shared repository of local network interfaces.
        class InterfaceRepository
        {
            TS_DECLARE_SINGLETON(InterfaceRepository);
        public:
            std::mutex mutex {};
            NetworkInterfaceVector addresses {};

            // Reload the repository. Must be called with mutex held.
            bool reload(bool force_reload, Report& report);

        private:
            // Add a unique address in the repository.
            void add(const NetworkInterface&);
        };
    };
}
